#include <base/system.h>
#include <base/math.h>
#include <base/vmath.h>

#include <game/server/lc/expedition/moons.h>

#include <game/server/lc/mapgen/mapgen_random.h>

#include "room.h"
#include "gen_layer.h"

static const int LC_ROOM_TILES = 6;

static int RoomGridSize(int FacilityType)
{
	(void)FacilityType;
	return LC_ROOM_TILES;
}

static int RoomMinLeaf(int FacilityType)
{
	if(FacilityType == LC_FACILITY_MINES)
		return 5;
	return LC_ROOM_TILES;
}

static int RoomTargetSize(int FacilityType)
{
	if(FacilityType == LC_FACILITY_MINES)
		return 5;
	return LC_ROOM_TILES;
}

static int GridAlign(int Value, int Grid)
{
	if(Grid < 2)
		return Value;
	return ((Value + Grid / 2) / Grid) * Grid;
}

static int GridSplit(int Size, int Grid, int MinLeaf)
{
	if(Size < MinLeaf * 2)
		return 0;

	int Half = Size / 2;
	Half = GridAlign(Half, Grid);
	if(Half < MinLeaf)
		Half = MinLeaf;
	if(Half > Size - MinLeaf)
		Half = Size - MinLeaf;
	if(Half <= 0 || Half >= Size)
		return 0;
	return Half;
}

static bool CanSplit(int Size, int FacilityType)
{
	return Size >= RoomMinLeaf(FacilityType) * 2;
}

// bsp map, acts as template for rooms
CRoom::CRoom(int x, int y, int w, int h, int FacilityType)
{
	m_Open = false;
	m_FacilityType = FacilityType;
	
	m_X = x;
	m_Y = y;
	m_W = w;
	m_H = h;
	
	m_pChild1 = NULL;
	m_pChild2 = NULL;
	
	int RoomSize = RoomTargetSize(FacilityType);
	
	if (m_H < m_W)
	{
		if(CanSplit(m_W, FacilityType) && m_W > RoomSize)
			Split(false);
		if(CanSplit(m_H, FacilityType) && m_H > RoomSize)
			Split(true);
	}
	else
	{
		if(CanSplit(m_H, FacilityType) && m_H > RoomSize)
			Split(true);
		if(CanSplit(m_W, FacilityType) && m_W > RoomSize)
			Split(false);
	}
}

CRoom::~CRoom()
{
	if (m_pChild1)
		delete m_pChild1;
	if (m_pChild2)
		delete m_pChild2;
}

void CRoom::Split(bool Vertical)
{
	if (TooSmall())
		return;
	if(m_pChild1 || m_pChild2)
		return;

	int Grid = RoomGridSize(m_FacilityType);
	int MinLeaf = RoomMinLeaf(m_FacilityType);
		
	if (Vertical)
	{
		int h2 = m_H;

		if(LcFacilityUsesGridLayout((ELcFacilityType)m_FacilityType))
			m_H = GridSplit(m_H, Grid, MinLeaf);
		else if (m_H < 4)
			return;
		else if (m_W < 32)
			m_H = 3 + MapGenRand()%(m_H-2);
		else
			m_H = m_H/(2 + MapGenRand()%2);

		if(m_H <= MinLeaf || m_H >= h2 - MinLeaf)
			return;

		m_pChild1 = new CRoom(m_X, m_Y, m_W, m_H, m_FacilityType);
		m_pChild2 = new CRoom(m_X, m_Y+m_H, m_W, h2-m_H, m_FacilityType);
		if(!m_pChild1 || !m_pChild2)
		{
			delete m_pChild1;
			delete m_pChild2;
			m_pChild1 = 0;
			m_pChild2 = 0;
		}
	}
	else
	{
		int w2 = m_W;

		if(LcFacilityUsesGridLayout((ELcFacilityType)m_FacilityType))
			m_W = GridSplit(m_W, Grid, MinLeaf);
		else if (m_W < 4)
			return;
		else if (m_H < 32)
			m_W = 3 + MapGenRand()%(m_W-2);
		else
			m_W = m_W/(2 + MapGenRand()%2);

		if(m_W <= MinLeaf || m_W >= w2 - MinLeaf)
			return;

		m_pChild1 = new CRoom(m_X, m_Y, m_W, m_H, m_FacilityType);
		m_pChild2 = new CRoom(m_X+m_W, m_Y, w2-m_W, m_H, m_FacilityType);
		if(!m_pChild1 || !m_pChild2)
		{
			delete m_pChild1;
			delete m_pChild2;
			m_pChild1 = 0;
			m_pChild2 = 0;
		}
	}
}


