#include "AGameState.h"

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

bool AGameState::init()
{
	return true;
}

void AGameState::onEnter()
{
	return;
}

bool AGameState::update(float dtime)
{
	return true;
}

void AGameState::onExit()
{
	return;
}

const std::string& AGameState::getNextState() const
{
	return "Error";
}