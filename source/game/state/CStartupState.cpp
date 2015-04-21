#include "CStartupState.h"

bool CStartupState::update(float dtime)
{
	// Update game world
	getGameWorld().update(dtime);
	return true;
}