#include "CTitleState.h"

bool CTitleState::update(float dtime)
{
	// Update game world
	getGameWorld().update(dtime);
	return true;
}