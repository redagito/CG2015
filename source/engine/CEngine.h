#pragma once

#include "graphics/Graphics.h"
#include "core/CJobSystem.h"

/**
* \brief Game engine class.
*/
class CEngine
{
public:
	bool init(int argc, const char** argv);

	int run();

private:
	CJobSystem m_jobSystem;
	CGraphicsSystem m_graphicsSystem;
};