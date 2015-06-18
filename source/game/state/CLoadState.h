#pragma once

#include "game/IGameState.h"
#include "animation/CAnimationWorld.h"
#include "graphics/IScene.h"
#include "graphics/IGraphicsSystem.h"
#include "graphics/ICamera.h"
#include "resource/IResourceManager.h"
#include "game/CGameSystem.h"

/**
* \brief Load state.
*
* Just for showing off some effects.
*/
class CLoadState : public IGameState
{
public:
	CLoadState(const std::string& sceneFile, float transitionTime);
	~CLoadState();

	bool init(IGraphicsSystem* graphicsSystem, IInputProvider* inputProvider, IResourceManager* resourceManager);

	void onEnter();
	bool update(float dtime);
	void onExit();
	const std::string& getNextState() const;

private:
	float m_transitionTime = 0.f;
	float m_timeLeft = 0.f;
	std::string m_sceneFile;
	std::shared_ptr<ICamera> m_camera = nullptr;
	IScene* m_scene = nullptr;
	IGraphicsSystem* m_graphicsSystem = nullptr;
	IResourceManager* m_resourceManager = nullptr;
	CAnimationWorld m_animationWorld;
	std::string m_nextState; /**< Next state. */
	CGameSystem* m_gameSystem; /**< Game system. */
};