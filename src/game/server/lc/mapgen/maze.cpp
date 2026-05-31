#include <base/system.h>
#include <base/math.h>
#include <base/vmath.h>

#include <engine/shared/config.h>

#include <game/server/lc/expedition/moons.h>
#include <game/server/lc/mapgen/mapgen_theme.h>

#include <game/server/lc/mapgen/mapgen_random.h>

#include <queue>

#include "room.h"
#include "maze.h"
#include "gen_layer.h"

CMaze::CMaze(int w, int h, int FacilityType)
{
	m_W = w;
	m_H = h;
	m_FacilityType = FacilityType;

	m_aOpen = new bool[w * h];
	m_aConnected = new bool[w * h];

	for (int i = 0; i < w * h; i++)
	{
		m_aOpen[i] = false;
		m_aConnected[i] = false;
	}

	if(LcFacilityUsesGridLayout((ELcFacilityType)m_FacilityType))
		GenerateGrid();
	else
		GenerateOrganic();
}

CMaze::~CMaze()
{
	if (m_aOpen)
		delete m_aOpen;

	if (m_aConnected)
		delete m_aConnected;
}

void CMaze::GenerateGrid()
{
	m_Rooms = 0;

	const int RoomW = 6;
	const int RoomH = 6;
	int GridStep = max(8, min(m_W, m_H) / 10);
	GridStep = (GridStep / 2) * 2;
	if(GridStep < 8)
		GridStep = 8;

	int SpineX = ((m_W / 2) / 2) * 2;
	int SpineY = ((m_H / 2) / 2) * 2;

	Connect(vec2(2.f, (float)SpineY), vec2((float)(m_W - 2), (float)SpineY));
	Connect(vec2((float)SpineX, 2.f), vec2((float)SpineX, (float)(m_H - 2)));

	for(int gx = GridStep; gx < m_W - GridStep / 2 && m_Rooms < 900; gx += GridStep)
	{
		for(int gy = GridStep; gy < m_H - GridStep / 2 && m_Rooms < 900; gy += GridStep)
		{
			m_aRoom[m_Rooms++] = vec2((float)gx, (float)gy);
			OpenRect(gx - RoomW / 2, gy - RoomH / 2, RoomW, RoomH);
		}
	}

	for(int r = 0; r < m_Rooms; r++)
	{
		vec2 p = m_aRoom[r];
		Connect(p, vec2(p.x, (float)SpineY));
		Connect(p, vec2((float)SpineX, p.y));
	}

	ConnectEverything();
	EnsureAccessibility();
}

void CMaze::GenerateOrganic()
{
	m_Rooms = 0;

	int r = m_W * m_H / 220;
	if(r < 12)
		r = 12;
	if(r > 40)
		r = 40;
	if(m_FacilityType == LC_FACILITY_MINES)
	{
		r = m_W * m_H / 260;
		if(r < 10)
			r = 10;
		if(r > 32)
			r = 32;
	}

	const SLcMapgenThemeProfile *pTheme = LcGetMapgenThemeProfile(g_Config.m_SvMapgenTheme);
	if(pTheme && pTheme->m_OrganicRoomMul != 100)
	{
		r = r * pTheme->m_OrganicRoomMul / 100;
		if(r < 8)
			r = 8;
	}

	float s = 0.14f;

	Connect(vec2(m_W * (0.3f - s), m_H * (0.5f + s * 0.4f)), vec2(m_W * (0.5f + s), m_H * (0.5f + s * 0.4f)));
	Connect(vec2(m_W * (0.5f + s), m_H * (0.5f + s * 0.4f)), vec2(m_W * (0.5f + s), m_H * 0.5f));

	Connect(vec2(m_W * (0.35f - s), m_H * 0.5f), vec2(m_W * 0.4f, m_H * 0.5f));
	Connect(vec2(m_W * 0.6f, m_H * 0.5f), vec2(m_W * (0.65f + s), m_H * 0.5f));

	Connect(vec2(m_W * (0.5f - s), m_H * (0.5f - s * 0.4f)), vec2(m_W * (0.5f + s), m_H * (0.5f - s * 0.4f)));
	Connect(vec2(m_W * 0.5f, m_H * (0.5f - s * 0.4f)), vec2(m_W * 0.5f, m_H * (0.5f - s * 0.8f)));
	Connect(vec2(m_W * 0.5f, m_H * (0.5f - s * 0.8f)), vec2(m_W * (0.6f + s), m_H * (0.5f - s * 0.8f)));

	Connect(vec2(m_W * (0.5f - s), m_H * (0.5f + s * 0.8f)), vec2(m_W * (0.5f + s), m_H * (0.5f + s * 0.8f)));
	Connect(vec2(m_W * (0.5f - s), m_H * (0.5f + s * 1.2f)), vec2(m_W * (0.5f + s), m_H * (0.5f + s * 1.2f)));

	Connect(vec2(m_W * 0.42f, m_H * (0.5f - s * 1.2f)), vec2(m_W * 0.58f, m_H * (0.5f - s * 1.2f)));

	for (int i = 0; i < r; i++)
		GenerateRoom();

	ConnectRooms();
	ConnectEverything();
	EnsureAccessibility();
}

