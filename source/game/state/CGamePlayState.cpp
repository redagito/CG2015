#include "CGamePlayState.h"

#include "graphics/IGraphicsSystem.h"

bool CGamePlayState::init(IGraphicsSystem* graphicsSystem)
{
	m_scene = graphicsSystem->createScene();
	return true;
}

void CGamePlayState::onEnter()
{
	// Set scene created by the state as active scene
	m_graphicsSystem->setActiveScene(m_scene);
}

bool CGamePlayState::update(float dtime)
{
	// Update game world
	getGameWorld().update(dtime);
	return true;
}

void CGamePlayState::onExit()
{
	// Cleanup?
}

const std::string& CGamePlayState::getNextState() const
{
	return "exit";
}