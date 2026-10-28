#pragma once
#include "game/IController.h"
#include <glm/glm.hpp>
class CCollisionSystem;
class CCombatEffects;
class CProjectileController : public IController
{
  public:
    CProjectileController(glm::vec3 velocity, float lifetime, CCollisionSystem &collisions,
                          unsigned group, CCombatEffects *effects = nullptr,
                          bool playerShot = false, glm::vec3 color = glm::vec3(.25f, .85f, 2));
    void attach(CGameObject *object) override;
    void detach() override;
    void update(float seconds) override;
    void setActive(bool active) override;
    void receiveMessage(Message message) override;

  private:
    CCombatEffects *m_effects;
    bool m_playerShot;
    glm::vec3 m_color;
    float m_untilTrail = 0;
    glm::vec3 m_velocity;
    float m_remaining;
    CCollisionSystem &m_collisions;
    unsigned m_group;
    CGameObject *m_object = nullptr;
    bool m_active = true;
};
