#ifndef GAME_SERVER_MAPGEN_MAZE_H
#define GAME_SERVER_MAPGEN_MAZE_H

#include <game/server/lc/hazards/hazards.h>

class CMaze
{
private:
	int m_W, m_H;
	int m_FacilityType;
	
	vec2 m_aRoom[999];
	int m_Rooms;
	
	bool *m_aOpen;
	bool *m_aConnected;
	
	void GenerateOrganic();
	void GenerateGrid();
	void GenerateRoom(bool AutoConnect = false, bool MirrorMode = false);
	void ConnectRandomRooms();
	void ConnectRooms();
	void ConnectEverything();
	void SetConnections(ivec2 Pos);
	ivec2 GetClosestConnected(ivec2 Pos);
	vec2 GetClosestRoom(vec2 Pos);
	ivec2 GetUnconnected();
	
	void GenerateLinear(int Width, int Rooms = 0);
	
	void Open(vec2 Pos, int Size = 1);
	void Open(int x, int y);
	
	void Connect(vec2 Pos0, vec2 Pos1);

	void EnsureAccessibility();
	vec2 GetFurthestPoint(vec2 Origin) const;
	
public:
	CMaze(int w, int h, int FacilityType = LC_FACILITY_FACTORY);
	~CMaze();
	
	void OpenRect(int x, int y, int w, int h);
	void ConnectToFacility(vec2 From, vec2 To);
	void Carve(class CGenLayer *pTiles) const;

	void OpenRooms(class CRoom *pRoom, int OffX = 0, int OffY = 0);
};


#endif
