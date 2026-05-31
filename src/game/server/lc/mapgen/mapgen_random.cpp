#include <stdlib.h>

#include "mapgen_random.h"

static thread_local unsigned s_MapGenRandState = 1;
static thread_local bool s_MapGenRandActive = false;

CMapGenRandomScope::CMapGenRandomScope(unsigned Seed)
{
	s_MapGenRandState = Seed ? Seed : 1;
	s_MapGenRandActive = true;
}

CMapGenRandomScope::~CMapGenRandomScope()
{
	s_MapGenRandActive = false;
}

int MapGenRand()
{
	if(!s_MapGenRandActive)
		return rand();

	s_MapGenRandState = s_MapGenRandState * 1103515245u + 12345u;
	return (int)((s_MapGenRandState >> 16) & 0x7fff);
}

float MapGenRandomFloat()
{
	return MapGenRand() / 32767.0f;
}