void CMaze::GenerateLinear(int Width, int Rooms)
{
	float y = 0.3f + MapGenRandomFloat() * 0.4f;
	Connect(vec2(m_W * 0.5f - Width, m_H * y), vec2(m_W * 0.5f + Width, m_H * y));

	if (Rooms > 0)
	{
		m_aRoom[m_Rooms++] = vec2(m_W * 0.5f - Width * MapGenRandomFloat(), m_H * y);
		m_aRoom[m_Rooms++] = vec2(m_W * 0.5f + Width * MapGenRandomFloat(), m_H * y);

		for (int i = 0; i < Rooms; i++)
			GenerateRoom();
	}

	ConnectEverything();
}

void CMaze::GenerateRoom(bool AutoConnect, bool MirrorMode)
{
	// find a free spot to the room

	bool Valid = false;
	int i = 0;

	while (!Valid && i++ < 2000)
	{
		Valid = true;
		vec2 p = vec2(2 + MapGenRandomFloat() * (m_W - 4), 2 + MapGenRandomFloat() * (m_H - 4));

		if (MirrorMode)
			p = vec2(2 + MapGenRandomFloat() * (m_W * 0.5f), 2 + MapGenRandomFloat() * (m_H - 4));

		if (m_Rooms > 0)
		{
			vec2 rp = vec2(-1, -1);

			float d = -1.0f;
			for (int r = 0; r < m_Rooms; r++)
				if (d < 0.0f || distance(vec2(p.x, p.y * 2.25f), vec2(m_aRoom[r].x, m_aRoom[r].y * 2.25f)) < d)
				{
					d = distance(vec2(p.x, p.y * 2.25f), vec2(m_aRoom[r].x, m_aRoom[r].y * 2.25f));
					rp = m_aRoom[r];
				}

			if (fabs(p.x - rp.x) > 6 && fabs(p.y - rp.y) > 6)
				Valid = false;

			float MinDist = max(6.0f, m_W * 0.08f);
			float MaxDist = max(18.0f, m_W * 0.35f);
			if (d < MinDist || d > MaxDist)
				Valid = false;
		}

		if (Valid)
		{
			if (AutoConnect)
				Connect(p, GetClosestRoom(p));

			m_aRoom[m_Rooms] = p;
			Open(m_aRoom[m_Rooms], 1 + MapGenRand() % 4);

			//	Connect(p, m_aRoom[MapGenRand()%m_Rooms]);

			m_Rooms++;
			return;
		}
	}
}

void CMaze::ConnectRandomRooms()
{
	if (m_Rooms < 2)
		return;

	int r0 = MapGenRand() % (m_Rooms - 1);
	int r1 = MapGenRand() % (m_Rooms - 1);

	if (r0 != r1)
		Connect(m_aRoom[r0], m_aRoom[r1]);
}

