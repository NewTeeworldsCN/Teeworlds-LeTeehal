/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <new>
#include <engine/shared/config.h>
#include <game/server/lc/expedition/balance.h>
#include <game/server/lc/hazards/hazards.h>
#include <game/server/core/gamecontext.h>
#include <game/server/lc/ui/gameplay_ui.h>
#include <game/collision.h>
#include <game/mapitems.h>

#include "character.h"
#include "laser.h"
#include "../lc/monster.h"
#include "projectile.h"
#include "../lc/ship.h"
#include "../vehicle/vehicle_util.h"
#include <engine/shared/protocol.h>

//input count
struct CInputCount
{
	int m_Presses;
	int m_Releases;
};

CInputCount CountInput(int Prev, int Cur)
{
	CInputCount c = {0, 0};
	Prev &= INPUT_STATE_MASK;
	Cur &= INPUT_STATE_MASK;
	int i = Prev;

	while(i != Cur)
	{
		i = (i+1)&INPUT_STATE_MASK;
		if(i&1)
			c.m_Presses++;
		else
			c.m_Releases++;
	}

	return c;
}


MACRO_ALLOC_POOL_ID_IMPL(CCharacter, MAX_CLIENTS)

// Character, "physical" player's part
CCharacter::CCharacter(CGameWorld *pWorld)
: CEntity(pWorld, CGameWorld::ENTTYPE_CHARACTER)
{
	m_ProximityRadius = ms_PhysSize;
	m_Health = 0;
	m_Armor = 0;
}

void CCharacter::Reset()
{
	Destroy();
}

bool CCharacter::Spawn(CPlayer *pPlayer, vec2 Pos)
{
	m_EmoteStop = -1;
	m_LastAction = -1;
	m_LastNoAmmoSound = -1;
	m_ActiveWeapon = WEAPON_HAMMER;
	m_LastWeapon = WEAPON_HAMMER;
	m_QueuedWeapon = -1;
	m_InShip = false;
	m_LastHazardWarnTick = 0;
	m_LastHazardType = 0;
	m_LastProximityWarnTick = 0;
	m_LastCompassTick = 0;
	m_LastFacilityRoomType = 0;
	m_LastZoneBand = -1;
	m_LastSpikeGridX = -100000;
	m_LastSpikeGridY = -100000;
	m_SpeedBoostUntilTick = 0;
	m_OnVehicle = false;
	m_VehicleSeat = VEHICLE_SEAT_NONE;
	m_VehicleDismountTick = 0;

	m_pPlayer = pPlayer;
	if(Server()->GetClientSession(GetPlayer()->GetCID())->m_RoundId == GameServer()->m_pController->m_RoundId && Server()->m_LocateGame == LOCATE_GAME)
	{
		vec2 Saved = vec2(Server()->GetClientSession(GetPlayer()->GetCID())->m_X, Server()->GetClientSession(GetPlayer()->GetCID())->m_Y);
		if(GameServer()->m_pController->IsSpawnSafe(Saved))
			Pos = Saved;
		m_Freeze = Server()->GetClientSession(GetPlayer()->GetCID())->m_Freeze;
	}
	m_Pos = Pos;

	m_Core.Reset();
	m_Core.Init(&GameServer()->m_World.m_Core, GameServer()->Collision());
	m_Core.m_Pos = m_Pos;
	GameServer()->m_World.m_Core.m_apCharacters[m_pPlayer->GetCID()] = &m_Core;

	m_ReckoningTick = 0;
	mem_zero(&m_SendCore, sizeof(m_SendCore));
	mem_zero(&m_ReckoningCore, sizeof(m_ReckoningCore));

	GameServer()->m_World.InsertEntity(this);
	m_Alive = true;

	GameServer()->m_pController->OnCharacterSpawn(this);
	m_HookMode = 0;

	GameServer()->ResetVotes(GetPlayer()->GetCID());

	m_LeekTick = -1;

	Server()->GetClientSession(GetPlayer()->GetCID())->m_RoundId = GameServer()->m_pController->m_RoundId;
	return true;
}

void CCharacter::Destroy()
{
	m_HookMode = 0;
	GameServer()->m_World.m_Core.m_apCharacters[m_pPlayer->GetCID()] = 0;
	m_Alive = false;
}

void CCharacter::SetWeapon(int W)
{
	if(W == m_ActiveWeapon)
		return;

	m_LastWeapon = m_ActiveWeapon;
	m_QueuedWeapon = -1;
	m_ActiveWeapon = W;
	GameServer()->CreateSound(m_Pos, SOUND_WEAPON_SWITCH);

	if(m_ActiveWeapon < 0 || m_ActiveWeapon >= NUM_WEAPONS)
		m_ActiveWeapon = 0;
}

bool CCharacter::IsGrounded()
{
	if(GameServer()->Collision()->CheckPoint(m_Pos.x+m_ProximityRadius/2, m_Pos.y+m_ProximityRadius/2+5))
		return true;
	if(GameServer()->Collision()->CheckPoint(m_Pos.x-m_ProximityRadius/2, m_Pos.y+m_ProximityRadius/2+5))
		return true;
	return false;
}


void CCharacter::HandleNinja()
{
	return;
}


void CCharacter::DoWeaponSwitch()
{
	// make sure we can switch
	if(m_ReloadTimer != 0 || m_QueuedWeapon == -1)
		return;

	// switch Weapon
	SetWeapon(m_QueuedWeapon);
}

void CCharacter::HandleWeaponSwitch()
{
	// select Weapon
	int Next = CountInput(m_LatestPrevInput.m_NextWeapon, m_LatestInput.m_NextWeapon).m_Presses;
	int Prev = CountInput(m_LatestPrevInput.m_PrevWeapon, m_LatestInput.m_PrevWeapon).m_Presses;

	{
		int WantedWeapon = m_ActiveWeapon;
		if (m_QueuedWeapon != -1)
			WantedWeapon = m_QueuedWeapon;

		if (Next < 128) // make sure we only try sane stuff
		{
			while (Next) // Next Weapon selection
			{
				WantedWeapon = (WantedWeapon + 1) % NUM_WEAPONS;
				if (m_aWeapons[WantedWeapon].m_Got)
					Next--;
			}
		}

		if (Prev < 128) // make sure we only try sane stuff
		{
			while (Prev) // Prev Weapon selection
			{
				WantedWeapon = (WantedWeapon - 1) < 0 ? NUM_WEAPONS - 1 : WantedWeapon - 1;
				if (m_aWeapons[WantedWeapon].m_Got)
					Prev--;
			}
		}

		// Direct Weapon selection
		if (m_LatestInput.m_WantedWeapon)
			WantedWeapon = m_Input.m_WantedWeapon - 1;

		// check for insane values
		if (WantedWeapon >= 0 && WantedWeapon < NUM_WEAPONS && WantedWeapon != m_ActiveWeapon && m_aWeapons[WantedWeapon].m_Got)
			m_QueuedWeapon = WantedWeapon;

		DoWeaponSwitch();
	}
}

