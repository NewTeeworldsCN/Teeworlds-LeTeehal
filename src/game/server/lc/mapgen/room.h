#ifndef GAME_SERVER_MAPGEN_ROOM_H
#define GAME_SERVER_MAPGEN_ROOM_H

#include <game/server/lc/hazards/hazards.h>

class CRoom
{
private:
	CRoom *m_pChild1, *m_pChild2;
	int m_X, m_Y, m_W, m_H;
	int m_FacilityType;
	
	bool m_Open;
	
public:
	CRoom(int x, int y, int w, int h, int FacilityType = LC_FACILITY_FACTORY);
	~CRoom();
	
	bool TooSmall()
	{
		if(m_FacilityType == LC_FACILITY_MINES)
		{
			if(m_W < 5 || m_H < 5)
				return true;
			return false;
		}
		if(m_W < 6 || m_H < 6)
			return true;

		return false;
	}
	
	bool Open(int x, int y);
	
	void Split(bool Vertical);
	
	void Generate(class CGenLayer *pTiles);
	
	void Fill(class CGenLayer *pTiles, int Index, int x, int y, int w, int h);

	int GetMaxDepth() const;
	vec2 GetFurthestPoint() const;
};


#endif
