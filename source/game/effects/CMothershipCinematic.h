#pragma once
#include "graphics/SceneConfig.h"
#include "graphics/camera/CCamera.h"
#include "graphics/ui/UiDocument.h"
#include <glm/glm.hpp>
class IScene;
class CGameObject;
class GameAudio;

// Owns presentation only. Gameplay simulation remains frozen until the result screen.
class CMothershipCinematic
{
  public:
    CMothershipCinematic(IScene &scene, CGameObject &temple, glm::vec3 emitter, glm::vec3 impact,
                         GameAudio *audio);
    ~CMothershipCinematic();
    void update(float seconds, glm::vec2 viewport);
    CCamera &camera();
    UiDocument overlay(glm::vec2 viewport) const;
    bool finished() const;
    bool beamActive() const;

  private:
    void publishEffects();
    void updateCamera(glm::vec2 viewport);
    IScene &m_scene;
    CGameObject &m_temple;
    glm::vec3 m_emitter, m_impact;
    GameAudio *m_audio;
    CCamera m_camera;
    SceneObjectId m_sourceLight, m_impactLight;
    float m_time = 0;
    float m_effectScale = 1;
    bool m_fired = false, m_destroyed = false;
};
