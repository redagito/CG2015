#pragma once

#include "graphics/Graphics.h"

/**
* \brief Game engine class.
*/
class CEngine
{
public:
	bool init(int argc, const char** argv);

	int run();

private:
	CGraphicsSystem m_graphicsSystem;
};