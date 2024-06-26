// Ninslash
#ifndef GAME_MAPGEN_H
#define GAME_MAPGEN_H

#include <engine/storage.h>
#include <game/layers.h>
#include <game/collision.h>

#include <base/tl/array.h>

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

	void ModifTile(ivec2 Pos, int Layer, int Tile, int Flags = 0);

	// auto mapper
	struct CPosRule
	{
		int m_X;
		int m_Y;
		int m_Value;
		bool m_IndexValue;

		enum
		{
			EMPTY=0,
			FULL
		};
	};

	struct CIndexRule
	{
		int m_ID;
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
		char m_aName[128];
	};
	
	array<CConfiguration> m_lConfigs;
	bool m_FileLoaded;
	
	void Load(const char* pTileName);
	void Proceed(class CGenLayer *pTiles, int ConfigID);

	int ConfigNamesNum() { return m_lConfigs.size(); }
	const char* GetConfigName(int Index);

	
	const bool IsLoaded() { return m_FileLoaded; }
	
public:
	CMapGen();
	~CMapGen();

	void FillMap();
	void Init(CLayers *pLayers, CCollision *pCollision, IStorage *pStorage);
};

#endif
