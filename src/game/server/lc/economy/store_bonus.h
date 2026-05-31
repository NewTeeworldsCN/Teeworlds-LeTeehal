#ifndef GAME_SERVER_LC_STORE_BONUS_H
#define GAME_SERVER_LC_STORE_BONUS_H

struct CLcStoreBonus
{
	int m_ShotgunAmmo;
	int m_RifleAmmo;
	int m_GrenadeAmmo;
	int m_GunAmmo;
	int m_NinjaAmmo;
	int m_HealthBonus;
	int m_Medkit;
	int m_Whistle;
	int m_Soda;
	int m_Megaphone;
	int m_Boombox;
	int m_Remote;
	int m_Aircraft;

	void Reset()
	{
		m_ShotgunAmmo = 0;
		m_RifleAmmo = 0;
		m_GrenadeAmmo = 0;
		m_GunAmmo = 0;
		m_NinjaAmmo = 0;
		m_HealthBonus = 0;
		m_Medkit = 0;
		m_Whistle = 0;
		m_Soda = 0;
		m_Megaphone = 0;
		m_Boombox = 0;
		m_Remote = 0;
		m_Aircraft = 0;
	}

	bool HasAny() const
	{
		return m_ShotgunAmmo || m_RifleAmmo || m_GrenadeAmmo || m_GunAmmo || m_NinjaAmmo != 0 ||
			m_HealthBonus || m_Medkit || m_Whistle || m_Soda || m_Megaphone || m_Boombox || m_Remote || m_Aircraft;
	}
};

#endif
