#include "CGamePlayState.h"

bool CGamePlayState::update(float dtime)
{
	// Update game world
	getGameWorld().update(dtime);
	return true;
}