void CCharacter::FireWeapon()
{
	if(m_pPlayer->m_TerminalMenuFireBlock)
		return;

	if(m_ReloadTimer != 0)
		return;

	if(m_Freeze)
		return;

	if(m_OnVehicle)
		return;

	DoWeaponSwitch();
	vec2 Direction = normalize(vec2(m_LatestInput.m_TargetX, m_LatestInput.m_TargetY));

	bool FullAuto = false;
	if(m_ActiveWeapon == WEAPON_GRENADE || m_ActiveWeapon == WEAPON_SHOTGUN || m_ActiveWeapon == WEAPON_RIFLE)
		FullAuto = true;


	// check if we gonna fire
	bool WillFire = false;
	if(CountInput(m_LatestPrevInput.m_Fire, m_LatestInput.m_Fire).m_Presses)
		WillFire = true;

	if(FullAuto && (m_LatestInput.m_Fire&1) && m_aWeapons[m_ActiveWeapon].m_Ammo)
		WillFire = true;

	if(!WillFire)
		return;

	// check for ammo
	if(!m_aWeapons[m_ActiveWeapon].m_Ammo)
	{
		// 125ms is a magical limit of how fast a human can click
		m_ReloadTimer = 125 * Server()->TickSpeed() / 1000;
		if(m_LastNoAmmoSound+Server()->TickSpeed() <= Server()->Tick())
		{
			GameServer()->CreateSound(m_Pos, SOUND_WEAPON_NOAMMO);
			m_LastNoAmmoSound = Server()->Tick();
		}
		return;
	}

	vec2 ProjStartPos = m_Pos+Direction*m_ProximityRadius*0.75f;

	switch(m_ActiveWeapon)
	{
		case WEAPON_HAMMER:
		{
			// reset objects Hit
			m_NumObjectsHit = 0;
			GameServer()->CreateSound(m_Pos, SOUND_HAMMER_FIRE);

			CCharacter *apEnts[MAX_CLIENTS];
			int Hits = 0;
			int Num = GameServer()->m_World.FindEntities(ProjStartPos, m_ProximityRadius*0.5f, (CEntity**)apEnts,
														MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);

			for (int i = 0; i < Num; ++i)
			{
				CCharacter *pTarget = apEnts[i];

				if ((pTarget == this) || GameServer()->Collision()->IntersectLine(ProjStartPos, pTarget->m_Pos, NULL, NULL))
					continue;

				if(pTarget->m_Freeze && pTarget->TryReviveBy(m_pPlayer->GetCID()))
				{
					Hits++;
					continue;
				}

				// set his velocity to fast upward (for now)
				if(length(pTarget->m_Pos-ProjStartPos) > 0.0f)
					GameServer()->CreateHammerHit(pTarget->m_Pos-normalize(pTarget->m_Pos-ProjStartPos)*m_ProximityRadius*0.5f);
				else
					GameServer()->CreateHammerHit(ProjStartPos);

				vec2 Dir;
				if (length(pTarget->m_Pos - m_Pos) > 0.0f)
					Dir = normalize(pTarget->m_Pos - m_Pos);
				else
					Dir = vec2(0.f, -1.f);

				int D = 1;
				if(GetPlayer()->m_Hand == SCRAP_L2_SIGN)
					D += 2;

				pTarget->TakeDamage(vec2(0.f, -1.f) + normalize(Dir + vec2(0.f, -1.1f)) * 10.0f, D,
					m_pPlayer->GetCID(), m_ActiveWeapon);
				Hits++;
			}

			CMonster *apMonsters[MAX_MONSTERS];
			int NumMonsters = GameServer()->m_World.FindEntities(ProjStartPos, m_ProximityRadius*0.5f, (CEntity**)apMonsters,
				MAX_MONSTERS, CGameWorld::ENTTYPE_MONSTER);
			for(int i = 0; i < NumMonsters; ++i)
			{
				CMonster *pTarget = apMonsters[i];
				if(!GameServer()->PlayerCanDamageMonster(pTarget))
					continue;
				if(GameServer()->Collision()->IntersectLine(ProjStartPos, pTarget->m_Pos, NULL, NULL))
					continue;

				if(length(pTarget->m_Pos-ProjStartPos) > 0.0f)
					GameServer()->CreateHammerHit(pTarget->m_Pos-normalize(pTarget->m_Pos-ProjStartPos)*m_ProximityRadius*0.5f);
				else
					GameServer()->CreateHammerHit(ProjStartPos);

				vec2 Dir;
				if(length(pTarget->m_Pos - m_Pos) > 0.0f)
					Dir = normalize(pTarget->m_Pos - m_Pos);
				else
					Dir = vec2(0.f, -1.f);

				int D = GC_PLAYER_HAMMER_MONSTER_DMG;
				if(GetPlayer()->m_Hand == SCRAP_L2_SIGN)
					D += GC_PLAYER_SIGN_MONSTER_BONUS;

				GameServer()->DamageMonsterFromPlayer(pTarget, m_pPlayer->GetCID(), m_ActiveWeapon, D,
					vec2(0.f, -1.f) + normalize(Dir + vec2(0.f, -1.1f)) * 10.0f);
				Hits++;
			}

			PickupScrap();

			// if we Hit anything, we have to wait for the reload
			if(Hits)
				m_ReloadTimer = Server()->TickSpeed()/3;

		} break;

		case WEAPON_GUN:
		{
			CProjectile *pProj = new CProjectile(GameWorld(), WEAPON_GUN,
				m_pPlayer->GetCID(),
				ProjStartPos,
				Direction,
				(int)(Server()->TickSpeed()*GameServer()->Tuning()->m_GunLifetime),
				1, 0, 0, -1, WEAPON_GUN);

			GameServer()->CreateSound(m_Pos, SOUND_GUN_FIRE);
		} break;

		case WEAPON_SHOTGUN:
		{
			int ShotSpread = 1;

			for(int i = -ShotSpread; i <= ShotSpread; ++i)
			{
				float Spreading[] = {-0.070f, 0, 0.070f};
				float a = GetAngle(Direction);
				a += Spreading[i+1];
				float v = 1-(absolute(i)/(float)ShotSpread);
				float Speed = mix((float)GameServer()->Tuning()->m_ShotgunSpeeddiff, 1.0f, v);
				CProjectile *pProj = new CProjectile(GameWorld(), WEAPON_SHOTGUN,
					m_pPlayer->GetCID(),
					ProjStartPos,
					vec2(cosf(a), sinf(a))*Speed,
					(int)(Server()->TickSpeed()*GameServer()->Tuning()->m_ShotgunLifetime),
					1, 0, 0, -1, WEAPON_SHOTGUN);
			}

			GameServer()->CreateSound(m_Pos, SOUND_SHOTGUN_FIRE);
		} break;

		case WEAPON_GRENADE:
		{
			CProjectile *pProj = new CProjectile(GameWorld(), WEAPON_GRENADE,
				m_pPlayer->GetCID(),
				ProjStartPos,
				Direction,
				(int)(Server()->TickSpeed()*GameServer()->Tuning()->m_GrenadeLifetime),
				1, true, 0, SOUND_GRENADE_EXPLODE, WEAPON_GRENADE);

			GameServer()->CreateSound(m_Pos, SOUND_GRENADE_FIRE);
		} break;

		case WEAPON_RIFLE:
		{
			new CLaser(GameWorld(), m_Pos, Direction, GameServer()->Tuning()->m_LaserReach, m_pPlayer->GetCID());
			GameServer()->CreateSound(m_Pos, SOUND_RIFLE_FIRE);
		} break;

		case WEAPON_NINJA:
		{
			// reset objects Hit
			m_NumObjectsHit = 0;
			GameServer()->CreateSound(m_Pos, SOUND_NINJA_FIRE);

			CCharacter *apEnts[MAX_CLIENTS];
			int Hits = 0;
			int Num = GameServer()->m_World.FindEntities(ProjStartPos, m_ProximityRadius*0.5f, (CEntity**)apEnts,
														MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);

			for (int i = 0; i < Num; ++i)
			{
				CCharacter *pTarget = apEnts[i];

				if ((pTarget == this) || GameServer()->Collision()->IntersectLine(ProjStartPos, pTarget->m_Pos, NULL, NULL))
					continue;

				// set his velocity to fast upward (for now)
				if(length(pTarget->m_Pos-ProjStartPos) > 0.0f)
					GameServer()->CreateHammerHit(pTarget->m_Pos-normalize(pTarget->m_Pos-ProjStartPos)*m_ProximityRadius*0.5f);
				else
					GameServer()->CreateHammerHit(ProjStartPos);

				vec2 Dir;
				if (length(pTarget->m_Pos - m_Pos) > 0.0f)
					Dir = normalize(pTarget->m_Pos - m_Pos);
				else
					Dir = vec2(0.f, -1.f);

				pTarget->TakeDamage(vec2(0.f, -1.f) + normalize(Dir + vec2(0.f, -1.1f)) * 10.0f, 20,
					m_pPlayer->GetCID(), m_ActiveWeapon);
				Hits++;
			}

			CMonster *apMonsters[MAX_MONSTERS];
			int NumMonsters = GameServer()->m_World.FindEntities(ProjStartPos, m_ProximityRadius*0.5f, (CEntity**)apMonsters,
				MAX_MONSTERS, CGameWorld::ENTTYPE_MONSTER);
			for(int i = 0; i < NumMonsters; ++i)
			{
				CMonster *pTarget = apMonsters[i];
				if(!GameServer()->PlayerCanDamageMonster(pTarget))
					continue;
				if(GameServer()->Collision()->IntersectLine(ProjStartPos, pTarget->m_Pos, NULL, NULL))
					continue;

				if(length(pTarget->m_Pos-ProjStartPos) > 0.0f)
					GameServer()->CreateHammerHit(pTarget->m_Pos-normalize(pTarget->m_Pos-ProjStartPos)*m_ProximityRadius*0.5f);
				else
					GameServer()->CreateHammerHit(ProjStartPos);

				vec2 Dir;
				if(length(pTarget->m_Pos - m_Pos) > 0.0f)
					Dir = normalize(pTarget->m_Pos - m_Pos);
				else
					Dir = vec2(0.f, -1.f);

				GameServer()->DamageMonsterFromPlayer(pTarget, m_pPlayer->GetCID(), m_ActiveWeapon, GC_PLAYER_NINJA_MONSTER_DMG,
					vec2(0.f, -1.f) + normalize(Dir + vec2(0.f, -1.1f)) * 10.0f);
				Hits++;
			}

			// if we Hit anything, we have to wait for the reload
			if(Hits)
				m_ReloadTimer = Server()->TickSpeed()/3;

		} break;

	}

	m_AttackTick = Server()->Tick();

	if(m_aWeapons[m_ActiveWeapon].m_Ammo > 0) // -1 == unlimited
		m_aWeapons[m_ActiveWeapon].m_Ammo--;

	if(!m_ReloadTimer)
		m_ReloadTimer = g_pData->m_Weapons.m_aId[m_ActiveWeapon].m_Firedelay * Server()->TickSpeed() / 1000;
}

