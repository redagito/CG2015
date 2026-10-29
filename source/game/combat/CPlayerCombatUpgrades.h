#pragma once
#include "PlayerUpgradeTuning.h"
#include "collision/DamageKind.h"
#include <cstdint>
#include <deque>
#include <glm/glm.hpp>
class CGameWorld;
class CHealthController;
class CCombatEffects;
class CPlayerCombatUpgrades
{
  public:
    CPlayerCombatUpgrades(const CGameWorld &world, CHealthController &health,
                          CCombatEffects *effects, const PlayerUpgradeTuning &tuning);
    void onHit(glm::vec3 position, float actualHullDamage, float splashRadius, float splashDamage,
               std::uint64_t primary);
    void onKill(glm::vec3 position, DamageKind kind = DamageKind::Direct);
    void update(float seconds = 0);

  private:
    struct Blast
    {
        glm::vec3 position;
        float damage, radius;
        std::uint64_t excluded;
        DamageKind kind;
    };
    void queue(Blast blast);
    void repair(float requested, float &allowance);
    const CGameWorld &m_world;
    CHealthController &m_health;
    CCombatEffects *m_effects;
    PlayerUpgradeTuning m_tuning;
    std::deque<Blast> m_blasts;
    float m_killHealing = 0, m_hitHealing = 0;
};
