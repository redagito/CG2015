#include "CProjectileController.h"
#include "collision/CCollidable.h"
#include "collision/CCollisionSystem.h"
#include "game/CGameObject.h"
#include "game/combat/CPlayerCombatUpgrades.h"
#include "game/effects/CCombatEffects.h"
#include <algorithm>
#include <cmath>
CProjectileController::CProjectileController(glm::vec3 velocity, float lifetime,
                                             CCollisionSystem &collisions, unsigned group,
                                             CCombatEffects *effects, bool playerShot,
                                             glm::vec3 color, const ProjectileModifiers &modifiers,
                                             CPlayerCombatUpgrades *upgrades)
    : m_modifiers(modifiers), m_upgrades(upgrades), m_effects(effects), m_playerShot(playerShot),
      m_color(color), m_velocity(velocity), m_remaining(lifetime), m_collisions(collisions),
      m_group(group)
{
}
void CProjectileController::attach(CGameObject *object)
{
    m_object = object;
    object->setVelocity(m_velocity);
}
void CProjectileController::detach()
{
    m_object = nullptr;
}
void CProjectileController::setActive(bool active)
{
    m_active = active;
}
void CProjectileController::receiveMessage(Message)
{
}
void CProjectileController::update(float seconds)
{
    if (!m_active || !m_object || m_object->isDeleteRequested() || !std::isfinite(seconds) ||
        seconds <= 0)
        return;
    const float duration = std::min(seconds, .25f);
    const float dt = std::min(duration, m_remaining);
    const auto start = m_object->getPosition(), end = start + m_velocity * dt;
    float stopFraction = 1;
    // A piercing shot sweeps the same segment repeatedly, excluding stable IDs already hit.
    while (m_hitTargets.size() < m_modifiers.targets)
    {
        const auto hit =
            m_collisions.trace(start, end, m_object->getCollidable()->getAABBox().getHalfWidths(),
                               m_group, dt / duration, m_hitTargets);
        if (!hit.entity)
            break;
        const float damage = m_object->getCollidable()->getDamage();
        const float actualHullDamage = hit.entity->receiveDamage(damage, start - m_velocity * .1f);
        m_hitTargets.push_back(hit.entity->id());
        const auto contact = glm::mix(start, end, hit.fraction) -
                             hit.normal * m_object->getCollidable()->getAABBox().getHalfWidths();
        if (m_effects)
            m_effects->impact(contact, hit.normal, m_color, m_playerShot);
        if (m_upgrades)
            m_upgrades->onHit(contact, actualHullDamage, m_modifiers.splashRadius,
                              m_modifiers.splashDamage, hit.entity->id());
        if (m_hitTargets.size() >= m_modifiers.targets)
        {
            stopFraction = hit.fraction;
            m_object->markDeleted();
            break;
        }
    }
    m_object->setPosition(glm::mix(start, end, stopFraction));
    if (m_effects)
    {
        const float travel = dt * stopFraction;
        while (m_untilTrail <= travel)
        {
            m_effects->trail(start + m_velocity * m_untilTrail, m_velocity, m_color);
            m_untilTrail += m_effects->trailInterval();
        }
        m_untilTrail -= travel;
    }
    m_remaining -= dt;
    if (m_remaining <= 0)
        m_object->markDeleted();
}
