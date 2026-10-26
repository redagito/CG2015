#pragma once

#include "animation/CAnimationWorld.h"
#include "game/CGameSystem.h"
#include "game/IGameState.h"
#include "graphics/ICamera.h"
#include "graphics/IGraphicsSystem.h"
#include "graphics/IScene.h"
#include "input/IInputProvider.h"
#include "resource/IResourceManager.h"

/**
 * \brief Victory state, similar to demo state with static camera.
 */
class CWinState : public IGameState
{
  public:
    CWinState(const std::string &sceneFile);
    ~CWinState();

    bool init(IGraphicsSystem *graphicsSystem, IInputProvider *inputProvider,
              IResourceManager *resourceManager);

    void onEnter();
    bool update(float dtime);
    void onExit();
    const std::string &getNextState() const;

  private:
    std::string m_sceneFile;
    IInputProvider *m_inputProvider = nullptr;
    std::shared_ptr<ICamera> m_camera = nullptr;
    IScene *m_scene = nullptr;
    IGraphicsSystem *m_graphicsSystem = nullptr;
    IResourceManager *m_resourceManager = nullptr;
    CAnimationWorld m_animationWorld;
};