void CCharacter::TickVehicleWeapon()
{
	if(!m_OnVehicle || m_Freeze || m_pPlayer->m_TerminalMenuFireBlock)
		return;

	if(m_ReloadTimer > 0)
	{
		m_ReloadTimer--;
		return;
	}

	if(!(m_Input.m_Fire & 1))
		return;

	vec2 Direction = vec2(m_Input.m_TargetX, m_Input.m_TargetY);
	if(length(Direction) < 0.001f)
		return;
	Direction = normalize(Direction);

	vec2 ProjStartPos = m_Pos + Direction * (float)ms_PhysSize * 0.75f;
	new CProjectile(GameWorld(), WEAPON_GUN, m_pPlayer->GetCID(), ProjStartPos, Direction,
		(int)(Server()->TickSpeed() * GameServer()->Tuning()->m_GunLifetime),
		1, 0, 0, -1, WEAPON_GUN);
	GameServer()->CreateSound(m_Pos, SOUND_GUN_FIRE);
	m_AttackTick = Server()->Tick();
	m_ReloadTimer = g_pData->m_Weapons.m_aId[WEAPON_GUN].m_Firedelay * Server()->TickSpeed() / 1000;
}

void CCharacter::HandleWeapons()
{
	//ninja
	HandleNinja();

	// check reload timer
	if(m_ReloadTimer)
	{
		m_ReloadTimer--;
		return;
	}

	// fire Weapon, if wanted
	FireWeapon();

	// ammo regen
	int AmmoRegenTime = g_pData->m_Weapons.m_aId[m_ActiveWeapon].m_Ammoregentime;
	if(AmmoRegenTime)
	{
		// If equipped and not active, regen ammo?
		if (m_ReloadTimer <= 0)
		{
			if (m_aWeapons[m_ActiveWeapon].m_AmmoRegenStart < 0)
				m_aWeapons[m_ActiveWeapon].m_AmmoRegenStart = Server()->Tick();

			if ((Server()->Tick() - m_aWeapons[m_ActiveWeapon].m_AmmoRegenStart) >= AmmoRegenTime * Server()->TickSpeed() / 1000)
			{
				// Add some ammo
				m_aWeapons[m_ActiveWeapon].m_Ammo = min(m_aWeapons[m_ActiveWeapon].m_Ammo + 1, 10);
				m_aWeapons[m_ActiveWeapon].m_AmmoRegenStart = -1;
			}
		}
		else
		{
			m_aWeapons[m_ActiveWeapon].m_AmmoRegenStart = -1;
		}
	}

	return;
}

