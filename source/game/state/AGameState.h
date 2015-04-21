#pragma once

#include "game/IGameState.h"
#include "game/CGameWorld.h"

/**
* \brief Abstract game state base class.
*
* Provides common functionality for all game states.
*/
class AGameState : public IGameState
{
public:
	virtual ~AGameState();

	CGameWorld& getGameWorld();
	const CGameWorld& getGameWorld() const;

	virtual bool init();
	virtual void onEnter();
	virtual bool update(float dtime);
	virtual void onExit();
	virtual const std::string& getNextState() const;

private:
	CGameWorld m_gameWorld;
};