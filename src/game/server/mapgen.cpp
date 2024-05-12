// Ninslash
#include <random>

#include <stdio.h> // sscanf
#include <base/system.h>
#include <base/math.h>
#include <base/vmath.h>
#include <engine/shared/config.h>
#include <engine/shared/linereader.h>

#include "mapgen.h"
#include <game/server/mapgen/gen_layer.h>
#include <game/server/mapgen/room.h>
#include <game/server/mapgen/maze.h>
#include <game/server/gamecontext.h>
#include <game/layers.h>
#include <game/mapitems.h>

CMapGen::CMapGen()
{
	m_pLayers = 0x0;
	m_pCollision = 0x0;
	m_pStorage = 0x0;
	m_FileLoaded = false;
}
CMapGen::~CMapGen()
{
}

void CMapGen::Init(CLayers *pLayers, CCollision *pCollision, IStorage *pStorage)
{
	m_pLayers = pLayers;
	m_pCollision = pCollision;
	m_pStorage = pStorage;

	Load("metal_main");
}

void CMapGen::Load(const char *pTileName)
{
	char aPath[256];
	str_format(aPath, sizeof(aPath), "mapgen/%s.rules", pTileName);
	IOHANDLE RulesFile = Storage()->OpenFile(aPath, IOFLAG_READ, IStorage::TYPE_ALL);
	if (!RulesFile)
		return;

	CLineReader LineReader;
	LineReader.Init(RulesFile);

	CConfiguration *pCurrentConf = 0;
	CIndexRule *pCurrentIndex = 0;

	// read each line
	while (char *pLine = LineReader.Get())
	{
		// skip blank/empty lines as well as comments
		if (str_length(pLine) > 0 && pLine[0] != '#' && pLine[0] != '\n' && pLine[0] != '\r' && pLine[0] != '\t' && pLine[0] != '\v' && pLine[0] != ' ')
		{
			if (pLine[0] == '[')
			{
				// new configuration, get the name
				pLine++;

				CConfiguration NewConf;
				int ID = m_lConfigs.add(NewConf);
				pCurrentConf = &m_lConfigs[ID];

				str_copy(pCurrentConf->m_aName, pLine, str_length(pLine));
			}
			else
			{
				if (!str_comp_num(pLine, "Index", 5))
				{
					// new index
					int ID = 0;
					char aFlip[128] = "";

					sscanf(pLine, "Index %d %127s", &ID, aFlip);

					CIndexRule NewIndexRule;
					NewIndexRule.m_ID = ID;
					NewIndexRule.m_Flag = 0;
					NewIndexRule.m_RandomValue = 0;
					NewIndexRule.m_YDivisor = 0;
					NewIndexRule.m_YRemainder = 0;
					NewIndexRule.m_BaseTile = false;

					if (str_length(aFlip) > 0)
					{
						if (!str_comp(aFlip, "XFLIP"))
							NewIndexRule.m_Flag = TILEFLAG_VFLIP;
						else if (!str_comp(aFlip, "YFLIP"))
							NewIndexRule.m_Flag = TILEFLAG_HFLIP;
						else if (!str_comp(aFlip, "XYFLIP"))
							NewIndexRule.m_Flag = TILEFLAG_VFLIP + TILEFLAG_HFLIP;
						else if (!str_comp(aFlip, "ROTATE"))
							NewIndexRule.m_Flag = TILEFLAG_ROTATE;
						else if (!str_comp(aFlip, "XFLIP_ROTATE"))
							NewIndexRule.m_Flag = TILEFLAG_ROTATE + TILEFLAG_VFLIP;
						else if (!str_comp(aFlip, "YFLIP_ROTATE"))
							NewIndexRule.m_Flag = TILEFLAG_ROTATE + TILEFLAG_HFLIP;
						else if (!str_comp(aFlip, "XYFLIP_ROTATE"))
							NewIndexRule.m_Flag = TILEFLAG_ROTATE + TILEFLAG_VFLIP + TILEFLAG_HFLIP;
					}

					// add the index rule object and make it current
					int ArrayID = pCurrentConf->m_aIndexRules.add(NewIndexRule);
					pCurrentIndex = &pCurrentConf->m_aIndexRules[ArrayID];
				}
				else if (!str_comp_num(pLine, "BaseTile", 8) && pCurrentIndex)
				{
					pCurrentIndex->m_BaseTile = true;
				}
				else if (!str_comp_num(pLine, "Pos", 3) && pCurrentIndex)
				{
					int x = 0, y = 0;
					char aValue[128];
					int Value = CPosRule::EMPTY;
					bool IndexValue = false;

					sscanf(pLine, "Pos %d %d %127s", &x, &y, aValue);

					if (!str_comp(aValue, "FULL"))
						Value = CPosRule::FULL;
					else if (!str_comp_num(aValue, "INDEX", 5))
					{
						sscanf(pLine, "Pos %*d %*d INDEX %d", &Value);
						IndexValue = true;
					}

					CPosRule NewPosRule = {x, y, Value, IndexValue};
					pCurrentIndex->m_aRules.add(NewPosRule);
				}
				else if (!str_comp_num(pLine, "Random", 6) && pCurrentIndex)
				{
					sscanf(pLine, "Random %d", &pCurrentIndex->m_RandomValue);
				}
				else if (!str_comp_num(pLine, "YRemainder", 10) && pCurrentIndex)
				{
					sscanf(pLine, "YRemainder %d %d", &pCurrentIndex->m_YDivisor, &pCurrentIndex->m_YRemainder);
				}
			}
		}
	}

	io_close(RulesFile);

	m_FileLoaded = true;
}

