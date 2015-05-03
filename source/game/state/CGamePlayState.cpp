#include "CGamePlayState.h"

#include "graphics/IGraphicsSystem.h"
#include "graphics/IScene.h"

const std::string exitStr = "exit";

bool CGamePlayState::init(IGraphicsSystem* graphicsSystem, IInputProvider* inputProvider, IResourceManager* resourceManager)
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
	return exitStr;
}