void CMaze::ConnectRooms()
{
	if (m_Rooms < 2)
		return;

	// for (int r = 1; r < m_Rooms; r++)
	//	Connect(m_aRoom[r-1], m_aRoom[r]);

	// connect to closest room
	for (int r = 0; r < m_Rooms; r++)
	{
		vec2 Closest = vec2(-1000000, 0);

		for (int r2 = 0; r2 < m_Rooms; r2++)
			if (r2 != r)
				if (distance(m_aRoom[r], m_aRoom[r2]) < distance(m_aRoom[r], Closest))
					Closest = m_aRoom[r2];

		if (Closest.x >= 0.0f)
			Connect(m_aRoom[r], Closest);
	}
}

void CMaze::ConnectEverything()
{
	// find unconnected
	bool Looping = true;
	int i = 0;

	SetConnections(GetUnconnected());

	while (Looping && i++ < 1000)
	{
		ivec2 n = GetUnconnected();

		if (n.x <= 0)
			Looping = false;
		else
		{
			ivec2 np = GetClosestConnected(n);
			if (np.x > 0)
			{
				Connect(vec2(np.x, np.y), vec2(n.x, n.y));

				// for (int c = 0; c < m_W*m_H; c++)
				//	m_aConnected[c] = false;
			}

			SetConnections(n);
		}
	}
}

ivec2 CMaze::GetUnconnected()
{
	bool Looping = true;
	int i = 0;

	// check random spots
	while (Looping && i++ < 1000)
	{
		ivec2 p = ivec2(1 + MapGenRand() % (m_W - 2), 1 + MapGenRand() % (m_H - 2));
		if (m_aOpen[p.x + p.y * m_W] && !m_aConnected[p.x + p.y * m_W])
			return p;
	}

	// check everything if random failed
	for (int x = 1; x < m_W - 1; x++)
		for (int y = 1; y < m_H - 1; y++)
			if (m_aOpen[x + y * m_W] && !m_aConnected[x + y * m_W])
				return ivec2(x, y);

	return ivec2(-1, -1);
}

ivec2 CMaze::GetClosestConnected(ivec2 Pos)
{
	vec2 p0 = vec2(Pos.x, Pos.y);
	ivec2 Closest = ivec2(-1, -1);
	float d = 90000;

	for (int x = 1; x < m_W - 1; x++)
		for (int y = 1; y < m_H - 1; y++)
			if (m_aConnected[x + y * m_W] && m_aOpen[x + y * m_W])
				if (distance(p0, vec2(x, y)) < d)
				{
					Closest = ivec2(x, y);
					d = distance(p0, vec2(x, y));
				}

	return Closest;
}

vec2 CMaze::GetClosestRoom(vec2 Pos)
{
	if (m_Rooms < 1)
		return Pos;

	if (m_Rooms == 1)
		return m_aRoom[0];

	vec2 Closest = Pos;
	float ClosestDist = 90000;

	for (int i = 0; i < m_Rooms; i++)
	{
		float d = distance(m_aRoom[i], Pos);

		if (d < ClosestDist)
		{
			Closest = m_aRoom[i];
			ClosestDist = d;
		}
	}

	return Closest;
}

void CMaze::SetConnections(ivec2 Pos)
{
	if (Pos.x < 0 || Pos.y < 0 || Pos.x >= m_W || Pos.y >= m_H)
		return;

	if (m_aConnected[Pos.x + Pos.y * m_W] || !m_aOpen[Pos.x + Pos.y * m_W])
		return;

	m_aConnected[Pos.x + Pos.y * m_W] = true;

	SetConnections(Pos + ivec2(1, 0));
	SetConnections(Pos + ivec2(-1, 0));
	SetConnections(Pos + ivec2(0, 1));
	SetConnections(Pos + ivec2(0, -1));
}