bool CRoom::Open(int x, int y)
{
	bool c1 = false;
	bool c2 = false;
	
	if (m_pChild1)
		c1 = m_pChild1->Open(x, y);
	
	if (m_pChild2)
		c2 = m_pChild2->Open(x, y);
	
	if (m_X <= x && m_X+m_W >= x &&
		m_Y <= y && m_Y+m_H >= y)
	{
		m_Open = true;
		return (!TooSmall() || c1 || c2);
	}
	
	return false;
}

void CRoom::Generate(CGenLayer *pTiles)
{
	if (!m_pChild1 && m_Open)
	{
		int x = m_X;
		int y = m_Y;
		int w = m_W;
		int h = m_H;

		if(LcFacilityUsesGridLayout((ELcFacilityType)m_FacilityType))
		{
			int Grid = RoomGridSize(m_FacilityType);
			x = GridAlign(m_X, Grid);
			y = GridAlign(m_Y, Grid);
			w = GridAlign(m_X + m_W, Grid) - x;
			h = GridAlign(m_Y + m_H, Grid) - y;
			if(w > LC_ROOM_TILES)
				w = LC_ROOM_TILES;
			if(h > LC_ROOM_TILES)
				h = LC_ROOM_TILES;
			if(w < Grid)
				w = Grid;
			if(h < Grid)
				h = Grid;
		}

		Fill(pTiles, 0, x, y, w, h);
	}
	
	if (m_pChild1)
		m_pChild1->Generate(pTiles);
	
	if (m_pChild2)
		m_pChild2->Generate(pTiles);
}


void CRoom::Fill(CGenLayer *pTiles, int Index, int x, int y, int w, int h)
{
	for(int py = y; py < y+h; py++)
		for(int px = x; px < x+w; px++)
		{
			pTiles->Set(Index, px, py);
		}
}

int CRoom::GetMaxDepth() const
{
    int CurrentDepth = m_Y + m_H;
    int ChildDepth = 0;
    
    if (m_pChild1)
		ChildDepth = m_pChild1->GetMaxDepth();
    if (m_pChild2)
		ChildDepth = std::max(ChildDepth, m_pChild2->GetMaxDepth());
    
    return std::max(CurrentDepth, ChildDepth);
}

vec2 CRoom::GetFurthestPoint() const
{
    vec2 current_furthest = vec2(m_X + m_W, m_Y + m_H);

    vec2 child_furthest = current_furthest;
    if (m_pChild1)
    {
        vec2 child1_furthest = m_pChild1->GetFurthestPoint();
        if (distance(child1_furthest, vec2(0, 0)) > distance(child_furthest, vec2(0, 0)))
            child_furthest = child1_furthest;
    }
    if (m_pChild2)
    {
        vec2 child2_furthest = m_pChild2->GetFurthestPoint();
        if (distance(child2_furthest, vec2(0, 0)) > distance(child_furthest, vec2(0, 0)))
            child_furthest = child2_furthest;
    }

    return distance(current_furthest, vec2(0, 0)) > distance(child_furthest, vec2(0, 0)) 
           ? current_furthest 
           : child_furthest;
}
