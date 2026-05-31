// Ninslash
#ifndef GAME_MAPGEN_H
#define GAME_MAPGEN_H

#include <engine/storage.h>
#include <game/layers.h>
#include <game/collision.h>
#include <game/mapitems.h>

#include <base/tl/array.h>

struct SMapGenParams
{
	int m_Seed;
	int m_GcMoon;
	int m_GcRounds;
	int m_MapGenLevel;
};

struct SMapGenStaging
{
	int m_W;
	int m_H;
	CTile *m_pGame;
	CTile *m_pBackground;
	CTile *m_pDoodads;
	CTile *m_pForeground;

	SMapGenStaging()
	{
		m_W = 0;
		m_H = 0;
		m_pGame = 0;
		m_pBackground = 0;
		m_pDoodads = 0;
		m_pForeground = 0;
	}

	bool Init(int W, int H);
	void Free();
};

enum EMapGenStep
{
	MAPGENSTEP_IDLE = 0,
	MAPGENSTEP_RESTORE_TEMPLATE,
	MAPGENSTEP_CLEAR,
	MAPGENSTEP_GENERATE,
	MAPGENSTEP_DONE,
};

class CMapGen
{
	IStorage *m_pStorage;
	IStorage *Storage() const { return m_pStorage; }
	
	class CLayers *m_pLayers;
	CCollision *m_pCollision;

	void GenerateLevel();
	
	void WriteLayers(class CGenLayer *pTiles);
	void WriteBackground(class CGenLayer *pTiles);
	void WriteBase(class CGenLayer *pTiles, int BaseNum, ivec2 Pos, float Size);
	
	void Mirror(class CGenLayer *pTiles);

	void GenerateEnemySpawn(class CGenLayer *pTiles);	
	void GenerateWeapon(class CGenLayer *pTiles, int Weapon);
	void PlaceHazards(class CGenLayer *pTiles, int FacilityType, ivec2 ShipPos, ivec2 BayPos, int BayW, int BayH);
	void PlaceTurrets(class CGenLayer *pTiles, int FacilityType, ivec2 ShipPos, ivec2 BayPos, int BayW, int BayH);
	void PlaceFixedRooms(class CGenLayer *pTiles, int FacilityType, ivec2 ShipPos, ivec2 BayPos, int BayW, int BayH);
	void FinalizeShipLanding(ivec2 BayPos, int BayW, int BayH, ivec2 ShipPos);

	int GetGameTileIndex(int x, int y) const;
	void WallGameCell(int x, int y, class CGenLayer *pTiles);
	void EnforceGameBorder(int Thickness, class CGenLayer *pTiles);
	void FixGameConnectivity(class CGenLayer *pTiles, ivec2 &BayPos, int BayW, int BayH, ivec2 &ShipPos);
	void PostProcessGenLayout(class CGenLayer *pTiles, ivec2 &BayPos, int BayW, int BayH, ivec2 &ShipPos);

	void ModifTile(ivec2 Pos, int Layer, int Tile, int Flags = 0, int Reserved = 0);

	bool RestoreTemplateFromFile(const char *pMapName);
	void ClearMapChunk(int TilesPerStep);

	// auto mapper
	struct CPosRule
	{
		int m_X;
		int m_Y;
		int m_Type;
		int m_aIndexValues[16];
		int m_NumIndexValues;

		enum
		{
			EMPTY=0,
			FULL,
			INDEX,
			NOTINDEX,
		};
	};

	struct CIndexRule
	{
		int m_ID;
		int m_aIDs[8];
		int m_NumIDs;
		array<CPosRule> m_aRules;
		int m_Flag;
		int m_RandomValue;
		int m_YDivisor;
		int m_YRemainder;
		bool m_BaseTile;
	};

	struct CConfiguration
	{
		array<CIndexRule> m_aIndexRules;
		array<int> m_aRunOffsets;
		char m_aName[128];
	};
	
	array<CConfiguration> m_lConfigs;
	bool m_FileLoaded;
	
	void Load(const char* pTileName);
	bool TryLoadRules(const char *pTileName);
	static void ParseIndexLine(const char *pLine, CIndexRule &Rule);
	static bool ParsePosRuleLine(const char *pLine, CPosRule &Rule);
	static bool PosRuleMatches(const CPosRule &Rule, int CheckTile);
	static int PickIndexRuleTile(const CIndexRule &Rule);
	void Proceed(class CGenLayer *pTiles, int ConfigID);
	void ProceedTheme(class CGenLayer *pTiles, const char *pThemeRules);
	int FindConfigId(const char *pThemeRules) const;

	int ConfigNamesNum() { return m_lConfigs.size(); }
	const char* GetConfigName(int Index);

	EMapGenStep m_Step;
	int m_ClearIndex;
	int m_LayerSize;
	int m_ApplyIndex;
	char m_aTemplateMap[128];

	SMapGenParams m_GenParams;
	SMapGenStaging *m_pStaging;

	class CGenLayer *m_pGenTiles;
	class CRoom *m_pGenRoom;
	class CMaze *m_pGenMaze;
	int m_GenSubStep;
	int m_GenCorridorY;
	int m_GenFacility;
	bool m_GenGridLayout;
	ivec2 m_GenBayPos;
	int m_GenBayW;
	int m_GenBayH;
	int m_GenLevel;
	int m_GenW;
	int m_GenH;
	ivec2 m_GenShipPos;

	void BeginGenerateLevel();
	bool StepGenerateLevel();
	void FinishGenerateLevel();
	void WidenCorridors(class CGenLayer *pTiles, class CRoom *pRoom);

	const bool IsLoaded() { return m_FileLoaded; }
	
public:
	CMapGen();
	~CMapGen();

	void FillMap();
	bool BeginFillMap(const char *pTemplateMap, const char *pThemeRules);
	bool StepFillMap();
	bool IsGenerating() const { return m_Step != MAPGENSTEP_IDLE && m_Step != MAPGENSTEP_DONE; }
	bool IsDone() const { return m_Step == MAPGENSTEP_DONE; }
	void ResetStep() { m_Step = MAPGENSTEP_IDLE; }

	void SetParams(const SMapGenParams &Params);
	void UseStaging(SMapGenStaging *pStaging);
	void ClearStaging();
	void ResetApplyIndex();
	bool ApplyStagingChunk(int TilesPerStep);
	void ApplyThemeTilesets(const char *pThemeRules);

	void Init(CLayers *pLayers, CCollision *pCollision, IStorage *pStorage);
};

#endif