const char *CMapGen::GetConfigName(int Index)
{
	if (Index < 0 || Index >= m_lConfigs.size())
		return "";

	return m_lConfigs[Index].m_aName;
}

void CMapGen::FillMap()
{
	dbg_msg("mapgen", "started map generation");

	for (int i = 0; i < g_Config.m_SvMapGenLevel; i++)
		rand();

	int64 ProcessTime = 0;
	int64 TotalTime = time_get();

	int MineTeeLayerSize = m_pLayers->GameLayer()->m_Width * m_pLayers->GameLayer()->m_Height;

	// clear map, but keep background, envelopes etc
	ProcessTime = time_get();
	for (int i = 0; i < MineTeeLayerSize; i++)
	{
		int x = i % m_pLayers->GameLayer()->m_Width;
		ivec2 TilePos(x, (i - x) / m_pLayers->GameLayer()->m_Width);

		// clear the different layers
		ModifTile(TilePos, m_pLayers->GetGameLayerIndex(), 0);
		ModifTile(TilePos, m_pLayers->GetBackgroundLayerIndex(), 0);
		ModifTile(TilePos, m_pLayers->GetDoodadsLayerIndex(), 0);
		ModifTile(TilePos, m_pLayers->GetForegroundLayerIndex(), 0);
	}
	dbg_msg("mapgen", "map normalized in %.5fs", (float)(time_get() - ProcessTime) / time_freq());

	ProcessTime = time_get();

	GenerateLevel();

	dbg_msg("mapgen", "map successfully generated in %.5fs", (float)(time_get() - TotalTime) / time_freq());
}

void CMapGen::GenerateEnemySpawn(CGenLayer *pTiles)
{
	ivec2 p = pTiles->GetPlatform();

	if (p.x == 0)
		p = pTiles->GetMedPlatform();

	if (p.x == 0)
		p = pTiles->GetOpenArea();

	if (p.x == 0)
		p = pTiles->GetCeiling();

	if (p.x == 0)
		return;

	ModifTile(p + ivec2(-1, 0), m_pLayers->GetGameLayerIndex(), ENTITY_OFFSET + ENTITY_MONSTER_SPAWN);
	ModifTile(p + ivec2(+1, 0), m_pLayers->GetGameLayerIndex(), ENTITY_OFFSET + ENTITY_MONSTER_SPAWN);
	pTiles->Use(p.x, p.y);
}

