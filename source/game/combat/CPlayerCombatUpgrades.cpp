#include "CPlayerCombatUpgrades.h"
#include "collision/CCollidable.h"
#include "game/CGameWorld.h"
#include "game/control/CHealthController.h"
#include "game/effects/CCombatEffects.h"
#include <algorithm>
#include <cmath>
CPlayerCombatUpgrades::CPlayerCombatUpgrades(const CGameWorld &world, CHealthController &health,
                                             CCombatEffects *effects,
                                             const PlayerUpgradeTuning &tuning)
    : m_world(world), m_health(health), m_effects(effects), m_tuning(tuning),
      m_killHealing(tuning.killRepairPerSecond), m_hitHealing(tuning.lifestealPerSecond)
{
}
void CPlayerCombatUpgrades::queue(Blast blast)
{
    // Damage bursts are deferred, preventing recursive death callbacks and unbounded chain work.
    if (blast.damage > 0 && blast.radius > 0 && m_blasts.size() < 16384)
        m_blasts.push_back(blast);
}
void CPlayerCombatUpgrades::repair(float requested, float &allowance)
{
    const float amount =
        std::min({std::max(0.f, requested), allowance, m_health.maxHealth() - m_health.health()});
    if (amount <= 0 || m_health.health() <= 0)
        return;
    m_health.heal(amount);
    allowance -= amount;
}
void CPlayerCombatUpgrades::onHit(glm::vec3 position, float actualHullDamage, float radius,
                                  float splashDamage, std::uint64_t primary)
{
    repair(actualHullDamage * m_tuning.lifesteal, m_hitHealing);
    queue({position, splashDamage, radius, primary, DamageKind::Splash});
}
void CPlayerCombatUpgrades::onKill(glm::vec3 position, DamageKind kind)
{
    // Area kills still award points, but cannot produce repair or another generation of blasts.
    if (kind != DamageKind::Direct)
        return;
    repair(m_tuning.killRepair, m_killHealing);
    queue({position, m_tuning.chainDamage, m_tuning.chainRadius, 0, DamageKind::Chain});
}
void CPlayerCombatUpgrades::update(float seconds)
{
    if (!std::isfinite(seconds) || seconds < 0)
        return;
    const float dt = std::min(seconds, .25f);
    // Each source can bank at most one second of healing, regardless of kills or pellet count.
    m_killHealing =
        std::min(m_tuning.killRepairPerSecond, m_killHealing + dt * m_tuning.killRepairPerSecond);
    m_hitHealing =
        std::min(m_tuning.lifestealPerSecond, m_hitHealing + dt * m_tuning.lifestealPerSecond);
    unsigned processed = 0;
    while (!m_blasts.empty() && processed++ < 32)
    {
        const auto blast = m_blasts.front();
        m_blasts.pop_front();
        if (m_effects)
            m_effects->burst(blast.position, blast.radius);
        for (const auto &target : m_world.nearbyTargets(blast.position, blast.radius))
        {
            const auto *object = m_world.getObject(target.id);
            if (!object || object->isDead() || !object->hasCollidable())
                continue;
            auto *collider = object->getCollidable();
            if (collider->id() == blast.excluded)
                continue;
            const float distance = glm::distance(target.position, blast.position);
            const float falloff = 1 - .5f * std::clamp(distance / blast.radius, 0.f, 1.f);
            collider->receiveDamage(blast.damage * falloff, blast.position, blast.kind);
        }
    }
}
