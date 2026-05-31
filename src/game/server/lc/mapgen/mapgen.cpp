// Ninslash
#include <random>

#include <stdio.h> // sscanf
#include <base/system.h>
#include <base/math.h>
#include <base/vmath.h>
#include <engine/shared/config.h>
#include <engine/shared/linereader.h>
#include <engine/shared/datafile.h>

#include "mapgen.h"
#include <game/server/lc/mapgen/gen_layer.h>
#include <game/server/lc/mapgen/room.h>
#include <game/server/lc/mapgen/maze.h>
#include <game/server/lc/mapgen/mapgen_random.h>
#include <game/server/lc/mapgen/mapgen_theme.h>
#include <game/server/core/gamecontext.h>
#include <game/server/lc/expedition/balance.h>
#include <game/server/lc/expedition/moons.h>
#include <game/server/lc/hazards/hazards.h>
#include <game/layers.h>
#include <game/mapitems.h>

#include <queue>

namespace
{
const int MAPGEN_BORDER = 2;

static void EnforceGenBorder(CGenLayer *pTiles, int W, int H, int Thickness)
{
	for(int t = 0; t < Thickness; t++)
	{
		for(int x = 0; x < W; x++)
		{
			pTiles->Set(1, x, t);
			pTiles->Set(1, x, H - 1 - t);
		}
		for(int y = 0; y < H; y++)
		{
			pTiles->Set(1, t, y);
			pTiles->Set(1, W - 1 - t, y);
		}
	}
}

static bool IsGenAir(CGenLayer *pTiles, int x, int y)
{
	return pTiles->Get(x, y) == 0;
}

static int FloodFillGenComponent(CGenLayer *pTiles, int W, int H, int StartX, int StartY, int CompId, int *pComp)
{
	std::queue<ivec2> Queue;
	Queue.push(ivec2(StartX, StartY));
	pComp[StartX + StartY * W] = CompId;
	int Size = 0;

	while(!Queue.empty())
	{
		ivec2 Pos = Queue.front();
		Queue.pop();
		Size++;

		const ivec2 Dirs[] = {ivec2(1, 0), ivec2(-1, 0), ivec2(0, 1), ivec2(0, -1)};
		for(unsigned d = 0; d < sizeof(Dirs) / sizeof(Dirs[0]); d++)
		{
			ivec2 Next = Pos + Dirs[d];
			if(Next.x < MAPGEN_BORDER || Next.y < MAPGEN_BORDER || Next.x >= W - MAPGEN_BORDER || Next.y >= H - MAPGEN_BORDER)
				continue;
			const int Index = Next.x + Next.y * W;
			if(!IsGenAir(pTiles, Next.x, Next.y) || pComp[Index] >= 0)
				continue;
			pComp[Index] = CompId;
			Queue.push(Next);
		}
	}

	return Size;
}

static int RemoveDisconnectedGenAir(CGenLayer *pTiles, int W, int H)
{
	int *pComp = new int[W * H];
	for(int i = 0; i < W * H; i++)
		pComp[i] = -1;

	int NumComps = 0;
	int MainComp = -1;
	int MainSize = 0;

	for(int y = MAPGEN_BORDER; y < H - MAPGEN_BORDER; y++)
	{
		for(int x = MAPGEN_BORDER; x < W - MAPGEN_BORDER; x++)
		{
			const int Index = x + y * W;
			if(!IsGenAir(pTiles, x, y) || pComp[Index] >= 0)
				continue;

			const int Size = FloodFillGenComponent(pTiles, W, H, x, y, NumComps, pComp);
			if(Size > MainSize)
			{
				MainSize = Size;
				MainComp = NumComps;
			}
			NumComps++;
		}
	}

	int Removed = 0;
	if(MainComp >= 0)
	{
		for(int y = MAPGEN_BORDER; y < H - MAPGEN_BORDER; y++)
		{
			for(int x = MAPGEN_BORDER; x < W - MAPGEN_BORDER; x++)
			{
				const int Index = x + y * W;
				if(IsGenAir(pTiles, x, y) && pComp[Index] != MainComp)
				{
					pTiles->Set(1, x, y);
					Removed++;
				}
			}
		}
	}

	delete[] pComp;
	(void)Removed;
	return MainSize;
}

static bool GenBayInteriorClear(CGenLayer *pTiles, ivec2 BayPos, int BayW, int BayH)
{
	for(int dy = 0; dy < BayH - 2; dy++)
		for(int dx = 0; dx < BayW; dx++)
			if(pTiles->Get(BayPos.x + dx, BayPos.y + dy) != 0)
				return false;
	return true;
}

static int ScoreGenBay(CGenLayer *pTiles, int W, int H, ivec2 BayPos, int BayW, int BayH)
{
	if(!GenBayInteriorClear(pTiles, BayPos, BayW, BayH))
		return -1;

	int Score = 0;
	for(int dy = -2; dy <= BayH + 2; dy++)
	{
		for(int dx = -2; dx <= BayW + 3; dx++)
		{
			const int x = BayPos.x + dx;
			const int y = BayPos.y + dy;
			if(x >= MAPGEN_BORDER && x < W - MAPGEN_BORDER && y >= MAPGEN_BORDER && y < H - MAPGEN_BORDER && IsGenAir(pTiles, x, y))
				Score++;
		}
	}

	int ExitBonus = 0;
	const int ExitY = BayPos.y + BayH / 2;
	const int ExitX = BayPos.x + BayW;
	for(int dx = 0; dx < 16; dx++)
	{
		int Col = 0;
		for(int dy = -2; dy <= 2; dy++)
		{
			const int x = ExitX + dx;
			const int y = ExitY + dy;
			if(x >= MAPGEN_BORDER && x < W - MAPGEN_BORDER && y >= MAPGEN_BORDER && y < H - MAPGEN_BORDER && IsGenAir(pTiles, x, y))
				Col++;
		}
		if(Col > ExitBonus)
			ExitBonus = Col;
	}
	Score += ExitBonus * 8;

	if(BayPos.x < W / 3)
		Score += 12;

	return Score;
}

static void FillGenRect(CGenLayer *pTiles, int x, int y, int w, int h)
{
	for(int py = y; py < y + h; py++)
		for(int px = x; px < x + w; px++)
			pTiles->Set(1, px, py);
}

static void CarveGenBay(CGenLayer *pTiles, ivec2 BayPos, int BayW, int BayH)
{
	for(int dy = 0; dy < BayH; dy++)
		for(int dx = 0; dx < BayW; dx++)
			pTiles->Set(0, BayPos.x + dx, BayPos.y + dy);

	for(int dy = 0; dy < 3; dy++)
		for(int dx = 0; dx < 2; dx++)
			pTiles->Set(0, BayPos.x + BayW - 1 + dx, BayPos.y + BayH / 2 - 1 + dy);
}

static void FindBestGenShipBay(CGenLayer *pTiles, int W, int H, int BayW, int BayH, ivec2 OldBay, ivec2 &BayPos, ivec2 &ShipPos)
{
	int BestScore = -1;
	ivec2 BestBay = OldBay;

	for(int y = MAPGEN_BORDER + 1; y <= H - BayH - MAPGEN_BORDER - 1; y++)
	{
		for(int x = MAPGEN_BORDER + 1; x <= W - BayW - MAPGEN_BORDER - 1; x++)
		{
			const int Score = ScoreGenBay(pTiles, W, H, ivec2(x, y), BayW, BayH);
			if(Score > BestScore)
			{
				BestScore = Score;
				BestBay = ivec2(x, y);
			}
		}
	}

	if(BestScore < 0)
		return;

	if(BestBay.x != OldBay.x || BestBay.y != OldBay.y)
		FillGenRect(pTiles, OldBay.x - 1, OldBay.y - 1, BayW + 2, BayH + 2);

	BayPos = BestBay;
	ShipPos = ivec2(BayPos.x + BayW / 2, BayPos.y + BayH - 3);
	CarveGenBay(pTiles, BayPos, BayW, BayH);
}

static bool IsWalkableGameIndex(int Index)
{
	return Index != TILE_SOLID && Index != TILE_DEATH && Index != TILE_NOHOOK;
}
} // namespace

bool SMapGenStaging::Init(int W, int H)
{
	Free();
	if(W <= 0 || H <= 0)
		return false;

	const int Size = W * H;
	m_pGame = new(std::nothrow) CTile[Size];
	m_pBackground = new(std::nothrow) CTile[Size];
	m_pDoodads = new(std::nothrow) CTile[Size];
	m_pForeground = new(std::nothrow) CTile[Size];
	if(!m_pGame || !m_pBackground || !m_pDoodads || !m_pForeground)
	{
		Free();
		return false;
	}

	mem_zero(m_pGame, Size * sizeof(CTile));
	mem_zero(m_pBackground, Size * sizeof(CTile));
	mem_zero(m_pDoodads, Size * sizeof(CTile));
	mem_zero(m_pForeground, Size * sizeof(CTile));
	m_W = W;
	m_H = H;
	return true;
}

