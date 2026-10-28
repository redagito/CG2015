#include "CProjectileController.h"
#include "collision/CCollidable.h"
#include "collision/CCollisionSystem.h"
#include "game/CGameObject.h"
#include "game/effects/CCombatEffects.h"
#include <algorithm>
#include <cmath>
CProjectileController::CProjectileController(glm::vec3 velocity, float lifetime,
                                             CCollisionSystem &collisions, unsigned group,
                                             CCombatEffects *effects, bool playerShot,
                                             glm::vec3 color)
    : m_effects(effects), m_playerShot(playerShot), m_color(color), m_velocity(velocity),
      m_remaining(lifetime), m_collisions(collisions), m_group(group)
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
    // Sweep the whole step; overlap tests alone miss small ships at projectile speed.
    const auto hit = m_collisions.trace(
        start, end, m_object->getCollidable()->getAABBox().getHalfWidths(), m_group, dt / duration);
    m_object->setPosition(glm::mix(start, end, hit.fraction));
    if (m_effects)
    {
        const float travel = hit.entity ? dt * hit.fraction : dt;
        while (m_untilTrail <= travel)
        {
            m_effects->trail(start + m_velocity * m_untilTrail, m_velocity, m_color);
            m_untilTrail += m_effects->trailInterval();
        }
        m_untilTrail -= travel;
    }
    if (hit.entity)
    {
        hit.entity->receiveDamage(m_object->getCollidable()->getDamage(), start - m_velocity * .1f);
        if (m_effects)
        {
            const auto contact =
                m_object->getPosition() -
                hit.normal * m_object->getCollidable()->getAABBox().getHalfWidths();
            m_effects->impact(contact, hit.normal, m_color, m_playerShot);
        }
        m_object->markDeleted();
    }
    m_remaining -= dt;
    if (m_remaining <= 0)
        m_object->markDeleted();
}
