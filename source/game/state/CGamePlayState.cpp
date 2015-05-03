#include "CGamePlayState.h"

#include "graphics/IGraphicsSystem.h"
#include "graphics/IScene.h"

#include <glm/glm.hpp>

const std::string exitStr = "exit";

bool CGamePlayState::init(IGraphicsSystem* graphicsSystem, IInputProvider* inputProvider, IResourceManager* resourceManager)
{
	m_scene = graphicsSystem->createScene();
	CGameObject* o = new CGameObject;
	o->setRotation(glm::vec3(0.f));
	o->setPosition(glm::vec3(0.f));
	o->setScale(glm::vec3(1.f));
	getGameWorld().addObject(o);
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