void CMapGen::GenerateLevel()
{
	int w = m_pLayers->GameLayer()->m_Width;
	int h = m_pLayers->GameLayer()->m_Height;

	if (w < 10 || h < 10)
		return;

	CGenLayer *pTiles = new CGenLayer(w, h);

	// generate room structure
	CRoom *pRoom = new CRoom(3, 3, w - 6, h - 6);
	CMaze *pMaze = new CMaze(w, h);

	int Level = g_Config.m_SvMapGenLevel;

	pMaze->OpenRooms(pRoom);

	pRoom->Generate(pTiles);

	// pTiles->GenerateMoreForeground();

	// check for too tight corridors
	{
		for (int y = 3; y < h - 4; y++)
			for (int x = 3; x < w - 4; x++)
			{
				if (!pTiles->Get(x - 1, y) && pTiles->Get(x, y) && pTiles->Get(x + 1, y) && !pTiles->Get(x + 2, y))
					pRoom->Fill(pTiles, 0, x, y, 2, 1);

				if (!pTiles->Get(x, y - 1) && pTiles->Get(x, y) && pTiles->Get(x, y + 1) && !pTiles->Get(x, y + 2))
					pRoom->Fill(pTiles, 0, x, y, 1, 2);
			}
	}

	pTiles->GenerateSlopes();
	pTiles->RemoveSingles();

	dbg_msg("mapgen", "rooms generated, map size: %d", pTiles->Size());

	int n = pTiles->Size() / 500;

	pTiles->GenerateBackground();
	pTiles->GenerateMoreBackground();

	if (n > 1)
		pTiles->GenerateAirPlatforms(n / 2 + rand() % (n / 2));
	else
		pTiles->GenerateAirPlatforms(n);

	dbg_msg("mapgen", "Proceed tiles");
	Proceed(pTiles, 0);

	pTiles->GenerateBoxes();
	pTiles->GeneratePlatforms();

	pTiles->GenerateFences();

	// write to layers; foreground
	for (int x = 0; x < w; x++)
		for (int y = 0; y < h; y++)
		{
			int i = pTiles->Get(x, y);

			if (i > 0)
			{
				int f = pTiles->GetFlags(x, y);
				ModifTile(ivec2(x, y), m_pLayers->GetForegroundLayerIndex(), i, f);
				ModifTile(ivec2(x, y), m_pLayers->GetGameLayerIndex(), TILE_SOLID);
			}
		}

	// write to layers; FGOBJECTS to foreground
	for (int x = 0; x < w; x++)
		for (int y = 0; y < h; y++)
		{
			int i = pTiles->Get(x, y, CGenLayer::FGOBJECTS);

			if (i > 0)
			{
				int f = pTiles->GetFlags(x, y, CGenLayer::FGOBJECTS);
				ModifTile(ivec2(x, y), m_pLayers->GetForegroundLayerIndex(), i, f);

				if (i >= 14 * 16 + 1 && i <= 14 * 16 + 3)
					ModifTile(ivec2(x, y), m_pLayers->GetGameLayerIndex(), TILE_AIR);
				else
					ModifTile(ivec2(x, y), m_pLayers->GetGameLayerIndex(), 1);
			}
		}

	// background
	for (int x = 0; x < w; x++)
		for (int y = 0; y < h; y++)
		{
			int i = pTiles->Get(x, y, CGenLayer::BACKGROUND);

			if (i > 0)
				ModifTile(ivec2(x, y), m_pLayers->GetBackgroundLayerIndex(), i, pTiles->GetFlags(x, y, CGenLayer::BACKGROUND));
		}

	// doodads
	for (int x = 0; x < w; x++)
		for (int y = 0; y < h; y++)
		{
			int i = pTiles->Get(x, y, CGenLayer::DOODADS);

			if (i > 0)
				ModifTile(ivec2(x, y), m_pLayers->GetDoodadsLayerIndex(), i, pTiles->GetFlags(x, y, CGenLayer::DOODADS));
		}

	// find platforms, corners etc.
	dbg_msg("mapgen", "Scanning level");
	pTiles->Scan();

	ivec2 p = pTiles->GetPlayerSpawn();
	// start pos
	ModifTile(p + ivec2(-1, -1), m_pLayers->GetGameLayerIndex(), ENTITY_OFFSET + ENTITY_SPAWN);
	ModifTile(p + ivec2(+1, -1), m_pLayers->GetGameLayerIndex(), ENTITY_OFFSET + ENTITY_SPAWN);

	ModifTile(p, m_pLayers->GetGameLayerIndex(), ENTITY_OFFSET + ENTITY_SHIP);

	// enemy spawn positions
	for (int i = 0; i < 5; i++)
		GenerateEnemySpawn(pTiles);

	w = 4 + min(4, Level / 3);

	for (int i = 0; i < max(60, g_Config.m_GcRounds*3); i++)
		GenerateWeapon(pTiles, ENTITY_SCRAP_L1);

	for (int i = 0; i < min(30, g_Config.m_GcRounds*5); i++)
		GenerateWeapon(pTiles, ENTITY_SCRAP_L2);

	for (int i = 0; i < min(20, g_Config.m_GcRounds); i++)
		GenerateWeapon(pTiles, ENTITY_SCRAP_L3);

	// more enemy spawn positions
	for (int i = 0; i < min(Level, 10); i++)
		GenerateEnemySpawn(pTiles);

	if (pRoom)
		delete pRoom;

	if (pTiles)
		delete pTiles;

	if (pMaze)
		delete pMaze;

	dbg_msg("mapgen", "Level generated");
}