bool CCharacter::GiveWeapon(int Weapon, int Ammo)
{
	if(m_aWeapons[Weapon].m_Ammo < g_pData->m_Weapons.m_aId[Weapon].m_Maxammo || !m_aWeapons[Weapon].m_Got)
	{
		m_aWeapons[Weapon].m_Got = true;
		m_aWeapons[Weapon].m_Ammo = min(g_pData->m_Weapons.m_aId[Weapon].m_Maxammo, Ammo);
		return true;
	}
	return false;
}

void CCharacter::GiveNinja()
{
	m_Ninja.m_ActivationTick = Server()->Tick();
	m_aWeapons[WEAPON_NINJA].m_Got = true;
	m_aWeapons[WEAPON_NINJA].m_Ammo = -1;
	if (m_ActiveWeapon != WEAPON_NINJA)
		m_LastWeapon = m_ActiveWeapon;
	m_ActiveWeapon = WEAPON_NINJA;

	GameServer()->CreateSound(m_Pos, SOUND_PICKUP_NINJA);
}

void CCharacter::SetEmote(int Emote, int Tick)
{
	m_EmoteType = Emote;
	m_EmoteStop = Tick;
}

void CCharacter::OnPredictedInput(CNetObj_PlayerInput *pNewInput)
{
	// check for changes
	if(mem_comp(&m_Input, pNewInput, sizeof(CNetObj_PlayerInput)) != 0)
		m_LastAction = Server()->Tick();

	// copy new input
	mem_copy(&m_Input, pNewInput, sizeof(m_Input));
	m_NumInputs++;

	// it is not allowed to aim in the center
	if(m_Input.m_TargetX == 0 && m_Input.m_TargetY == 0)
		m_Input.m_TargetY = -1;
}

void CCharacter::OnDirectInput(CNetObj_PlayerInput *pNewInput)
{
	mem_copy(&m_LatestPrevInput, &m_LatestInput, sizeof(m_LatestInput));
	mem_copy(&m_LatestInput, pNewInput, sizeof(m_LatestInput));

	// it is not allowed to aim in the center
	if(m_LatestInput.m_TargetX == 0 && m_LatestInput.m_TargetY == 0)
		m_LatestInput.m_TargetY = -1;

	if(m_NumInputs > 2 && m_pPlayer->GetTeam() != TEAM_SPECTATORS)
	{
		HandleWeaponSwitch();
		FireWeapon();
	}

	mem_copy(&m_LatestPrevInput, &m_LatestInput, sizeof(m_LatestInput));
}

void CCharacter::ResetInput()
{
	m_Input.m_Direction = 0;
	m_Input.m_Hook = 0;
	// simulate releasing the fire button
	if((m_Input.m_Fire&1) != 0)
		m_Input.m_Fire++;
	m_Input.m_Fire &= INPUT_STATE_MASK;
	m_Input.m_Jump = 0;
	m_LatestPrevInput = m_LatestInput = m_Input;
}

void CCharacter::SyncDirectInput(const CNetObj_PlayerInput *pNewInput)
{
	if(!pNewInput)
		return;

	m_LatestInput = *pNewInput;
	if(m_LatestInput.m_TargetX == 0 && m_LatestInput.m_TargetY == 0)
		m_LatestInput.m_TargetY = -1;
	m_LatestPrevInput = m_LatestInput;
}

void CCharacter::Tick()
{
	if(m_OnVehicle)
	{
		UpdateTuningParam();
		m_PrevInput = m_Input;
		VehicleResetCharacterHook(this);
		return;
	}

	UpdateTuningParam();

	m_Core.m_Input = m_Input;

	CCharacterCore::CParams CoreTickParams(&m_pPlayer->m_NextTuningParams);
	CoreTickParams.m_HookMode = m_HookMode;

	vec2 PrevPos = m_Core.m_Pos;
	m_Core.Tick(true, &CoreTickParams);

	if(m_LeekTick > 0)
	{
		m_LeekTick--;
		if(Server()->Tick() % Server()->TickSpeed() == 0)
		{
			int Time = m_LeekTick/50;
			GameServer()->SendBroadcast(GetPlayer()->GetCID(), BROADCAST_PRIORITY_INTERFACE, BROADCAST_DURATION_REALTIME, _("[生命维持系统] 检测到生命危险！请立刻回到飞船!\n剩余时间: {sec:tick}"), "tick", &Time);
		}
		if(m_LeekTick == 0)
			Die(GetPlayer()->GetCID(), WEAPON_NINJA);
	}
	HandleHazards();
	HandleCompass();
	// handle Weapons
	HandleWeapons();

	if(m_Freeze)
	{
		if(m_InShip)
		{
			GameServer()->SendChatTarget(GetPlayer()->GetCID(), _("[生命维持系统-飞船]已为您修复生命维持系统"));
			IncreaseHealth(3);
			m_Freeze = false;
		}
		else if(Server()->Tick() % (Server()->TickSpeed() * 3) == 0)
			GameServer()->SendBroadcast(GetPlayer()->GetCID(), BROADCAST_PRIORITY_EFFECTSTATE, BROADCAST_DURATION_GAMEANNOUNCE, _("你的生命危在旦夕，叫你的队友来救你\n(把你钩回飞船上)"));
	}
	// Previnput
	m_PrevInput = m_Input;

	Server()->GetClientSession(GetPlayer()->GetCID())->m_X = m_Pos.x;
	Server()->GetClientSession(GetPlayer()->GetCID())->m_Y = m_Pos.y;
	Server()->GetClientSession(GetPlayer()->GetCID())->m_Freeze = m_Freeze;
	return;
}

