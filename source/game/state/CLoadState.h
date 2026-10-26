#pragma once

#include "game/IGameState.h"
#include "animation/CAnimationWorld.h"
#include "graphics/IScene.h"
#include "graphics/IGraphicsSystem.h"
#include "graphics/ICamera.h"
#include "resource/IResourceManager.h"
#include "game/CGameSystem.h"
#include "input/IInputProvider.h"

/**
* \brief Load state.
*
* Animates the loading scene while CPU preparation and render-thread uploads run.
*/
class CLoadState : public IGameState
{
public:
	CLoadState(const std::string& sceneFile, const std::string& manifest);
	~CLoadState();

	bool init(IGraphicsSystem* graphicsSystem, IInputProvider* inputProvider, IResourceManager* resourceManager);

	void onEnter();
	bool update(float dtime);
	void onExit();
	const std::string& getNextState() const;
    bool readyToContinue() const {return m_ready;}

private:
    std::string m_manifest;
    AsyncLoadId m_request=0;
    bool m_ready=false;
    bool m_enterWasDown=false;
    IInputProvider* m_inputProvider=nullptr;
	std::string m_sceneFile;
	std::shared_ptr<ICamera> m_camera = nullptr;
	IScene* m_scene = nullptr;
	IGraphicsSystem* m_graphicsSystem = nullptr;
	IResourceManager* m_resourceManager = nullptr;
	CAnimationWorld m_animationWorld;
	std::string m_nextState; /**< Next state. */
	CGameSystem* m_gameSystem; /**< Game system. */
};
