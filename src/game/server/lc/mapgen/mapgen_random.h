#ifndef GAME_SERVER_MAPGEN_MAPGEN_RANDOM_H
#define GAME_SERVER_MAPGEN_MAPGEN_RANDOM_H

class CMapGenRandomScope
{
public:
	CMapGenRandomScope(unsigned Seed);
	~CMapGenRandomScope();
};

int MapGenRand();
float MapGenRandomFloat();

#endif