void CCharacter::TickDefered()
{
	if(m_OnVehicle)
	{
		// Aircraft ticks after character; position is synced in CVehicle::Tick().
		m_SendCore = m_Core;
		m_ReckoningCore = m_Core;
		m_ReckoningTick = Server()->Tick();
		Server()->GetClientSession(GetPlayer()->GetCID())->m_X = m_Pos.x;
		Server()->GetClientSession(GetPlayer()->GetCID())->m_Y = m_Pos.y;
		Server()->GetClientSession(GetPlayer()->GetCID())->m_Freeze = m_Freeze;
		return;
	}

	// advance the dummy
	{
		CCharacterCore::CParams CoreTickParams(&GameWorld()->m_Core.m_Tuning);
		CWorldCore TempWorld;
		m_ReckoningCore.Init(&TempWorld, GameServer()->Collision());
		m_ReckoningCore.Tick(false, &CoreTickParams);
		m_ReckoningCore.Move(&CoreTickParams);
		m_ReckoningCore.Quantize();
	}

	CCharacterCore::CParams CoreTickParams(&m_pPlayer->m_NextTuningParams);

	//lastsentcore
	vec2 StartPos = m_Core.m_Pos;
	vec2 StartVel = m_Core.m_Vel;
	bool StuckBefore = GameServer()->Collision()->TestBox(m_Core.m_Pos, vec2(28.0f, 28.0f));

	m_Core.Move(&CoreTickParams);
	bool StuckAfterMove = GameServer()->Collision()->TestBox(m_Core.m_Pos, vec2(28.0f, 28.0f));
	m_Core.Quantize();
	bool StuckAfterQuant = GameServer()->Collision()->TestBox(m_Core.m_Pos, vec2(28.0f, 28.0f));
	m_Pos = m_Core.m_Pos;

	if(!StuckBefore && (StuckAfterMove || StuckAfterQuant))
	{
		// Hackish solution to get rid of strict-aliasing warning
		union
		{
			float f;
			unsigned u;
		}StartPosX, StartPosY, StartVelX, StartVelY;

		StartPosX.f = StartPos.x;
		StartPosY.f = StartPos.y;
		StartVelX.f = StartVel.x;
		StartVelY.f = StartVel.y;

		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "STUCK!!! %d %d %d %f %f %f %f %x %x %x %x",
			StuckBefore,
			StuckAfterMove,
			StuckAfterQuant,
			StartPos.x, StartPos.y,
			StartVel.x, StartVel.y,
			StartPosX.u, StartPosY.u,
			StartVelX.u, StartVelY.u);
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
	}

	int Events = m_Core.m_TriggeredEvents;
	int Mask = CmaskAllExceptOne(m_pPlayer->GetCID());

	if(Events&COREEVENT_GROUND_JUMP) GameServer()->CreateSound(m_Pos, SOUND_PLAYER_JUMP, Mask);

	if(Events&COREEVENT_HOOK_ATTACH_PLAYER) GameServer()->CreateSound(m_Pos, SOUND_HOOK_ATTACH_PLAYER, CmaskAll());
	if(Events&COREEVENT_HOOK_ATTACH_GROUND) GameServer()->CreateSound(m_Pos, SOUND_HOOK_ATTACH_GROUND, Mask);
	if(Events&COREEVENT_HOOK_HIT_NOHOOK) GameServer()->CreateSound(m_Pos, SOUND_HOOK_NOATTACH, Mask);


	if(m_pPlayer->GetTeam() == TEAM_SPECTATORS)
	{
		m_Pos.x = m_Input.m_TargetX;
		m_Pos.y = m_Input.m_TargetY;
	}

	// update the m_SendCore if needed
	{
		CNetObj_Character Predicted;
		CNetObj_Character Current;
		mem_zero(&Predicted, sizeof(Predicted));
		mem_zero(&Current, sizeof(Current));
		m_ReckoningCore.Write(&Predicted);
		m_Core.Write(&Current);

		// only allow dead reackoning for a top of 3 seconds
		if(m_ReckoningTick+Server()->TickSpeed()*3 < Server()->Tick() || mem_comp(&Predicted, &Current, sizeof(CNetObj_Character)) != 0)
		{
			m_ReckoningTick = Server()->Tick();
			m_SendCore = m_Core;
			m_ReckoningCore = m_Core;
		}
	}
}

void CCharacter::TickPaused()
{
	++m_AttackTick;
	++m_DamageTakenTick;
	++m_Ninja.m_ActivationTick;
	++m_ReckoningTick;
	if(m_LastAction != -1)
		++m_LastAction;
	if(m_aWeapons[m_ActiveWeapon].m_AmmoRegenStart > -1)
		++m_aWeapons[m_ActiveWeapon].m_AmmoRegenStart;
	if(m_EmoteStop > -1)
		++m_EmoteStop;
}

bool CCharacter::IncreaseHealth(int Amount)
{
	if(m_Health >= 10)
		return false;
	m_Health = clamp(m_Health+Amount, 0, 10);
	return true;
}

bool CCharacter::IncreaseArmor(int Amount)
{
	if(m_Armor >= 10)
		return false;
	m_Armor = clamp(m_Armor+Amount, 0, 10);
	return true;
}

