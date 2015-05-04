#include "AGameState.h"

const std::string errStr("Error");

AGameState::~AGameState()
{
	// Empty
}

CGameWorld& AGameState::getGameWorld()
{
	return m_gameWorld;
}

const CGameWorld& AGameState::getGameWorld() const
{
	return m_gameWorld;
}

bool AGameState::init(IGraphicsSystem* graphicsSystem, IInputProvider* inputProvider, IResourceManager* resourceManager)
{
	return true;
}

void AGameState::onEnter()
{
	return;
}

bool AGameState::update(float dtime)
{
	getGameWorld().update(dtime);
	return true;
}

void AGameState::onExit()
{
	return;
}

const std::string& AGameState::getNextState() const
{
	return errStr;
}