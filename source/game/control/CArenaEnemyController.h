#pragma once
#include "game/GameObjectId.h"
#include "game/IController.h"
#include "game/arena/ArenaTuning.h"
#include "game/arena/EnemyMode.h"
#include <glm/gtc/quaternion.hpp>
#include <memory>
class CGameWorld;
class CWeaponController;
class CArenaEnemyController : public IController
{
  public:
    CArenaEnemyController(const CGameWorld &world, GameObjectId player, const ArenaTuning &arena,
                          std::shared_ptr<CWeaponController> weapon, float phase = 0);
    void attach(CGameObject *object) override;
    void detach() override;
    void update(float seconds) override;
    void setActive(bool active) override;
    void receiveMessage(Message message) override;
    EnemyMode mode() const;
    bool firing() const;

  private:
    const CGameWorld &m_world;
    GameObjectId m_player, m_self = noGameObject;
    ArenaTuning m_arena;
    std::shared_ptr<CWeaponController> m_weapon;
    CGameObject *m_object = nullptr;
    glm::quat m_orientation{1, 0, 0, 0};
    glm::vec3 m_escape{0};
    float m_phase, m_elapsed = 0, m_breakTimer = 0, m_reaction = .6f;
    EnemyMode m_mode = EnemyMode::Pursuit;
    bool m_active = true, m_firing = false;
};