void CCharacter::Die(int Killer, int Weapon, bool DropScrap)
{
	VehicleOnCharacterDie(GameServer(), GetPlayer()->GetCID());

	// a nice sound
	GameServer()->CreateSound(m_Pos, SOUND_PLAYER_DIE);

	GameServer()->CreateDeath(m_Pos, m_pPlayer->GetCID());
	int ScrapCount = m_pPlayer->m_vScraps.size();
	if(DropScrap && ScrapCount > 0)
		GameServer()->SendChatTarget(-1, _("{str:name} 倒下，掉落了 {int:count} 件废品"), "name", Server()->ClientName(m_pPlayer->GetCID()), "count", &ScrapCount);
	if(DropScrap)
	{
		int Lost = m_pPlayer->GetBackpackValue();
		LcRecordDeathLoss(GameServer(), m_pPlayer->GetCID(), Lost);
		m_pPlayer->DropAllScrap(m_Pos, m_InShip);
	}
	LcRecordDeath(GameServer(), m_pPlayer->GetCID());
	LCDie();
}

void CCharacter::HandleHazards()
{
	if(Server()->m_LocateGame != LOCATE_GAME || m_InShip || m_Freeze)
		return;

	CCollision *pCol = GameServer()->Collision();
	int Hazard = pCol->GetHazardAtCharacter(m_Pos);
	if(Hazard == 0 && (pCol->GetCollisionAt(m_Pos.x, m_Pos.y) & CCollision::COLFLAG_DEATH))
		Hazard = LC_HAZARD_MINE;

	if(Hazard != m_LastHazardType)
	{
		if(Hazard == LC_HAZARD_GAS)
			GameServer()->SendBroadcast(GetPlayer()->GetCID(), BROADCAST_PRIORITY_EFFECTSTATE, Server()->TickSpeed() * 2, _("【警告】你进入了毒气区域！"));
		else if(Hazard == LC_HAZARD_SHOCK)
			GameServer()->SendBroadcast(GetPlayer()->GetCID(), BROADCAST_PRIORITY_EFFECTSTATE, Server()->TickSpeed() * 2, _("【警告】你进入了漏电区域！"));
		else if(Hazard == LC_HAZARD_TAR)
			GameServer()->SendBroadcast(GetPlayer()->GetCID(), BROADCAST_PRIORITY_EFFECTSTATE, Server()->TickSpeed() * 2, _("【警告】你踏入了黏性焦油！"));
		else if(Hazard == LC_HAZARD_MINE)
			GameServer()->SendBroadcast(GetPlayer()->GetCID(), BROADCAST_PRIORITY_EFFECTSTATE, Server()->TickSpeed() * 2, _("【警告】你进入了地雷区域！"));
		m_LastHazardType = Hazard;
	}
	if(Hazard == LC_HAZARD_NONE)
		m_LastHazardType = LC_HAZARD_NONE;

	if(Hazard == LC_HAZARD_GAS)
	{
		if(Server()->Tick() % (GC_HAZARD_GAS_TICK_SEC * Server()->TickSpeed()) == 0)
		{
			TakeDamage(vec2(0, 0.3), GC_HAZARD_GAS_DAMAGE, GetPlayer()->GetCID(), WEAPON_NINJA);
			if(Server()->Tick() - m_LastHazardWarnTick > Server()->TickSpeed() * 4)
			{
				m_LastHazardWarnTick = Server()->Tick();
				GameServer()->SendBroadcast(GetPlayer()->GetCID(), BROADCAST_PRIORITY_EFFECTSTATE, BROADCAST_DURATION_GAMEANNOUNCE, _("【毒气】吸入有害气体！"));
			}
		}
	}
	else if(Hazard == LC_HAZARD_SHOCK)
	{
		if(Server()->Tick() % (GC_HAZARD_SHOCK_TICK_SEC * Server()->TickSpeed()) == 0)
		{
			TakeDamage(vec2(0, 0.2), GC_HAZARD_SHOCK_DAMAGE, GetPlayer()->GetCID(), WEAPON_HAMMER);
			GameServer()->CreateSound(m_Pos, SOUND_PLAYER_PAIN_SHORT);
			if(Server()->Tick() - m_LastHazardWarnTick > Server()->TickSpeed() * 3)
			{
				m_LastHazardWarnTick = Server()->Tick();
				GameServer()->SendBroadcast(GetPlayer()->GetCID(), BROADCAST_PRIORITY_EFFECTSTATE, BROADCAST_DURATION_GAMEANNOUNCE, _("【漏电】触碰到带电地面！"));
			}
		}
	}
	else if(Hazard == LC_HAZARD_SPIKE)
	{
		int Gx = round_to_int(m_Pos.x) / 32;
		int Gy = round_to_int(m_Pos.y) / 32;
		if(Gx != m_LastSpikeGridX || Gy != m_LastSpikeGridY)
		{
			m_LastSpikeGridX = Gx;
			m_LastSpikeGridY = Gy;
			TakeDamage(vec2(0, 0.4), GC_HAZARD_SPIKE_DAMAGE, GetPlayer()->GetCID(), WEAPON_HAMMER);
			GameServer()->CreateHammerHit(m_Pos);
			GameServer()->SendBroadcast(GetPlayer()->GetCID(), BROADCAST_PRIORITY_EFFECTSTATE, Server()->TickSpeed() * 2, _("你踩中了尖刺陷阱！"));
		}
	}
	else if(Hazard == LC_HAZARD_TAR)
	{
		// movement penalty handled in UpdateTuningParam
	}
	else if(Hazard == LC_HAZARD_MINE)
	{
		TakeDamage(vec2(0, 0.5), GC_HAZARD_MINE_DAMAGE, GetPlayer()->GetCID(), WEAPON_GRENADE);
		pCol->ClearHazardAt(m_Pos);
		GameServer()->SendBroadcast(GetPlayer()->GetCID(), BROADCAST_PRIORITY_EFFECTSTATE, Server()->TickSpeed() * 2, _("你触发了地雷！"));
		GameServer()->CreateSound(m_Pos, SOUND_GRENADE_EXPLODE);
		GameServer()->CreateHammerHit(m_Pos);
	}
	else
	{
		m_LastSpikeGridX = -100000;
		m_LastSpikeGridY = -100000;
	}
}