void CMapGen::Mirror(CGenLayer *pTiles)
{
	int w = pTiles->Width();
	int h = pTiles->Height();

	for (int x = 0; x < w / 2; x++)
		for (int y = 0; y < h; y++)
		{
			pTiles->Set(pTiles->Get(w / 2 - x, y), w / 2 + x, y);
		}
}

void CMapGen::WriteBase(class CGenLayer *pTiles, int BaseNum, ivec2 Pos, float Size)
{
	int w = m_pLayers->GameLayer()->m_Width;
	int h = m_pLayers->GameLayer()->m_Height;

	CGenLayer *pBaseTiles = new CGenLayer(w, h);
	pBaseTiles->CleanTiles();

	// copy tiles & check distance to base pos
	for (int x = 1; x < w - 1; x++)
		for (int y = 1; y < h - 1; y++)
		{
			int i = pTiles->Get(x, y);

			if (i > 0 && distance(vec2(Pos.x, Pos.y), vec2(x, y)) < Size)
				pBaseTiles->Set(1, x, y);
		}

	// auto map
	pBaseTiles->RemoveSingles();
	pBaseTiles->BaseCleanup();
	Proceed(pBaseTiles, 0);

	// write to layer
	int LayerIndex = 0;

	if (BaseNum == 0)
		LayerIndex = m_pLayers->GetBase1LayerIndex();
	else if (BaseNum == 1)
		LayerIndex = m_pLayers->GetBase2LayerIndex();

	for (int x = 0; x < w; x++)
		for (int y = 0; y < h; y++)
		{
			int i = pBaseTiles->Get(x, y);

			if (i > 0)
			{
				int f = pBaseTiles->GetFlags(x, y);
				ModifTile(ivec2(x, y), LayerIndex, i, f);
			}
		}

	delete pBaseTiles;
}

void CMapGen::WriteLayers(CGenLayer *pTiles)
{
	int w = m_pLayers->GameLayer()->m_Width;
	int h = m_pLayers->GameLayer()->m_Height;

	// write to layers; foreground
	for (int x = 0; x < w; x++)
		for (int y = 0; y < h; y++)
		{
			int i = pTiles->Get(x, y);

			if (i > 0)
			{
				int f = pTiles->GetFlags(x, y);
				ModifTile(ivec2(x, y), m_pLayers->GetForegroundLayerIndex(), i, f);
				ModifTile(ivec2(x, y), m_pLayers->GetGameLayerIndex(), TILE_SOLID);
			}
		}

	// write to layers; FGOBJECTS to foreground
	for (int x = 0; x < w; x++)
		for (int y = 0; y < h; y++)
		{
			int i = pTiles->Get(x, y, CGenLayer::FGOBJECTS);

			if (i > 0)
			{
				int f = pTiles->GetFlags(x, y, CGenLayer::FGOBJECTS);
				ModifTile(ivec2(x, y), m_pLayers->GetForegroundLayerIndex(), i, f);

				if (i >= 14 * 16 + 1 && i <= 14 * 16 + 3)
					ModifTile(ivec2(x, y), m_pLayers->GetGameLayerIndex(), TILE_AIR);
				else
					ModifTile(ivec2(x, y), m_pLayers->GetGameLayerIndex(), 1);
			}
		}

	/*
	// background
	for(int x = 0; x < w; x++)
		for(int y = 0; y < h; y++)
		{
			int i = pTiles->Get(x, y, CGenLayer::BACKGROUND);

			if (i > 0)
				ModifTile(ivec2(x, y), m_pLayers->GetBackgroundLayerIndex(), i, pTiles->GetFlags(x, y, CGenLayer::BACKGROUND));
		}
	*/

	// doodads
	for (int x = 0; x < w; x++)
		for (int y = 0; y < h; y++)
		{
			int i = pTiles->Get(x, y, CGenLayer::DOODADS);

			if (i > 0)
				ModifTile(ivec2(x, y), m_pLayers->GetDoodadsLayerIndex(), i, pTiles->GetFlags(x, y, CGenLayer::DOODADS));
		}
}

void CMapGen::WriteBackground(CGenLayer *pTiles)
{
	int w = m_pLayers->GameLayer()->m_Width;
	int h = m_pLayers->GameLayer()->m_Height;

	// background
	for (int x = 0; x < w; x++)
		for (int y = 0; y < h; y++)
		{
			int i = pTiles->Get(x, y, CGenLayer::BACKGROUND);

			if (i > 0)
				ModifTile(ivec2(x, y), m_pLayers->GetBackgroundLayerIndex(), i, pTiles->GetFlags(x, y, CGenLayer::BACKGROUND));
		}
}

