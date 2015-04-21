#pragma once

#include "AGameState.h"

class CStartupState : public AGameState
{
public:
	bool update(float dtime);
};