void SMapGenStaging::Free()
{
	delete[] m_pGame;
	delete[] m_pBackground;
	delete[] m_pDoodads;
	delete[] m_pForeground;
	m_pGame = 0;
	m_pBackground = 0;
	m_pDoodads = 0;
	m_pForeground = 0;
	m_W = 0;
	m_H = 0;
}

CMapGen::CMapGen()
{
	m_pLayers = 0x0;
	m_pCollision = 0x0;
	m_pStorage = 0x0;
	m_FileLoaded = false;
	m_Step = MAPGENSTEP_IDLE;
	m_ClearIndex = 0;
	m_LayerSize = 0;
	m_ApplyIndex = 0;
	m_aTemplateMap[0] = 0;
	m_pStaging = 0;
	mem_zero(&m_GenParams, sizeof(m_GenParams));
	m_pGenTiles = 0;
	m_pGenRoom = 0;
	m_pGenMaze = 0;
	m_GenSubStep = 0;
	m_GenCorridorY = 0;
	m_GenFacility = LC_FACILITY_FACTORY;
	m_GenGridLayout = false;
	m_GenBayPos = ivec2(0, 0);
	m_GenBayW = 0;
	m_GenBayH = 0;
	m_GenLevel = 0;
	m_GenW = 0;
	m_GenH = 0;
	m_GenShipPos = ivec2(0, 0);
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

static bool IsIndexFlipToken(const char *pToken)
{
	return !str_comp(pToken, "XFLIP") || !str_comp(pToken, "YFLIP") || !str_comp(pToken, "XYFLIP") ||
		!str_comp(pToken, "ROTATE") || !str_comp(pToken, "XFLIP_ROTATE") || !str_comp(pToken, "YFLIP_ROTATE") ||
		!str_comp(pToken, "XYFLIP_ROTATE");
}

void CMapGen::ParseIndexLine(const char *pLine, CIndexRule &Rule)
{
	mem_zero(&Rule, sizeof(Rule));

	const char *p = pLine + 5;
	while(*p == ' ')
		p++;

	while(*p)
	{
		while(*p == ' ')
			p++;
		if(!*p)
			break;

		if(IsIndexFlipToken(p))
			break;

		if(Rule.m_NumIDs < (int)(sizeof(Rule.m_aIDs) / sizeof(Rule.m_aIDs[0])))
			Rule.m_aIDs[Rule.m_NumIDs++] = str_toint(p);

		const char *pOr = str_find(p, " OR ");
		if(!pOr)
			break;
		p = pOr + 4;
	}

	if(Rule.m_NumIDs > 0)
		Rule.m_ID = Rule.m_aIDs[0];

	if(str_find(pLine, "XYFLIP_ROTATE"))
		Rule.m_Flag = TILEFLAG_ROTATE + TILEFLAG_VFLIP + TILEFLAG_HFLIP;
	else if(str_find(pLine, "XFLIP_ROTATE"))
		Rule.m_Flag = TILEFLAG_ROTATE + TILEFLAG_VFLIP;
	else if(str_find(pLine, "YFLIP_ROTATE"))
		Rule.m_Flag = TILEFLAG_ROTATE + TILEFLAG_HFLIP;
	else if(str_find(pLine, "XYFLIP"))
		Rule.m_Flag = TILEFLAG_VFLIP + TILEFLAG_HFLIP;
	else if(str_find(pLine, "ROTATE"))
		Rule.m_Flag = TILEFLAG_ROTATE;
	else if(str_find(pLine, "XFLIP"))
		Rule.m_Flag = TILEFLAG_VFLIP;
	else if(str_find(pLine, "YFLIP"))
		Rule.m_Flag = TILEFLAG_HFLIP;
}

bool CMapGen::ParsePosRuleLine(const char *pLine, CPosRule &Rule)
{
	mem_zero(&Rule, sizeof(Rule));

	int x = 0;
	int y = 0;
	char aValue[256];
	if(sscanf(pLine, "Pos %d %d %255[^\n]", &x, &y, aValue) != 3)
		return false;

	Rule.m_X = x;
	Rule.m_Y = y;

	const char *p = aValue;
	while(*p == ' ')
		p++;

	if(!str_comp(p, "FULL"))
	{
		Rule.m_Type = CPosRule::FULL;
		return true;
	}

	if(!str_comp(p, "EMPTY"))
	{
		Rule.m_Type = CPosRule::EMPTY;
		return true;
	}

	const bool NotIndex = str_comp_num(p, "NOTINDEX", 8) == 0;
	if(NotIndex)
		p += 8;
	else if(str_comp_num(p, "INDEX", 5) == 0)
		p += 5;
	else
		return false;

	Rule.m_Type = NotIndex ? CPosRule::NOTINDEX : CPosRule::INDEX;

	while(*p == ' ')
		p++;

	while(*p && Rule.m_NumIndexValues < (int)(sizeof(Rule.m_aIndexValues) / sizeof(Rule.m_aIndexValues[0])))
	{
		Rule.m_aIndexValues[Rule.m_NumIndexValues++] = str_toint(p);
		const char *pOr = str_find(p, " OR ");
		if(!pOr)
			break;
		p = pOr + 4;
	}

	return Rule.m_NumIndexValues > 0;
}

bool CMapGen::PosRuleMatches(const CPosRule &Rule, int CheckTile)
{
	switch(Rule.m_Type)
	{
	case CPosRule::EMPTY:
		return CheckTile == 0;
	case CPosRule::FULL:
		return CheckTile > 0;
	case CPosRule::INDEX:
		for(int i = 0; i < Rule.m_NumIndexValues; i++)
			if(CheckTile == Rule.m_aIndexValues[i])
				return true;
		return false;
	case CPosRule::NOTINDEX:
		if(CheckTile < 0)
			return false;
		for(int i = 0; i < Rule.m_NumIndexValues; i++)
			if(CheckTile == Rule.m_aIndexValues[i])
				return false;
		return true;
	default:
		return false;
	}
}

int CMapGen::PickIndexRuleTile(const CIndexRule &Rule)
{
	if(Rule.m_NumIDs <= 1)
		return Rule.m_ID;
	return Rule.m_aIDs[MapGenRand() % Rule.m_NumIDs];
}

bool CMapGen::TryLoadRules(const char *pTileName)
{
	char aPath[256];
	str_format(aPath, sizeof(aPath), "mapgen/%s.rules", pTileName);
	IOHANDLE RulesFile = Storage()->OpenFile(aPath, IOFLAG_READ, IStorage::TYPE_ALL);
	if(!RulesFile)
		return false;

	m_lConfigs.clear();

	CLineReader LineReader;
	LineReader.Init(RulesFile);

	CConfiguration *pCurrentConf = 0;
	CIndexRule *pCurrentIndex = 0;

	while(char *pLine = LineReader.Get())
	{
		if(str_length(pLine) > 0 && pLine[0] != '#' && pLine[0] != '\n' && pLine[0] != '\r' && pLine[0] != '\t' && pLine[0] != '\v' && pLine[0] != ' ')
		{
			if(pLine[0] == '[')
			{
				pLine++;

				char aName[128];
				str_copy(aName, pLine, sizeof(aName));
				const int NameLen = str_length(aName);
				if(NameLen > 0 && aName[NameLen - 1] == ']')
					aName[NameLen - 1] = 0;

				CConfiguration NewConf = {};
				int ID = m_lConfigs.add(NewConf);
				pCurrentConf = &m_lConfigs[ID];
				str_copy(pCurrentConf->m_aName, aName, sizeof(pCurrentConf->m_aName));
				pCurrentConf->m_aRunOffsets.add(0);
				pCurrentIndex = 0;
			}
			else if(!pCurrentConf)
			{
				continue;
			}
			else if(!str_comp(pLine, "NewRun"))
			{
				pCurrentConf->m_aRunOffsets.add(pCurrentConf->m_aIndexRules.size());
				pCurrentIndex = 0;
			}
			else if(!str_comp_num(pLine, "Index", 5))
			{
				if(pCurrentIndex && pCurrentIndex->m_aRules.size() == 0 && !pCurrentIndex->m_BaseTile)
					pCurrentIndex->m_BaseTile = true;

				CIndexRule NewIndexRule;
				ParseIndexLine(pLine, NewIndexRule);

				int ArrayID = pCurrentConf->m_aIndexRules.add(NewIndexRule);
				pCurrentIndex = &pCurrentConf->m_aIndexRules[ArrayID];
			}
			else if(!str_comp_num(pLine, "BaseTile", 8) && pCurrentIndex)
			{
				pCurrentIndex->m_BaseTile = true;
			}
			else if(!str_comp_num(pLine, "Pos", 3) && pCurrentIndex)
			{
				CPosRule NewPosRule;
				if(ParsePosRuleLine(pLine, NewPosRule))
					pCurrentIndex->m_aRules.add(NewPosRule);
			}
			else if(!str_comp_num(pLine, "Random", 6) && pCurrentIndex)
			{
				sscanf(pLine, "Random %d", &pCurrentIndex->m_RandomValue);
			}
			else if(!str_comp_num(pLine, "YRemainder", 10) && pCurrentIndex)
			{
				sscanf(pLine, "YRemainder %d %d", &pCurrentIndex->m_YDivisor, &pCurrentIndex->m_YRemainder);
			}
		}
	}

	io_close(RulesFile);

	for(int c = 0; c < m_lConfigs.size(); c++)
	{
		CConfiguration *pConf = &m_lConfigs[c];
		if(pConf->m_aRunOffsets.size() == 0)
			pConf->m_aRunOffsets.add(0);

		CIndexRule *pLastIndex = 0;
		for(int i = 0; i < pConf->m_aIndexRules.size(); i++)
		{
			CIndexRule *pIndex = &pConf->m_aIndexRules[i];
			if(pLastIndex && pLastIndex->m_aRules.size() == 0 && !pLastIndex->m_BaseTile)
				pLastIndex->m_BaseTile = true;
			pLastIndex = pIndex;
		}
	}

	return m_lConfigs.size() > 0 && m_lConfigs[0].m_aIndexRules.size() > 0;
}

void CMapGen::Load(const char *pTileName)
{
	m_lConfigs.clear();
	m_FileLoaded = false;

	if(TryLoadRules(pTileName))
	{
		m_FileLoaded = true;
		dbg_msg("mapgen", "loaded automapper rules '%s'", pTileName);
		return;
	}

	dbg_msg("mapgen", "failed to load automapper rules '%s'", pTileName);

	if(str_comp(pTileName, "metal_main") != 0 && TryLoadRules("metal_main"))
	{
		m_FileLoaded = true;
		dbg_msg("mapgen", "using fallback automapper rules 'metal_main'");
		return;
	}

	dbg_msg("mapgen", "no automapper rules available; wall tiles will not be automapped");
}

const char *CMapGen::GetConfigName(int Index)
{
	if (Index < 0 || Index >= m_lConfigs.size())
		return "";

	return m_lConfigs[Index].m_aName;
}

int CMapGen::FindConfigId(const char *pThemeRules) const
{
	if(!m_FileLoaded || m_lConfigs.size() == 0)
		return -1;

	const SLcMapgenThemeProfile *pProfile = LcGetMapgenThemeProfile(pThemeRules);
	if(pProfile && pProfile->m_pAutomapConfig[0])
	{
		for(int i = 0; i < m_lConfigs.size(); i++)
		{
			if(str_comp_nocase(m_lConfigs[i].m_aName, pProfile->m_pAutomapConfig) == 0)
				return i;
		}
	}

	dbg_msg("mapgen", "automapper config '%s' not found in rules for theme '%s', using first section",
		pProfile ? pProfile->m_pAutomapConfig : "", pThemeRules);
	return 0;
}

void CMapGen::ProceedTheme(CGenLayer *pTiles, const char *pThemeRules)
{
	Proceed(pTiles, FindConfigId(pThemeRules));
}

static void BindMapImageToExternalTileset(IMap *pMap, int ImageIndex, const char *pThemeName)
{
	if(!pMap || !pThemeName || !pThemeName[0] || ImageIndex < 0)
		return;

	int ImageStart = 0;
	int ImageNum = 0;
	pMap->GetType(MAPITEMTYPE_IMAGE, &ImageStart, &ImageNum);
	if(ImageIndex >= ImageNum)
		return;

	CMapItemImage *pImg = (CMapItemImage *)pMap->GetItem(ImageStart + ImageIndex, 0, 0);
	if(!pImg || pImg->m_ImageData < 0)
		return;

	char *pNameBuf = (char *)pMap->GetData(pImg->m_ImageData);
	if(!pNameBuf)
		return;

	str_copy(pNameBuf, pThemeName, 128);
	pImg->m_ImageName = pImg->m_ImageData;
	pImg->m_External = 1;
	pImg->m_ImageData = -1;
	pImg->m_Width = 1024;
	pImg->m_Height = 1024;
	if(pImg->m_Version >= CMapItemImage::CURRENT_VERSION)
		pImg->m_Format = CImageInfoFile::FORMAT_RGBA;
}

void CMapGen::ApplyThemeTilesets(const char *pThemeRules)
{
	if(!m_pLayers || !m_pLayers->Map() || !pThemeRules || !pThemeRules[0])
		return;

	if(!LcMapgenThemeUsesClientTileset(pThemeRules))
	{
		dbg_msg("mapgen", "theme '%s' uses embedded map tileset (not a client builtin)", pThemeRules);
		return;
	}

	IMap *pMap = m_pLayers->Map();

	auto BindLayer = [&](CMapItemLayerTilemap *pLayer)
	{
		if(!pLayer || pLayer->m_Image < 0)
			return;
		BindMapImageToExternalTileset(pMap, pLayer->m_Image, pThemeRules);
	};

	BindLayer(m_pLayers->ForegroundLayer());
	BindLayer(m_pLayers->BackgroundLayer());
	BindLayer(m_pLayers->DoodadsLayer());

	dbg_msg("mapgen", "bound tile layers to client tileset '%s'", pThemeRules);
}

void CMapGen::FillMap()
{
	SMapGenParams Params;
	Params.m_Seed = g_Config.m_SvMapGenSeed;
	Params.m_GcMoon = g_Config.m_GcMoon;
	Params.m_GcRounds = g_Config.m_GcRounds;
	Params.m_MapGenLevel = g_Config.m_SvMapGenLevel;
	SetParams(Params);
	CMapGenRandomScope RandScope(Params.m_Seed);
	BeginFillMap(g_Config.m_SvMapLobby, g_Config.m_SvMapgenTheme);
	while(IsGenerating())
		StepFillMap();
}

void CMapGen::SetParams(const SMapGenParams &Params)
{
	m_GenParams = Params;
}

void CMapGen::UseStaging(SMapGenStaging *pStaging)
{
	m_pStaging = pStaging;
}

void CMapGen::ClearStaging()
{
	m_pStaging = 0;
}

void CMapGen::ResetApplyIndex()
{
	m_ApplyIndex = 0;
}

bool CMapGen::ApplyStagingChunk(int TilesPerStep)
{
	if(!m_pStaging || !m_pLayers || !m_pCollision)
		return true;

	const int W = m_pStaging->m_W;
	const int H = m_pStaging->m_H;
	const int LayerSize = W * H;
	const int Group = m_pLayers->GetGameGroupIndex();
	const int GameLayer = m_pLayers->GetGameLayerIndex();
	const int BackgroundLayer = m_pLayers->GetBackgroundLayerIndex();
	const int DoodadsLayer = m_pLayers->GetDoodadsLayerIndex();
	const int ForegroundLayer = m_pLayers->GetForegroundLayerIndex();

	for(int n = 0; n < TilesPerStep && m_ApplyIndex < LayerSize; n++, m_ApplyIndex++)
	{
		const int x = m_ApplyIndex % W;
		const int y = m_ApplyIndex / W;
		const ivec2 Pos(x, y);
		const CTile &GameTile = m_pStaging->m_pGame[m_ApplyIndex];
		const CTile &BackgroundTile = m_pStaging->m_pBackground[m_ApplyIndex];
		const CTile &DoodadsTile = m_pStaging->m_pDoodads[m_ApplyIndex];
		const CTile &ForegroundTile = m_pStaging->m_pForeground[m_ApplyIndex];

		m_pCollision->ModifTile(Pos, Group, GameLayer, GameTile.m_Index, GameTile.m_Flags, GameTile.m_Reserved);
		m_pCollision->ModifTile(Pos, Group, BackgroundLayer, BackgroundTile.m_Index, BackgroundTile.m_Flags, BackgroundTile.m_Reserved);
		m_pCollision->ModifTile(Pos, Group, DoodadsLayer, DoodadsTile.m_Index, DoodadsTile.m_Flags, DoodadsTile.m_Reserved);
		m_pCollision->ModifTile(Pos, Group, ForegroundLayer, ForegroundTile.m_Index, ForegroundTile.m_Flags, ForegroundTile.m_Reserved);
	}

	return m_ApplyIndex >= LayerSize;
}

bool CMapGen::RestoreTemplateFromFile(const char *pMapName)
{
	if(!m_pLayers || !m_pLayers->GameLayer())
		return false;

	CDataFileReader Reader;
	char aPath[256];
	str_format(aPath, sizeof(aPath), "maps/%s.map", pMapName);
	if(!Reader.Open(Storage(), aPath, IStorage::TYPE_ALL))
	{
		dbg_msg("mapgen", "failed to open template map '%s'", aPath);
		return false;
	}

	int W = m_pLayers->GameLayer()->m_Width;
	int H = m_pLayers->GameLayer()->m_Height;

	int Start = 0;
	int Num = 0;
	Reader.GetType(MAPITEMTYPE_LAYER, &Start, &Num);

	for(int i = 0; i < Num; i++)
	{
		CMapItemLayer *pLayer = (CMapItemLayer *)Reader.GetItem(Start + i, 0, 0);
		if(!pLayer || pLayer->m_Type != LAYERTYPE_TILES)
			continue;

		CMapItemLayerTilemap *pTilemap = (CMapItemLayerTilemap *)pLayer;
		if(pTilemap->m_Width != W || pTilemap->m_Height != H)
			continue;

		CTile *pTiles = (CTile *)Reader.GetData(pTilemap->m_Data);
		if(!pTiles)
			continue;

		char aName[64] = {0};
		IntsToStr(pTilemap->m_aName, sizeof(pTilemap->m_aName)/sizeof(int), aName);

		int LayerIndex = -1;
		if(str_comp_nocase(aName, "background") == 0)
			LayerIndex = m_pLayers->GetBackgroundLayerIndex();
		else if(str_comp_nocase(aName, "doodads") == 0)
			LayerIndex = m_pLayers->GetDoodadsLayerIndex();
		else if(str_comp_nocase(aName, "foreground") == 0)
			LayerIndex = m_pLayers->GetForegroundLayerIndex();
		else if(pTilemap->m_Flags & TILESLAYERFLAG_GAME)
			LayerIndex = m_pLayers->GetGameLayerIndex();

		if(LayerIndex < 0)
			continue;

		for(int y = 0; y < H; y++)
			for(int x = 0; x < W; x++)
			{
				const CTile &Tile = pTiles[y * W + x];
				ModifTile(ivec2(x, y), LayerIndex, Tile.m_Index, Tile.m_Flags);
			}
	}

	Reader.Close();
	dbg_msg("mapgen", "restored template from '%s'", aPath);
	return true;
}

void CMapGen::ClearMapChunk(int TilesPerStep)
{
	if(!m_pLayers->GameLayer())
		return;

	int W = m_pLayers->GameLayer()->m_Width;
	for(int n = 0; n < TilesPerStep && m_ClearIndex < m_LayerSize; n++, m_ClearIndex++)
	{
		int x = m_ClearIndex % W;
		int y = m_ClearIndex / W;
		ivec2 TilePos(x, y);

		ModifTile(TilePos, m_pLayers->GetGameLayerIndex(), 0);
		ModifTile(TilePos, m_pLayers->GetBackgroundLayerIndex(), 0);
		ModifTile(TilePos, m_pLayers->GetDoodadsLayerIndex(), 0);
		ModifTile(TilePos, m_pLayers->GetForegroundLayerIndex(), 0);
	}
}

bool CMapGen::BeginFillMap(const char *pTemplateMap, const char *pThemeRules)
{
	if(m_Step != MAPGENSTEP_IDLE)
		return false;

	str_copy(m_aTemplateMap, pTemplateMap, sizeof(m_aTemplateMap));
	Load(pThemeRules);

	const int ConfigId = FindConfigId(pThemeRules);
	for(int i = 0; i < m_GenParams.m_MapGenLevel; i++)
		MapGenRand();

	m_LayerSize = m_pLayers->GameLayer()->m_Width * m_pLayers->GameLayer()->m_Height;
	m_ClearIndex = 0;
	m_Step = MAPGENSTEP_RESTORE_TEMPLATE;
	dbg_msg("mapgen", "started staged map generation (theme '%s', automapper section '%s')",
		pThemeRules, ConfigId >= 0 ? GetConfigName(ConfigId) : "none");
	return true;
}

bool CMapGen::StepFillMap()
{
	switch(m_Step)
	{
	case MAPGENSTEP_RESTORE_TEMPLATE:
		RestoreTemplateFromFile(m_aTemplateMap);
		m_Step = MAPGENSTEP_CLEAR;
		return false;

	case MAPGENSTEP_CLEAR:
		ClearMapChunk(2048);
		if(m_ClearIndex >= m_LayerSize)
		{
			m_Step = MAPGENSTEP_GENERATE;
			dbg_msg("mapgen", "map normalized");
		}
		return false;

	case MAPGENSTEP_GENERATE:
		if(m_GenSubStep == 0)
			BeginGenerateLevel();
		if(StepGenerateLevel())
		{
			int64 Start = time_get();
			dbg_msg("mapgen", "level generated in %.5fs", (float)(time_get() - Start) / time_freq());
			m_Step = MAPGENSTEP_DONE;
			return true;
		}
		return false;

	default:
		return false;
	}
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

void CMapGen::PlaceHazards(CGenLayer *pTiles, int FacilityType, ivec2 ShipPos, ivec2 BayPos, int BayW, int BayH)
{
	int w = pTiles->Width();
	int h = pTiles->Height();
	int Stars = LcGetMoon(m_GenParams.m_GcMoon)->m_HazardStars;
	const SLcMapgenThemeProfile *pTheme = LcGetMapgenThemeProfile(g_Config.m_SvMapgenTheme);
	int Game = m_pLayers->GetGameLayerIndex();
	int Fg = m_pLayers->GetForegroundLayerIndex();
	int Dd = m_pLayers->GetDoodadsLayerIndex();

	auto PaintHazard = [&](ivec2 HPos, int Hazard)
	{
		if(HPos.x < 2 || HPos.y < 2 || HPos.x >= w - 2 || HPos.y >= h - 2)
			return;
		if(pTiles->Get(HPos.x, HPos.y) != 0)
			return;

		ModifTile(HPos, Game, TILE_AIR, 0, Hazard);

		switch(Hazard)
		{
		case LC_HAZARD_GAS:
			for(int dy = -1; dy <= 1; dy++)
				for(int dx = -1; dx <= 1; dx++)
				{
					ivec2 O = HPos + ivec2(dx, dy);
					if(O.x > 1 && O.y > 1 && O.x < w - 2 && O.y < h - 2 && pTiles->Get(O.x, O.y) == 0)
					{
						ModifTile(O, Game, TILE_AIR, 0, LC_HAZARD_GAS);
						ModifTile(O, Dd, LC_TILE_DOODAD_GAS, 0, 0);
						ModifTile(O, Fg, LC_TILE_FG_GAS, 0, 0);
					}
				}
			break;
		case LC_HAZARD_MINE:
			break;
		case LC_HAZARD_GRASS:
			ModifTile(HPos, Dd, LC_TILE_DOODAD_GRASS, 0, 0);
			break;
		case LC_HAZARD_SHOCK:
			break;
		case LC_HAZARD_SPIKE:
			break;
		case LC_HAZARD_TAR:
			ModifTile(HPos, Dd, LC_TILE_DOODAD_TAR, 0, 0);
			ModifTile(HPos, Fg, LC_TILE_DOODAD_TAR, 0, 0);
			break;
		}
	};

	int Attempts = pTiles->Size() / 100;
	if(FacilityType == LC_FACILITY_MINES)
		Attempts = pTiles->Size() / 60;
	else if(FacilityType == LC_FACILITY_MANSION || FacilityType == LC_FACILITY_WAREHOUSE)
		Attempts = pTiles->Size() / 140;
	else if(FacilityType == LC_FACILITY_RESEARCH)
		Attempts = pTiles->Size() / 110;
	Attempts = Attempts * Stars;
	if(Attempts < 10)
		Attempts = 10;

	for(int n = 0; n < Attempts; n++)
	{
		ivec2 Pos = pTiles->GetOpenArea();
		if(Pos.x == 0)
			continue;
		if(Pos.x >= BayPos.x - 2 && Pos.x < BayPos.x + BayW + 2 &&
			Pos.y >= BayPos.y - 2 && Pos.y < BayPos.y + BayH + 2)
			continue;
		if(distance(vec2((float)Pos.x, (float)Pos.y), vec2((float)ShipPos.x, (float)ShipPos.y)) < 14.0f)
			continue;

		int GrassW = 20, GasW = 22, MineW = 14, ShockW = 12, SpikeW = 14, TarW = 10;
		if(pTheme)
		{
			GrassW += pTheme->m_GrassHazardBonus;
			GasW += pTheme->m_GasHazardBonus;
			MineW += pTheme->m_MineHazardBonus;
			ShockW += pTheme->m_ShockHazardBonus;
		}
		if(FacilityType == LC_FACILITY_MINES)
		{
			GasW = 30; MineW = 26; GrassW = 6; ShockW = 18; SpikeW = 16; TarW = 8;
		}
		else if(FacilityType == LC_FACILITY_MANSION || FacilityType == LC_FACILITY_WAREHOUSE)
		{
			GasW = 12; MineW = 8; GrassW = 34; ShockW = 8; SpikeW = 10; TarW = 6;
		}
		else if(FacilityType == LC_FACILITY_RESEARCH)
		{
			GasW = 18; MineW = 12; GrassW = 20; ShockW = 16; SpikeW = 14; TarW = 10;
		}

		if(GrassW < 0) GrassW = 0;
		if(GasW < 0) GasW = 0;
		if(MineW < 0) MineW = 0;
		if(ShockW < 0) ShockW = 0;
		if(SpikeW < 0) SpikeW = 0;
		if(TarW < 0) TarW = 0;

		int Roll = MapGenRand() % 100;
		int Hazard = LC_HAZARD_NONE;
		int Total = GrassW + GasW + MineW + ShockW + SpikeW + TarW;
		if(Total > 100)
			Total = 100;
		if(Roll < GrassW)
			Hazard = LC_HAZARD_GRASS;
		else if(Roll < GrassW + GasW)
			Hazard = LC_HAZARD_GAS;
		else if(Roll < GrassW + GasW + MineW)
			Hazard = LC_HAZARD_MINE;
		else if(Roll < GrassW + GasW + MineW + ShockW)
			Hazard = LC_HAZARD_SHOCK;
		else if(Roll < GrassW + GasW + MineW + ShockW + SpikeW)
			Hazard = LC_HAZARD_SPIKE;
		else if(Roll < Total)
			Hazard = LC_HAZARD_TAR;
		if(Hazard == LC_HAZARD_NONE)
			continue;

		int Cluster = Hazard == LC_HAZARD_GAS ? 1 + MapGenRand() % 2 : 1;
		if(Hazard == LC_HAZARD_TAR)
			Cluster = 2 + MapGenRand() % 3;
		for(int c = 0; c < Cluster; c++)
		{
			ivec2 HPos = Pos + ivec2(MapGenRand() % 9 - 4, MapGenRand() % 9 - 4);
			PaintHazard(HPos, Hazard);
		}
	}
}

void CMapGen::PlaceTurrets(CGenLayer *pTiles, int FacilityType, ivec2 ShipPos, ivec2 BayPos, int BayW, int BayH)
{
	int w = pTiles->Width();
	int h = pTiles->Height();
	int Game = m_pLayers->GetGameLayerIndex();
	int Fg = m_pLayers->GetForegroundLayerIndex();
	int Dd = m_pLayers->GetDoodadsLayerIndex();

	int Count = maximum(1, pTiles->Size() / 1100);
	if(FacilityType == LC_FACILITY_MINES)
		Count += 2;
	else if(FacilityType == LC_FACILITY_RESEARCH)
		Count += 1;
	else if(FacilityType == LC_FACILITY_MANSION || FacilityType == LC_FACILITY_WAREHOUSE)
		Count = maximum(1, Count - 1);

	int Placed = 0;
	for(int n = 0; n < Count * 10 && Placed < Count; n++)
	{
		ivec2 Pos = pTiles->GetOpenArea();
		if(Pos.x == 0)
			continue;
		if(Pos.x >= BayPos.x - 2 && Pos.x < BayPos.x + BayW + 2 &&
			Pos.y >= BayPos.y - 2 && Pos.y < BayPos.y + BayH + 2)
			continue;
		if(distance(vec2((float)Pos.x, (float)Pos.y), vec2((float)ShipPos.x, (float)ShipPos.y)) < 16.f)
			continue;
		if(Pos.x < 3 || Pos.y < 3 || Pos.x >= w - 3 || Pos.y >= h - 3)
			continue;

		bool Valid = pTiles->Get(Pos.x, Pos.y) == 0 && !pTiles->Used(Pos.x, Pos.y);
		if(Valid && !pTiles->Get(Pos.x, Pos.y + 1) && !pTiles->Get(Pos.x, Pos.y + 1, CGenLayer::FGOBJECTS))
			Valid = false;
		if(!Valid)
			continue;

		ModifTile(Pos, Game, ENTITY_OFFSET + ENTITY_TURRET, 0, 0);
		ModifTile(Pos, Dd, LC_TILE_DOODAD_WARN, 0, 0);
		ModifTile(Pos, Fg, LC_TILE_FG_SHOCK, 0, 0);
		Placed++;
	}
}

static bool CanFitOpenRect(CGenLayer *pTiles, ivec2 Origin, int Rw, int Rh)
{
	for(int y = 0; y < Rh; y++)
		for(int x = 0; x < Rw; x++)
		{
			ivec2 P = Origin + ivec2(x, y);
			if(P.x < 2 || P.y < 2 || P.x >= pTiles->Width() - 2 || P.y >= pTiles->Height() - 2)
				return false;
			if(pTiles->Get(P.x, P.y) != 0)
				return false;
		}
	return true;
}

void CMapGen::PlaceFixedRooms(CGenLayer *pTiles, int FacilityType, ivec2 ShipPos, ivec2 BayPos, int BayW, int BayH)
{
	int Game = m_pLayers->GetGameLayerIndex();
	int Fg = m_pLayers->GetForegroundLayerIndex();
	int Dd = m_pLayers->GetDoodadsLayerIndex();

	struct SRoomSpec
	{
		int m_Type;
		int m_W;
		int m_H;
	};
	SRoomSpec aSpecs[3];
	int NumSpecs = 0;
	aSpecs[NumSpecs++] = SRoomSpec{LC_ROOM_BATTERY, 8, 6};
	if(FacilityType == LC_FACILITY_MINES || FacilityType == LC_FACILITY_RESEARCH)
		aSpecs[NumSpecs++] = SRoomSpec{LC_ROOM_FUSE, 6, 6};
	if(FacilityType != LC_FACILITY_WAREHOUSE)
		aSpecs[NumSpecs++] = SRoomSpec{LC_ROOM_GENERATOR, 8, 6};

	int RoomsToPlace = 1 + MapGenRand() % 2;
	if(FacilityType == LC_FACILITY_MINES)
		RoomsToPlace = 2 + MapGenRand() % 2;

	for(int r = 0; r < RoomsToPlace && r < NumSpecs; r++)
	{
		const SRoomSpec &Spec = aSpecs[r];
		bool Placed = false;
		for(int attempt = 0; attempt < 400 && !Placed; attempt++)
		{
			ivec2 Origin = pTiles->GetOpenArea();
			if(Origin.x == 0)
				continue;
			if(distance(vec2((float)Origin.x, (float)Origin.y), vec2((float)ShipPos.x, (float)ShipPos.y)) < 22.f)
				continue;
			if(Origin.x >= BayPos.x - 3 && Origin.x < BayPos.x + BayW + Spec.m_W &&
				Origin.y >= BayPos.y - 3 && Origin.y < BayPos.y + BayH + 3)
				continue;
			if(!CanFitOpenRect(pTiles, Origin, Spec.m_W, Spec.m_H))
				continue;

			for(int y = 0; y < Spec.m_H; y++)
				for(int x = 0; x < Spec.m_W; x++)
					pTiles->Set(0, Origin.x + x, Origin.y + y);

			ivec2 Center = Origin + ivec2(Spec.m_W / 2, Spec.m_H / 2);
			ModifTile(Center, Game, TILE_AIR, 0, Spec.m_Type);
			ModifTile(Center + ivec2(-1, 0), Game, TILE_AIR, 0, Spec.m_Type);

			for(int x = 0; x < Spec.m_W; x++)
			{
				ModifTile(Origin + ivec2(x, 0), Fg, LC_TILE_FG_ROOM, 0, 0);
				ModifTile(Origin + ivec2(x, Spec.m_H - 1), Fg, LC_TILE_FG_ROOM, 0, 0);
			}
			for(int y = 1; y < Spec.m_H - 1; y++)
			{
				ModifTile(Origin + ivec2(0, y), Fg, LC_TILE_FG_ROOM, 0, 0);
				ModifTile(Origin + ivec2(Spec.m_W - 1, y), Fg, LC_TILE_FG_ROOM, 0, 0);
			}

			if(Spec.m_Type == LC_ROOM_BATTERY)
			{
				for(int y = 2; y < Spec.m_H - 2; y += 2)
					for(int x = 2; x < Spec.m_W - 2; x += 2)
					{
						ivec2 P = Origin + ivec2(x, y);
						ModifTile(P, Dd, LC_TILE_FG_BATTERY, 0, 0);
						ModifTile(P, Game, TILE_AIR, 0, LC_HAZARD_SHOCK);
						ModifTile(P, Fg, LC_TILE_FG_SHOCK, 0, 0);
					}
				ModifTile(Center, Dd, LC_TILE_DOODAD_SHOCK, 0, 0);
			}
			else if(Spec.m_Type == LC_ROOM_FUSE)
			{
				for(int i = 0; i < 4; i++)
				{
					ivec2 P = Origin + ivec2(2 + (i % 2) * (Spec.m_W - 4), 2 + (i / 2) * (Spec.m_H - 4));
					ModifTile(P, Game, TILE_AIR, 0, LC_HAZARD_MINE);
					ModifTile(P, Dd, LC_TILE_DOODAD_MINE, 0, 0);
					ModifTile(P, Fg, LC_TILE_FG_MINE, 0, 0);
				}
			}
			else if(Spec.m_Type == LC_ROOM_GENERATOR)
			{
				for(int x = 2; x < Spec.m_W - 2; x++)
				{
					ivec2 P = Origin + ivec2(x, Spec.m_H / 2);
					ModifTile(P, Game, TILE_AIR, 0, LC_HAZARD_GAS);
					ModifTile(P, Dd, LC_TILE_DOODAD_GAS, 0, 0);
					ModifTile(P, Fg, LC_TILE_FG_GAS, 0, 0);
				}
				ModifTile(Center, Dd, LC_TILE_DOODAD_WARN, 0, 0);
			}

			for(int b = 0; b < 2; b++)
			{
				ivec2 Bonus = Origin + ivec2(2 + b * 2, Spec.m_H / 2);
				if(pTiles->Get(Bonus.x, Bonus.y) == 0)
					ModifTile(Bonus, Game, ENTITY_OFFSET + (MapGenRand() % 2 == 0 ? ENTITY_SCRAP_L2 : ENTITY_SCRAP_L3), 0, 0);
			}

			Placed = true;
		}
	}
}

void CMapGen::FinalizeShipLanding(ivec2 BayPos, int BayW, int BayH, ivec2 ShipPos)
{
	int Game = m_pLayers->GetGameLayerIndex();
	int Fg = m_pLayers->GetForegroundLayerIndex();
	int Dd = m_pLayers->GetDoodadsLayerIndex();

	for(int dy = -1; dy <= BayH; dy++)
	{
		for(int dx = -1; dx <= BayW; dx++)
		{
			ivec2 T = BayPos + ivec2(dx, dy);
			ModifTile(T, Fg, 0, 0);
			ModifTile(T, Dd, 0, 0);
		}
	}

	for(int dy = 0; dy < BayH; dy++)
	{
		for(int dx = 0; dx < BayW; dx++)
		{
			ivec2 T = BayPos + ivec2(dx, dy);
			if(dy >= BayH - 2)
				ModifTile(T, Game, TILE_SOLID, 0);
			else
				ModifTile(T, Game, TILE_AIR, 0);
		}
	}

	ModifTile(ShipPos, Game, TILE_AIR, 0);
	for(int dx = -2; dx <= 2; dx += 2)
		ModifTile(ShipPos + ivec2(dx, -1), Game, TILE_AIR, 0);
}

void CMapGen::BeginGenerateLevel()
{
	m_GenW = m_pLayers->GameLayer()->m_Width;
	m_GenH = m_pLayers->GameLayer()->m_Height;

	if(m_GenW < 10 || m_GenH < 10)
	{
		m_GenSubStep = 2;
		return;
	}

	m_pGenTiles = new CGenLayer(m_GenW, m_GenH);
	m_GenFacility = LcGetFacilityType(m_GenParams.m_GcMoon);
	m_GenGridLayout = LcFacilityUsesGridLayout((ELcFacilityType)m_GenFacility);

	int Margin = 5 - m_GenParams.m_MapGenLevel;
	if(Margin < 2)
		Margin = 2;
	if(Margin > 4)
		Margin = 4;
	if(m_GenFacility == LC_FACILITY_MANSION || m_GenFacility == LC_FACILITY_WAREHOUSE || m_GenFacility == LC_FACILITY_RESEARCH)
		Margin = Margin > 2 ? Margin - 1 : 2;
	if(m_GenFacility == LC_FACILITY_MINES)
		Margin = Margin < 4 ? Margin + 1 : 4;

	m_pGenRoom = new CRoom(Margin, Margin, m_GenW - Margin * 2, m_GenH - Margin * 2, m_GenFacility);
	m_pGenMaze = new CMaze(m_GenW, m_GenH, m_GenFacility);
	m_GenLevel = m_GenParams.m_MapGenLevel;

	m_GenBayW = 14;
	m_GenBayH = 10;
	if(m_GenFacility == LC_FACILITY_MANSION || m_GenFacility == LC_FACILITY_WAREHOUSE)
	{
		m_GenBayW = 18;
		m_GenBayH = 12;
	}
	else if(m_GenFacility == LC_FACILITY_RESEARCH)
	{
		m_GenBayW = 16;
		m_GenBayH = 11;
	}
	else if(m_GenFacility == LC_FACILITY_MINES)
	{
		m_GenBayW = 10;
		m_GenBayH = 8;
	}

	m_GenBayPos = ivec2(Margin + 3, m_GenH / 2 - m_GenBayH / 2);
	if(m_GenGridLayout)
		m_GenBayPos.y = ((m_GenH / 2 - m_GenBayH / 2) / 4) * 4;
	if(m_GenBayPos.y < Margin + 2)
		m_GenBayPos.y = Margin + 2;
	m_GenShipPos = ivec2(m_GenBayPos.x + m_GenBayW / 2, m_GenBayPos.y + m_GenBayH - 3);

	m_pGenMaze->OpenRect(m_GenBayPos.x, m_GenBayPos.y, m_GenBayW, m_GenBayH);
	vec2 BayExit((float)(m_GenBayPos.x + m_GenBayW - 1), (float)(m_GenBayPos.y + m_GenBayH / 2));
	vec2 MazeCore((float)((m_GenW * 2 / 5) / 4 * 4), (float)((m_GenH * 9 / 20) / 4 * 4));
	if(!m_GenGridLayout)
		MazeCore = vec2(m_GenW * (0.36f + MapGenRandomFloat() * 0.04f), m_GenH * (0.48f + MapGenRandomFloat() * 0.04f));
	m_pGenMaze->ConnectToFacility(BayExit, MazeCore);
	m_pGenMaze->OpenRooms(m_pGenRoom);
	m_pGenRoom->Generate(m_pGenTiles);
	m_pGenMaze->Carve(m_pGenTiles);
	m_pGenRoom->Fill(m_pGenTiles, 0, m_GenBayPos.x, m_GenBayPos.y, m_GenBayW, m_GenBayH);
	m_pGenRoom->Fill(m_pGenTiles, 0, m_GenBayPos.x + m_GenBayW - 1, m_GenBayPos.y + m_GenBayH / 2 - 1, 2, 3);

	m_GenCorridorY = 3;
	m_GenSubStep = 1;
}

void CMapGen::WidenCorridors(CGenLayer *pTiles, CRoom *pRoom)
{
	if(!pTiles || !pRoom)
		return;

	const int w = pTiles->Width();
	const int h = pTiles->Height();
	for(int y = 3; y < h - 4; y++)
		for(int x = 3; x < w - 4; x++)
		{
			// Punch single-tile walls between open areas.
			if(!pTiles->Get(x - 1, y) && pTiles->Get(x, y) && !pTiles->Get(x + 1, y))
				pRoom->Fill(pTiles, 0, x, y, 1, 1);
			if(!pTiles->Get(x, y - 1) && pTiles->Get(x, y) && !pTiles->Get(x, y + 1))
				pRoom->Fill(pTiles, 0, x, y, 1, 1);

			// Widen 2-tile choke points for tee-sized passage (grid/research maps).
			if(!pTiles->Get(x - 1, y) && pTiles->Get(x, y) && pTiles->Get(x + 1, y) && !pTiles->Get(x + 2, y))
				pRoom->Fill(pTiles, 0, x, y, 2, 1);
			if(!pTiles->Get(x, y - 1) && pTiles->Get(x, y) && pTiles->Get(x, y + 1) && !pTiles->Get(x, y + 2))
				pRoom->Fill(pTiles, 0, x, y, 1, 2);
		}
}

void CMapGen::PostProcessGenLayout(CGenLayer *pTiles, ivec2 &BayPos, int BayW, int BayH, ivec2 &ShipPos)
{
	if(!pTiles)
		return;

	const int W = m_GenW;
	const int H = m_GenH;
	const ivec2 OldBay = BayPos;

	EnforceGenBorder(pTiles, W, H, MAPGEN_BORDER);
	RemoveDisconnectedGenAir(pTiles, W, H);
	FindBestGenShipBay(pTiles, W, H, BayW, BayH, OldBay, BayPos, ShipPos);
	EnforceGenBorder(pTiles, W, H, MAPGEN_BORDER);

	m_GenBayPos = BayPos;
	m_GenShipPos = ShipPos;
}

int CMapGen::GetGameTileIndex(int x, int y) const
{
	if(x < 0 || y < 0 || x >= m_GenW || y >= m_GenH)
		return TILE_SOLID;

	const int Index = y * m_GenW + x;
	if(m_pStaging)
		return m_pStaging->m_pGame[Index].m_Index;

	CTile *pTiles = static_cast<CTile *>(m_pLayers->Map()->GetData(m_pLayers->GameLayer()->m_Data));
	return pTiles[Index].m_Index;
}

void CMapGen::WallGameCell(int x, int y, CGenLayer *pTiles)
{
	ModifTile(ivec2(x, y), m_pLayers->GetGameLayerIndex(), TILE_SOLID, 0, 0);
	ModifTile(ivec2(x, y), m_pLayers->GetForegroundLayerIndex(), 0, 0, 0);
	ModifTile(ivec2(x, y), m_pLayers->GetDoodadsLayerIndex(), 0, 0, 0);
	if(pTiles)
		pTiles->Set(1, x, y);
}

void CMapGen::EnforceGameBorder(int Thickness, CGenLayer *pTiles)
{
	const int W = m_GenW;
	const int H = m_GenH;
	for(int t = 0; t < Thickness; t++)
	{
		for(int x = 0; x < W; x++)
		{
			WallGameCell(x, t, pTiles);
			WallGameCell(x, H - 1 - t, pTiles);
		}
		for(int y = 0; y < H; y++)
		{
			WallGameCell(t, y, pTiles);
			WallGameCell(W - 1 - t, y, pTiles);
		}
	}
}

void CMapGen::FixGameConnectivity(CGenLayer *pTiles, ivec2 &BayPos, int BayW, int BayH, ivec2 &ShipPos)
{
	const int W = m_GenW;
	const int H = m_GenH;
	int *pComp = new int[W * H];
	for(int i = 0; i < W * H; i++)
		pComp[i] = -1;

	int NumComps = 0;
	int MainComp = -1;
	int MainSize = 0;

	for(int y = MAPGEN_BORDER; y < H - MAPGEN_BORDER; y++)
	{
		for(int x = MAPGEN_BORDER; x < W - MAPGEN_BORDER; x++)
		{
			const int Index = x + y * W;
			if(!IsWalkableGameIndex(GetGameTileIndex(x, y)) || pComp[Index] >= 0)
				continue;

			std::queue<ivec2> Queue;
			Queue.push(ivec2(x, y));
			pComp[Index] = NumComps;
			int Size = 0;

			while(!Queue.empty())
			{
				ivec2 Pos = Queue.front();
				Queue.pop();
				Size++;

				const ivec2 Dirs[] = {ivec2(1, 0), ivec2(-1, 0), ivec2(0, 1), ivec2(0, -1)};
				for(unsigned d = 0; d < sizeof(Dirs) / sizeof(Dirs[0]); d++)
				{
					ivec2 Next = Pos + Dirs[d];
					if(Next.x < MAPGEN_BORDER || Next.y < MAPGEN_BORDER || Next.x >= W - MAPGEN_BORDER || Next.y >= H - MAPGEN_BORDER)
						continue;
					const int NextIndex = Next.x + Next.y * W;
					if(!IsWalkableGameIndex(GetGameTileIndex(Next.x, Next.y)) || pComp[NextIndex] >= 0)
						continue;
					pComp[NextIndex] = NumComps;
					Queue.push(Next);
				}
			}

			if(Size > MainSize)
			{
				MainSize = Size;
				MainComp = NumComps;
			}
			NumComps++;
		}
	}

	if(MainComp >= 0)
	{
		for(int y = MAPGEN_BORDER; y < H - MAPGEN_BORDER; y++)
		{
			for(int x = MAPGEN_BORDER; x < W - MAPGEN_BORDER; x++)
			{
				const int Index = x + y * W;
				if(IsWalkableGameIndex(GetGameTileIndex(x, y)) && pComp[Index] != MainComp)
					WallGameCell(x, y, pTiles);
			}
		}
	}

	const int ShipIndex = ShipPos.x + ShipPos.y * W;
	const bool ShipInMain = MainComp >= 0 && ShipIndex >= 0 && ShipIndex < W * H &&
		IsWalkableGameIndex(GetGameTileIndex(ShipPos.x, ShipPos.y)) && pComp[ShipIndex] == MainComp;
	if(!ShipInMain)
	{
		const ivec2 OldBay = BayPos;
		for(int dy = -1; dy <= BayH; dy++)
			for(int dx = -1; dx <= BayW; dx++)
				WallGameCell(OldBay.x + dx, OldBay.y + dy, pTiles);
		FindBestGenShipBay(pTiles, W, H, BayW, BayH, OldBay, BayPos, ShipPos);
		m_GenBayPos = BayPos;
		m_GenShipPos = ShipPos;
	}

	delete[] pComp;
}

void CMapGen::FinishGenerateLevel()
{
	if(!m_pGenTiles)
		return;

	CGenLayer *pTiles = m_pGenTiles;
	CRoom *pRoom = m_pGenRoom;
	const int w = m_GenW;
	const int h = m_GenH;
	const int Facility = m_GenFacility;
	const bool GridLayout = m_GenGridLayout;
	ivec2 BayPos = m_GenBayPos;
	ivec2 ShipPos = m_GenShipPos;
	const int BayW = m_GenBayW;
	const int BayH = m_GenBayH;
	const int Level = m_GenLevel;
	const SLcMapgenThemeProfile *pTheme = LcGetMapgenThemeProfile(g_Config.m_SvMapgenTheme);

	WidenCorridors(pTiles, pRoom);

	if(GridLayout)
		pTiles->GenerateSlopesLight();
	else
		pTiles->GenerateSlopes();
	pTiles->RemoveSingles();
	PostProcessGenLayout(pTiles, BayPos, BayW, BayH, ShipPos);

	dbg_msg("mapgen", "rooms generated, map size: %d, ship bay %dx%d at %d,%d", pTiles->Size(), BayW, BayH, BayPos.x, BayPos.y);

	int n = pTiles->Size() / 500;
	pTiles->GenerateBackground();
	pTiles->GenerateMoreBackground();
	if(pTheme && pTheme->m_GeneratePlatforms)
	{
		if(pTheme->m_AirPlatformMul > 0.01f)
			n = maximum(1, (int)(n * pTheme->m_AirPlatformMul));
		if(n > 1)
			pTiles->GenerateAirPlatforms(n / 2 + MapGenRand() % (n / 2));
		else
			pTiles->GenerateAirPlatforms(n);
	}

	dbg_msg("mapgen", "Proceed tiles");
	ProceedTheme(pTiles, g_Config.m_SvMapgenTheme);

	if(pTheme && pTheme->m_GeneratePlatforms)
	{
		if(GridLayout)
		{
			int BoxCount = 4 + MapGenRand() % 4;
			BoxCount = maximum(1, (int)(BoxCount * pTheme->m_BoxMul));
			pTiles->GenerateBoxes(BoxCount);
		}
		else
			pTiles->GenerateBoxes();
		pTiles->GeneratePlatforms();
		pTiles->GenerateFences();
	}

	for(int x = 0; x < w; x++)
		for(int y = 0; y < h; y++)
		{
			int i = pTiles->Get(x, y);
			if(i > 0)
			{
				int f = pTiles->GetFlags(x, y);
				ModifTile(ivec2(x, y), m_pLayers->GetForegroundLayerIndex(), i, f);
				ModifTile(ivec2(x, y), m_pLayers->GetGameLayerIndex(), TILE_SOLID);
			}
		}

	for(int x = 0; x < w; x++)
		for(int y = 0; y < h; y++)
		{
			int i = pTiles->Get(x, y, CGenLayer::FGOBJECTS);
			if(i > 0)
			{
				int f = pTiles->GetFlags(x, y, CGenLayer::FGOBJECTS);
				ModifTile(ivec2(x, y), m_pLayers->GetForegroundLayerIndex(), i, f);
				if(i >= 14 * 16 + 1 && i <= 14 * 16 + 3)
					ModifTile(ivec2(x, y), m_pLayers->GetGameLayerIndex(), TILE_AIR);
				else
					ModifTile(ivec2(x, y), m_pLayers->GetGameLayerIndex(), 1);
			}
		}

	for(int x = 0; x < w; x++)
		for(int y = 0; y < h; y++)
		{
			int i = pTiles->Get(x, y, CGenLayer::BACKGROUND);
			if(i > 0)
				ModifTile(ivec2(x, y), m_pLayers->GetBackgroundLayerIndex(), i, pTiles->GetFlags(x, y, CGenLayer::BACKGROUND));
		}

	for(int x = 0; x < w; x++)
		for(int y = 0; y < h; y++)
		{
			int i = pTiles->Get(x, y, CGenLayer::DOODADS);
			if(i > 0)
				ModifTile(ivec2(x, y), m_pLayers->GetDoodadsLayerIndex(), i, pTiles->GetFlags(x, y, CGenLayer::DOODADS));
		}

	dbg_msg("mapgen", "Scanning level");
	pTiles->Scan();

	PlaceHazards(pTiles, Facility, ShipPos, BayPos, BayW, BayH);
	PlaceTurrets(pTiles, Facility, ShipPos, BayPos, BayW, BayH);
	PlaceFixedRooms(pTiles, Facility, ShipPos, BayPos, BayW, BayH);
	EnforceGameBorder(MAPGEN_BORDER, pTiles);
	FixGameConnectivity(pTiles, BayPos, BayW, BayH, ShipPos);
	FinalizeShipLanding(BayPos, BayW, BayH, ShipPos);
	ModifTile(ShipPos + ivec2(-2, -1), m_pLayers->GetGameLayerIndex(), ENTITY_OFFSET + ENTITY_SPAWN);
	ModifTile(ShipPos + ivec2(0, -1), m_pLayers->GetGameLayerIndex(), ENTITY_OFFSET + ENTITY_SPAWN);
	ModifTile(ShipPos + ivec2(+2, -1), m_pLayers->GetGameLayerIndex(), ENTITY_OFFSET + ENTITY_SPAWN);
	ModifTile(ShipPos, m_pLayers->GetGameLayerIndex(), ENTITY_OFFSET + ENTITY_SHIP);

	pTiles->Scan();

	for(int i = 0; i < 5; i++)
		GenerateEnemySpawn(pTiles);

	for(int i = 0; i < BalanceScrapL1Count(m_GenParams.m_GcRounds); i++)
		GenerateWeapon(pTiles, ENTITY_SCRAP_L1);
	for(int i = 0; i < BalanceScrapL2Count(m_GenParams.m_GcRounds); i++)
		GenerateWeapon(pTiles, ENTITY_SCRAP_L2);
	for(int i = 0; i < BalanceScrapL3Count(m_GenParams.m_GcRounds); i++)
		GenerateWeapon(pTiles, ENTITY_SCRAP_L3);
	for(int i = 0; i < min(Level, 10); i++)
		GenerateEnemySpawn(pTiles);

	if(pRoom)
		delete pRoom;
	if(pTiles)
		delete pTiles;
	if(m_pGenMaze)
		delete m_pGenMaze;

	m_pGenRoom = 0;
	m_pGenTiles = 0;
	m_pGenMaze = 0;
	m_GenSubStep = 0;
	dbg_msg("mapgen", "Level generated");
}

bool CMapGen::StepGenerateLevel()
{
	if(m_GenSubStep == 2)
	{
		FinishGenerateLevel();
		return true;
	}

	if(!m_pGenTiles || !m_pGenRoom)
	{
		m_GenSubStep = 2;
		return false;
	}

	if(m_GenSubStep != 1)
		return false;

	if(!(m_GenGridLayout || m_GenFacility == LC_FACILITY_FACTORY))
	{
		m_GenSubStep = 2;
		return false;
	}

	const int w = m_GenW;
	const int h = m_GenH;
	const int yEnd = minimum(m_GenCorridorY + 24, h - 4);

	for(int y = m_GenCorridorY; y < yEnd; y++)
		for(int x = 3; x < w - 4; x++)
		{
			if(!m_pGenTiles->Get(x - 1, y) && m_pGenTiles->Get(x, y) && m_pGenTiles->Get(x + 1, y) && !m_pGenTiles->Get(x + 2, y))
				m_pGenRoom->Fill(m_pGenTiles, 0, x, y, 2, 1);
			if(!m_pGenTiles->Get(x, y - 1) && m_pGenTiles->Get(x, y) && m_pGenTiles->Get(x, y + 1) && !m_pGenTiles->Get(x, y + 2))
				m_pGenRoom->Fill(m_pGenTiles, 0, x, y, 1, 2);
		}

	m_GenCorridorY = yEnd;
	if(m_GenCorridorY >= h - 4)
		m_GenSubStep = 2;
	return false;
}

void CMapGen::GenerateLevel()
{
	int64 Start = time_get();
	BeginGenerateLevel();
	while(!StepGenerateLevel())
		;
	dbg_msg("mapgen", "level generated in %.5fs", (float)(time_get() - Start) / time_freq());
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
	ProceedTheme(pBaseTiles, g_Config.m_SvMapgenTheme);

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

inline void CMapGen::ModifTile(ivec2 Pos, int Layer, int Tile, int Flags, int Reserved)
{
	if(m_pStaging)
	{
		if(Pos.x < 0 || Pos.y < 0 || Pos.x >= m_pStaging->m_W || Pos.y >= m_pStaging->m_H)
			return;

		const int Index = Pos.y * m_pStaging->m_W + Pos.x;
		CTile *pTile = 0;
		if(Layer == m_pLayers->GetGameLayerIndex())
			pTile = &m_pStaging->m_pGame[Index];
		else if(Layer == m_pLayers->GetBackgroundLayerIndex())
			pTile = &m_pStaging->m_pBackground[Index];
		else if(Layer == m_pLayers->GetDoodadsLayerIndex())
			pTile = &m_pStaging->m_pDoodads[Index];
		else if(Layer == m_pLayers->GetForegroundLayerIndex())
			pTile = &m_pStaging->m_pForeground[Index];

		if(!pTile)
			return;

		pTile->m_Index = Tile;
		pTile->m_Flags = Flags;
		pTile->m_Reserved = Reserved;
		return;
	}

	m_pCollision->ModifTile(Pos, m_pLayers->GetGameGroupIndex(), Layer, Tile, Flags, Reserved);
}

void CMapGen::Proceed(CGenLayer *pTiles, int ConfigID)
{
	if(!m_FileLoaded || ConfigID < 0 || ConfigID >= m_lConfigs.size())
		return;

	CConfiguration *pConf = &m_lConfigs[ConfigID];

	if(!pConf->m_aIndexRules.size())
		return;

	if(pConf->m_aRunOffsets.size() == 0)
		pConf->m_aRunOffsets.add(0);

	int BaseTile = 1;

	for(int i = 0; i < pConf->m_aIndexRules.size(); ++i)
	{
		if(pConf->m_aIndexRules[i].m_BaseTile)
		{
			BaseTile = pConf->m_aIndexRules[i].m_ID;
			break;
		}
	}

	int Width = m_pLayers->GameLayer()->m_Width;
	int Height = m_pLayers->GameLayer()->m_Height;
	int MaxIndex = Width * Height;

	for(int Run = 0; Run < pConf->m_aRunOffsets.size(); Run++)
	{
		const int RuleStart = pConf->m_aRunOffsets[Run];
		const int RuleEnd = (Run + 1 < pConf->m_aRunOffsets.size()) ? pConf->m_aRunOffsets[Run + 1] : pConf->m_aIndexRules.size();

		for(int l = 0; l < 3; l++)
		{
			for(int y = 0; y < Height; y++)
			{
				for(int x = 0; x < Width; x++)
				{
					if(pTiles->Get(x, y, l) == 0)
						continue;

					if(Run == 0)
						pTiles->Set(BaseTile, x, y, 0, l);

					if(y == 0 || y == Height - 1 || x == 0 || x == Width - 1)
						continue;

					for(int i = RuleStart; i < RuleEnd; ++i)
					{
						if(pConf->m_aIndexRules[i].m_BaseTile)
							continue;

						bool RespectRules = true;
						for(int j = 0; j < pConf->m_aIndexRules[i].m_aRules.size() && RespectRules; ++j)
						{
							const CPosRule *pRule = &pConf->m_aIndexRules[i].m_aRules[j];
							const int CheckIndex = (y + pRule->m_Y) * Width + (x + pRule->m_X);
							int CheckTile = -1;

							if(CheckIndex < 0 || CheckIndex >= MaxIndex)
								CheckTile = -1;
							else
								CheckTile = pTiles->GetByIndex(CheckIndex, l);

							if(!PosRuleMatches(*pRule, CheckTile))
								RespectRules = false;
						}

						if(RespectRules &&
							(pConf->m_aIndexRules[i].m_YDivisor < 2 || y % pConf->m_aIndexRules[i].m_YDivisor == pConf->m_aIndexRules[i].m_YRemainder) &&
							(pConf->m_aIndexRules[i].m_RandomValue <= 1 || (int)((float)MapGenRand() / 32768.0f * pConf->m_aIndexRules[i].m_RandomValue) == 1))
						{
							pTiles->Set(PickIndexRuleTile(pConf->m_aIndexRules[i]), x, y, pConf->m_aIndexRules[i].m_Flag, l);
						}
					}
				}
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