inline void CMapGen::ModifTile(ivec2 Pos, int Layer, int Tile, int Flags)
{
	if (Layer == m_pLayers->GetForegroundLayerIndex() && Tile != 0)
		dbg_msg("dsadasd", "x:%d, y:%d Tile:%d", Pos.x, Pos.y, Tile);
	m_pCollision->ModifTile(Pos, m_pLayers->GetGameGroupIndex(), Layer, Tile, Flags, 0);
}

void CMapGen::Proceed(CGenLayer *pTiles, int ConfigID)
{
	if (!m_FileLoaded || ConfigID < 0 || ConfigID >= m_lConfigs.size())
		return;

	CConfiguration *pConf = &m_lConfigs[ConfigID];

	if (!pConf->m_aIndexRules.size())
		return;

	int BaseTile = 1;

	// find base tile if there is one
	for (int i = 0; i < pConf->m_aIndexRules.size(); ++i)
	{
		if (pConf->m_aIndexRules[i].m_BaseTile)
		{
			BaseTile = pConf->m_aIndexRules[i].m_ID;
			break;
		}
	}

	int Width = m_pLayers->GameLayer()->m_Width;
	int Height = m_pLayers->GameLayer()->m_Height;

	// auto map !
	int MaxIndex = Width * Height;
	for (int l = 0; l < 3; l++)
		for (int y = 0; y < Height; y++)
			for (int x = 0; x < Width; x++)
			{
				if (pTiles->Get(x, y, l) == 0)
					continue;

				pTiles->Set(BaseTile, x, y, 0, l);

				if (y == 0 || y == Height - 1 || x == 0 || x == Width - 1)
					continue;

				for (int i = 0; i < pConf->m_aIndexRules.size(); ++i)
				{
					if (pConf->m_aIndexRules[i].m_BaseTile)
						continue;

					bool RespectRules = true;
					for (int j = 0; j < pConf->m_aIndexRules[i].m_aRules.size() && RespectRules; ++j)
					{
						CPosRule *pRule = &pConf->m_aIndexRules[i].m_aRules[j];
						int CheckIndex = (y + pRule->m_Y) * Width + (x + pRule->m_X);

						if (CheckIndex < 0 || CheckIndex >= MaxIndex)
							RespectRules = false;
						else
						{
							if (pRule->m_IndexValue)
							{
								if (pTiles->GetByIndex(CheckIndex, l) != pRule->m_Value)
									RespectRules = false;
							}
							else
							{
								if (pTiles->GetByIndex(CheckIndex, l) > 0 && pRule->m_Value == CPosRule::EMPTY)
									RespectRules = false;

								if (pTiles->GetByIndex(CheckIndex, l) == 0 && pRule->m_Value == CPosRule::FULL)
									RespectRules = false;
							}
						}
					}

					if (RespectRules &&
						(pConf->m_aIndexRules[i].m_YDivisor < 2 || y % pConf->m_aIndexRules[i].m_YDivisor == pConf->m_aIndexRules[i].m_YRemainder) &&
						(pConf->m_aIndexRules[i].m_RandomValue <= 1 || (int)((float)rand() / ((float)RAND_MAX + 1) * pConf->m_aIndexRules[i].m_RandomValue) == 1))
					{
						pTiles->Set(pConf->m_aIndexRules[i].m_ID, x, y, pConf->m_aIndexRules[i].m_Flag, l);
					}
				}
			}
}

void CMapGen::GenerateWeapon(CGenLayer *pTiles, int Weapon)
{
	ivec2 p = ivec2(0, 0);

	p = pTiles->GetTopCorner();

	if (p.x != 0)
	{
		if (pTiles->Get(p.x - 1, p.y))
			p.x += 1;
		else
			p.x -= 1;

		p.y += 1;

		pTiles->Use(p.x, p.y);
		ModifTile(p, m_pLayers->GetGameLayerIndex(), ENTITY_OFFSET + Weapon);
	}
	else
	{
		p = pTiles->GetCeiling();

		if (p.x != 0)
		{
			p.y += 1;

			pTiles->Use(p.x, p.y);
			ModifTile(p, m_pLayers->GetGameLayerIndex(), ENTITY_OFFSET + Weapon);
		}
		else
		{
			p = pTiles->GetPlatform();

			if (p.x == 0)
				return;

			pTiles->Use(p.x, p.y);
			ModifTile(p, m_pLayers->GetGameLayerIndex(), ENTITY_OFFSET + Weapon);
		}
	}
}