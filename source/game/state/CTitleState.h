#pragma once

#include "game/IGameState.h"
#include "animation/CAnimationWorld.h"
#include "graphics/IScene.h"
#include "graphics/IGraphicsSystem.h"
#include "graphics/ICamera.h"
#include "resource/IResourceManager.h"
#include "input/IInputProvider.h"
#include "game/CGameSystem.h"

/**
* \brief Title state, similar to demo state with static camera.
*/
class CTitleState : public IGameState
{
public:
	CTitleState(const std::string& sceneFile);
	~CTitleState();

	bool init(IGraphicsSystem* graphicsSystem, IInputProvider* inputProvider, IResourceManager* resourceManager);

	void onEnter();
	bool update(float dtime);
	void onExit();
	const std::string& getNextState() const;

private:
	float m_fadeInTime;
	std::string m_sceneFile;
	IInputProvider* m_inputProvider = nullptr;
	std::shared_ptr<ICamera> m_camera = nullptr;
	IScene* m_scene = nullptr;
	IGraphicsSystem* m_graphicsSystem = nullptr;
	IResourceManager* m_resourceManager = nullptr;
	CAnimationWorld m_animationWorld;
	std::string m_nextState; /**< Next state. */
	CGameSystem* m_gameSystem; /**< Game system. */
};