static const char *LcCompassDir(vec2 From, vec2 To)
{
	vec2 D = To - From;
	if(length(D) < 1.0f)
		return _("近旁");
	if(fabs(D.x) > fabs(D.y))
		return D.x > 0 ? _("东方") : _("西方");
	return D.y > 0 ? _("南方") : _("北方");
}

void CCharacter::HandleCompass()
{
	if(Server()->m_LocateGame != LOCATE_GAME || m_InShip || m_Freeze)
		return;
	if(!GameServer()->m_pController || !GameServer()->m_pController->m_pShip)
		return;

	vec2 Ship = GameServer()->m_pController->m_pShip->m_Pos;
	float Dist = distance(m_Pos, Ship);
	if(Dist < GC_SHIP_HINT_DIST)
		return;
	if(Server()->Tick() - m_LastCompassTick < Server()->TickSpeed() * GC_SHIP_COMPASS_SEC)
		return;

	m_LastCompassTick = Server()->Tick();
	int Tiles = (int)(Dist / 32.0f);
	if(g_Config.m_SvTimelimit > 0)
	{
		int LimitTicks = g_Config.m_SvTimelimit * Server()->TickSpeed() * 60 + GameServer()->m_pController->ExpeditionTimeBonusSec() * Server()->TickSpeed();
		int RemainingTicks = LimitTicks - (Server()->Tick() - GameServer()->m_pController->RoundStartTick());
		int RemainingSec = RemainingTicks > 0 ? RemainingTicks / Server()->TickSpeed() : 0;
		const char *pAdvice = RemainingSec > 120 ? _("还可继续搜索") : _("建议现在返回飞船");
		GameServer()->SendBroadcast(GetPlayer()->GetCID(), BROADCAST_PRIORITY_INTERFACE, Server()->TickSpeed() * 2, _("【导航】着陆飞船在你{lstr:dir}边（约 {int:m} 格）| 班次剩余 {int:sec} 秒 | {lstr:advice}"), "dir", LcCompassDir(m_Pos, Ship), "m", &Tiles, "sec", &RemainingSec, "advice", pAdvice);
	}
	else
		GameServer()->SendBroadcast(GetPlayer()->GetCID(), BROADCAST_PRIORITY_INTERFACE, Server()->TickSpeed() * 2, _("【导航】着陆飞船在你{lstr:dir}边（约 {int:m} 格）"), "dir", LcCompassDir(m_Pos, Ship), "m", &Tiles);
}

bool CCharacter::TakeDamage(vec2 Force, int Dmg, int From, int Weapon)
{
	m_Core.m_Vel += Force;

	if(GameServer()->m_pController->IsFriendlyFire(m_pPlayer->GetCID(), From))
		return false;

	// m_pPlayer only inflicts half damage on self
	if(From == m_pPlayer->GetCID())
		Dmg = max(1, Dmg/2);

	m_DamageTaken++;

	// create healthmod indicator
	if(Server()->Tick() < m_DamageTakenTick+25)
	{
		// make sure that the damage indicators doesn't group together
		GameServer()->CreateDamageInd(m_Pos, m_DamageTaken*0.25f, Dmg);
	}
	else
	{
		m_DamageTaken = 0;
		GameServer()->CreateDamageInd(m_Pos, 0, Dmg);
	}

	if(Dmg)
	{
		if(m_Armor)
		{
			if(Dmg > 1)
			{
				m_Health--;
				Dmg--;
			}

			if(Dmg > m_Armor)
			{
				Dmg -= m_Armor;
				m_Armor = 0;
			}
			else
			{
				m_Armor -= Dmg;
				Dmg = 0;
			}
		}

		m_Health -= Dmg;
	}

	m_DamageTakenTick = Server()->Tick();

	// do damage Hit sound
	if(From >= 0 && From != m_pPlayer->GetCID() && GameServer()->m_apPlayers[From])
	{
		int Mask = CmaskOne(From);
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() == TEAM_SPECTATORS && GameServer()->m_apPlayers[i]->m_SpectatorID == From)
				Mask |= CmaskOne(i);
		}
		GameServer()->CreateSound(GameServer()->m_apPlayers[From]->m_ViewPos, SOUND_HIT, Mask);
	}

	// check for death
	if(m_Health <= 0)
	{
		Die(From, Weapon);

		// set attacker's face to happy (taunt!)
		if (From >= 0 && From != m_pPlayer->GetCID() && GameServer()->m_apPlayers[From])
		{
			CCharacter *pChr = GameServer()->m_apPlayers[From]->GetCharacter();
			if (pChr)
			{
				pChr->m_EmoteType = EMOTE_HAPPY;
				pChr->m_EmoteStop = Server()->Tick() + Server()->TickSpeed();
			}
		}

		return false;
	}

	if (Dmg > 2)
		GameServer()->CreateSound(m_Pos, SOUND_PLAYER_PAIN_LONG);
	else
		GameServer()->CreateSound(m_Pos, SOUND_PLAYER_PAIN_SHORT);

	m_EmoteType = EMOTE_PAIN;
	m_EmoteStop = Server()->Tick() + 500 * Server()->TickSpeed() / 1000;

	return true;
}

void CCharacter::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;

	CNetObj_Character *pCharacter = static_cast<CNetObj_Character *>(Server()->SnapNewItem(NETOBJTYPE_CHARACTER, m_pPlayer->GetCID(), sizeof(CNetObj_Character)));
	if(!pCharacter)
		return;

	// write down the m_Core
	if(m_OnVehicle)
	{
		VehicleResetCharacterHook(this);
		pCharacter->m_Tick = 0;
		m_Core.Write(pCharacter);
		pCharacter->m_HookState = HOOK_IDLE;
		pCharacter->m_HookX = pCharacter->m_X;
		pCharacter->m_HookY = pCharacter->m_Y;
	}
	else if(!m_ReckoningTick || GameServer()->m_World.m_Paused)
	{
		// no dead reckoning when paused because the client doesn't know
		// how far to perform the reckoning
		pCharacter->m_Tick = 0;
		m_Core.Write(pCharacter);
	}
	else
	{
		pCharacter->m_Tick = m_ReckoningTick;
		m_SendCore.Write(pCharacter);
	}

	// set emote
	if (m_EmoteStop < Server()->Tick())
	{
		m_EmoteType = EMOTE_NORMAL;
		m_EmoteStop = -1;
	}

	pCharacter->m_Emote = m_EmoteType;

	pCharacter->m_AmmoCount = 0;
	pCharacter->m_Health = 0;
	pCharacter->m_Armor = 0;

	pCharacter->m_Weapon = m_ActiveWeapon;
	pCharacter->m_AttackTick = m_AttackTick;

	pCharacter->m_Direction = m_Input.m_Direction;

	if(m_pPlayer->GetCID() == SnappingClient || SnappingClient == -1 ||
		(!g_Config.m_SvStrictSpectateMode && m_pPlayer->GetCID() == GameServer()->m_apPlayers[SnappingClient]->m_SpectatorID))
	{
		pCharacter->m_Health = m_Health;
		pCharacter->m_Armor = m_Armor;
		if(m_aWeapons[m_ActiveWeapon].m_Ammo > 0)
			pCharacter->m_AmmoCount = m_aWeapons[m_ActiveWeapon].m_Ammo;
	}

	if(pCharacter->m_Emote == EMOTE_NORMAL)
	{
		if(250 - ((Server()->Tick() - m_LastAction)%(250)) < 5)
			pCharacter->m_Emote = EMOTE_BLINK;
	}

	pCharacter->m_PlayerFlags = GetPlayer()->m_PlayerFlags;
}

