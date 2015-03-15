#include "CEngine.h"
#include "log/Log.h"

bool CEngine::init(int argc, const char** argv)
{
	LOG_INFO("Initializing game engine.");
	m_graphicsSystem.init(800, 600, "Defenders of Cthedra - CG Project 2015");
	return true;
}

int CEngine::run()
{
	m_graphicsSystem.start();
	
	while (m_graphicsSystem.isRunning())
	{
		m_graphicsSystem.pollEvents();
	}

	return 0;
}