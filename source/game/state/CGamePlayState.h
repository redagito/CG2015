#pragma once

#include "AGameState.h"

#include "animation/CAnimationWorld.h"
#include "graphics/IScene.h"
#include "graphics/IGraphicsSystem.h"
#include "graphics/ICamera.h"

#include "resource/IResourceManager.h"
#include "input/IInputProvider.h"

#include "game/CGameObject.h"
#include "game/CGameWorld.h"

class CGamePlayState : public AGameState
{
public:
	CGamePlayState();
	~CGamePlayState();

	bool init(IGraphicsSystem* graphicsSystem, IInputProvider* inputProvider, IResourceManager* resourceManager);
	void onEnter();
	bool update(float dtime);
	void onExit();
	const std::string& getNextState() const;

private:
	IGraphicsSystem* m_graphicsSystem = nullptr;
	IScene* m_scene = nullptr;
	IResourceManager* m_resourceManager = nullptr;
	IInputProvider* m_inputProvider = nullptr; 
	std::shared_ptr<ICamera> m_camera = nullptr;

	CGameObject* m_player; /**< Player object. */
};