void CCharacter::PickupScrap()
{
	if(!IsAlive() || m_ReloadTimer)
		return;
	for(auto *pDrop = (CScrap*) GameWorld()->FindFirst(CGameWorld::ENTTYPE_SCRAP); pDrop; pDrop = (CScrap*) pDrop->TypeNext())
	{
        if (pDrop)
		{
            if (distance(pDrop->m_Pos, m_Pos) < (pDrop->GetWeight()*2)+8)
			{
                if(!pDrop->m_Hide)
				{
                	if (pDrop->Pickup(GetPlayer()->GetCID()))
					{
						GameServer()->CreateHammerHit(pDrop->m_Pos);
						int Value = pDrop->GetScrapValue();
						int Weight = pDrop->GetWeight();
						int TotalWeight = GetPlayer()->GetBackpackWeight() + Weight;
						GameServer()->SendChatTarget(GetPlayer()->GetCID(), _("你捡起了{lstr:iname}，价值{int:value}元，重量{int:weight}镑（背包总重{int:total}镑）"), "iname", GameServer()->ScrapInfo()->GetScrapName(pDrop->GetScrapType()), "value", &Value, "weight", &Weight, "total", &TotalWeight);
						pDrop->Reset();
                	    return;
                	}
				}
            }
        }
    }
}

void CCharacter::UpdateTuningParam()
{
	CTuningParams pTuningParams = m_pPlayer->m_NextTuningParams;
	if(m_pPlayer->m_Weight)
	{
		int Weight = m_pPlayer->m_Weight;
		float Factor = 1.0f - ((float)Weight / 200);
		m_pPlayer->m_NextTuningParams.m_GroundControlSpeed = pTuningParams.m_GroundControlSpeed * Factor;
		m_pPlayer->m_NextTuningParams.m_GroundJumpImpulse = pTuningParams.m_GroundJumpImpulse * Factor;
		m_pPlayer->m_NextTuningParams.m_AirJumpImpulse = pTuningParams.m_AirJumpImpulse * Factor;
		m_pPlayer->m_NextTuningParams.m_AirControlSpeed = pTuningParams.m_AirControlSpeed * Factor;
		m_pPlayer->m_NextTuningParams.m_Gravity = (0.5f + (Weight/400));
	}
	if(m_HookMode == 1)
	{
		m_pPlayer->m_NextTuningParams.m_HookDragSpeed = 0.0f;
		m_pPlayer->m_NextTuningParams.m_HookDragAccel = 1.0f;
	}
	if(m_Freeze)
	{
		m_pPlayer->m_NextTuningParams.m_GroundControlAccel = 0.0f;
		m_pPlayer->m_NextTuningParams.m_GroundJumpImpulse = 0.0f;
		m_pPlayer->m_NextTuningParams.m_AirJumpImpulse = 0.0f;
		m_pPlayer->m_NextTuningParams.m_AirControlAccel = 0.0f;
		m_pPlayer->m_NextTuningParams.m_HookLength = 0.0f;
	}
	if(m_OnVehicle)
	{
		m_pPlayer->m_NextTuningParams.m_HookLength = 0.0f;
		m_pPlayer->m_NextTuningParams.m_HookFireSpeed = 0.0f;
	}
	if(m_SpeedBoostUntilTick > Server()->Tick())
	{
		m_pPlayer->m_NextTuningParams.m_GroundControlSpeed = m_pPlayer->m_NextTuningParams.m_GroundControlSpeed * 1.35f;
		m_pPlayer->m_NextTuningParams.m_AirControlSpeed = m_pPlayer->m_NextTuningParams.m_AirControlSpeed * 1.35f;
	}
	if(Server()->m_LocateGame == LOCATE_GAME && !m_InShip && !m_Freeze)
	{
		if(GameServer()->Collision()->GetHazardAtCharacter(m_Pos) == LC_HAZARD_TAR)
		{
			m_pPlayer->m_NextTuningParams.m_GroundControlSpeed = m_pPlayer->m_NextTuningParams.m_GroundControlSpeed * 0.55f;
			m_pPlayer->m_NextTuningParams.m_AirControlSpeed = m_pPlayer->m_NextTuningParams.m_AirControlSpeed * 0.55f;
		}
	}
}

void CCharacter::LCDie()
{
	m_Freeze = true;
	GameServer()->SendBroadcast(GetPlayer()->GetCID(), BROADCAST_PRIORITY_EFFECTSTATE, Server()->TickSpeed() * 2, _("[生命维持系统]警告! 生命维持系统已损坏!"));
}

bool CCharacter::TryReviveBy(int FromClient)
{
	if(!m_Freeze)
		return false;

	m_Freeze = false;
	m_Health = 1;
	IncreaseHealth(4);
	Server()->GetClientSession(GetPlayer()->GetCID())->m_Freeze = false;
	LcRecordRevive(GameServer(), FromClient, GetPlayer()->GetCID());
	GameServer()->SendChatTarget(-1, _("{str:helper} 救活了 {str:name}"), "helper", Server()->ClientName(FromClient), "name", Server()->ClientName(GetPlayer()->GetCID()));
	return true;
}