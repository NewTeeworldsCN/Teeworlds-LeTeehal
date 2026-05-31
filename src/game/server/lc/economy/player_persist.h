#ifndef GAME_SERVER_LC_PLAYER_PERSIST_H
#define GAME_SERVER_LC_PLAYER_PERSIST_H

#include <engine/shared/memheap.h>
#include "../../scrap/scrap_info.h"

struct CLcPlayerPersist
{
	bool m_Active;
	int m_RoundId;
	float m_X;
	float m_Y;
	bool m_Freeze;
	int m_Hand;
	int m_LeekTick;
	int m_DisconnectTick;
	array<Scrap> m_vScraps;

	CLcPlayerPersist()
	{
		Reset();
	}

	void Reset()
	{
		m_Active = false;
		m_RoundId = 0;
		m_X = 0;
		m_Y = 0;
		m_Freeze = false;
		m_Hand = 0;
		m_LeekTick = 0;
		m_DisconnectTick = 0;
		m_vScraps.clear();
	}
};

#endif
