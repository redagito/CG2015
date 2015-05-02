#pragma once

#include "AGameState.h"

class IScene;

class CGamePlayState : public AGameState
{
public:
	bool init(IGraphicsSystem* graphicsSystem);
	void onEnter();
	bool update(float dtime);
	void onExit();
	const std::string& getNextState() const;

private:
	IGraphicsSystem* m_graphicsSystem = nullptr;
	IScene* m_scene = nullptr;
};