void CMaze::Connect(vec2 Pos0, vec2 Pos1)
{
	float Distance = distance(Pos0, Pos1) * 2;
	int End(Distance + 1);

	for (int i = 0; i < End; i++)
	{
		float a = i / Distance;
		vec2 Pos = mix(Pos0, Pos1, a);

		Open(Pos);
		Open(Pos + vec2(-1, -1));
		Open(Pos + vec2(1, -1));
		Open(Pos + vec2(1, 1));
		Open(Pos + vec2(-1, 1));
	}
}

void CMaze::Open(vec2 Pos, int Size)
{
	Open(Pos.x, Pos.y);

	/*
	for (int x = -(Size-1); x < (Size-1); x++)
		for (int y = -(Size-1); y < (Size-1); y++)
			Open(Pos.x+x, Pos.y+y);
		*/
}

void CMaze::Open(int x, int y)
{
	// Keep a two-tile solid border at map edges.
	x = max(2, x);
	x = min(m_W - 3, x);
	y = max(2, y);
	y = min(m_H - 3, y);

	m_aOpen[x + y * m_W] = true;
}

void CMaze::OpenRooms(CRoom *pRoom, int OffX, int OffY)
{
	for (int x = 0; x < m_W; x++)
		for (int y = 0; y < m_H; y++)
			if (m_aOpen[x + y * m_W])
				pRoom->Open(x + OffX, y + OffY);
}

void CMaze::OpenRect(int x, int y, int w, int h)
{
	for(int py = y; py < y + h; py++)
		for(int px = x; px < x + w; px++)
			Open(px, py);
}

void CMaze::ConnectToFacility(vec2 From, vec2 To)
{
	Connect(From, To);
	Open(From, 2);
	Open(To, 2);
}

void CMaze::Carve(CGenLayer *pTiles) const
{
	if(!pTiles)
		return;

	for(int x = 2; x < m_W - 2; x++)
		for(int y = 2; y < m_H - 2; y++)
			if(m_aOpen[x + y * m_W])
				pTiles->Set(0, x, y);
}

// Flood
void CMaze::EnsureAccessibility()
{
	bool *Visited = new bool[m_W * m_H]{false};
	std::queue<ivec2> Queue;

	for (int x = 0; x < m_W; x++)
	{
		for (int y = 0; y < m_H; y++)
		{
			if (m_aOpen[x + y * m_W])
			{
				Queue.push(ivec2(x, y));
				Visited[x + y * m_W] = true;
				x = m_W;
				break;
			}
		}
	}

	while (!Queue.empty())
	{
		ivec2 Pos = Queue.front();
		Queue.pop();

		const ivec2 Dirs[] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
		for (const auto &Dir : Dirs)
		{
			ivec2 NewPos = Pos + Dir;
			if (NewPos.x >= 0 && NewPos.x < m_W &&
				NewPos.y >= 0 && NewPos.y < m_H)
			{
				int Index = NewPos.x + NewPos.y * m_W;
				if (m_aOpen[Index] && !Visited[Index])
				{
					Visited[Index] = true;
					Queue.push(NewPos);
				}
			}
		}
	}

	for (int x = 0; x < m_W; x++)
	{
		for (int y = 0; y < m_H; y++)
		{
			if (m_aOpen[x + y * m_W] && !Visited[x + y * m_W])
			{
				ivec2 Closest = GetClosestConnected(ivec2(x, y));
				if (Closest.x != -1)
				{
					Connect(vec2(x, y), vec2(Closest.x, Closest.y));
				}
			}
		}
	}

	delete[] Visited;
}

vec2 CMaze::GetFurthestPoint(vec2 Origin) const
{
    vec2 FurthestPoint = vec2(0, 0);
    float MaxDistance = 0.0f;

    for (int i = 0; i < m_Rooms; i++)
    {
        vec2 RoomPoint = m_aRoom[i];
        float Dist = distance(RoomPoint, Origin);
        if (Dist > MaxDistance)
        {
            MaxDistance = Dist;
            FurthestPoint = RoomPoint;
        }
    }

    return FurthestPoint;
}