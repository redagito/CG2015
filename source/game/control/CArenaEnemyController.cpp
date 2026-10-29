#include "CArenaEnemyController.h"
#include "CWeaponController.h"
#include "game/CGameWorld.h"
#include "game/arena/ArenaBoundary.h"
#include "game/arena/EnemySeparation.h"
#include "game/combat/ProjectileBallistics.h"
#include <algorithm>
#include <cmath>
#include <glm/ext.hpp>
CArenaEnemyController::CArenaEnemyController(const CGameWorld &world, GameObjectId player,
                                             const ArenaTuning &arena,
                                             std::shared_ptr<CWeaponController> weapon, float phase)
    : m_world(world), m_player(player), m_arena(arena), m_weapon(std::move(weapon)), m_phase(phase)
{
}
void CArenaEnemyController::attach(CGameObject *object)
{
    m_object = object;
    m_orientation = object->getOrientation();
    m_separationTimer = std::fmod(m_phase * .6180339887f, 1.f) * .05f;
    object->setVelocity(object->getForward() * m_arena.enemies.speed);
}
void CArenaEnemyController::detach()
{
    if (m_weapon)
        m_weapon->setTrigger(false);
    m_object = nullptr;
}
void CArenaEnemyController::setActive(bool active)
{
    m_active = active;
    if (!active && m_weapon)
        m_weapon->setTrigger(false);
}
void CArenaEnemyController::receiveMessage(Message)
{
}
EnemyMode CArenaEnemyController::mode() const
{
    return m_mode;
}
bool CArenaEnemyController::firing() const
{
    return m_firing;
}
void CArenaEnemyController::update(float seconds)
{
    if (!m_active || !m_object || !std::isfinite(seconds) || seconds <= 0)
        return;
    const auto *player = m_world.getObject(m_player);
    m_firing = false;
    if (!player || player->isDead())
    {
        if (m_weapon)
            m_weapon->setTrigger(false);
        return;
    }
    const float duration = std::min(seconds, .25f);
    m_elapsed += duration;
    m_reaction -= duration;
    auto position = m_object->getPosition();
    const auto toPlayer = player->getPosition() - position;
    const float distance = glm::length(toPlayer);
    const auto boundary = ArenaBoundary::evaluate(position, m_arena);
    if (boundary.strength > .15f)
        m_mode = EnemyMode::Returning;
    else if (m_breakTimer > 0)
    {
        m_mode = EnemyMode::BreakAway;
        m_breakTimer -= duration;
    }
    else if (distance < m_arena.enemies.breakDistance)
    {
        m_mode = EnemyMode::BreakAway;
        m_breakTimer = m_arena.enemies.breakDuration;
        const auto away = distance > 1e-4f ? -toPlayer / distance : -m_object->getForward();
        m_escape = ArenaBoundary::safePoint(
            position + away * 100.f + glm::vec3(std::sin(m_phase) * 40, 30, std::cos(m_phase) * 40),
            m_arena);
    }
    else
        m_mode = distance < m_arena.enemies.attackRange ? EnemyMode::AttackRun : EnemyMode::Pursuit;
    auto aim = m_weapon
                   ? m_weapon->aim()
                   : WeaponAim{position, m_object->getForward(), m_object->getVelocity(), 90, 4};
    const auto intercept =
        ProjectileBallistics::intercept(aim, player->getPosition(), player->getVelocity());
    glm::vec3 target =
        intercept ? intercept->aimPoint : player->getPosition() + player->getVelocity() * .5f;
    if (m_mode == EnemyMode::Returning)
        target = m_arena.center;
    else if (m_mode == EnemyMode::BreakAway)
        target = m_escape;
    else if (m_mode == EnemyMode::Pursuit)
        target += glm::vec3(std::sin(m_elapsed * .7f + m_phase) * 12,
                            std::cos(m_elapsed * .5f + m_phase) * 10, 0);
    // Keep different ingress lanes even after pursuit becomes an attack run.
    const glm::vec3 lane(std::cos(m_phase * 2.4f), std::sin(m_phase * 1.7f) * .65f,
                         std::sin(m_phase * 2.4f));
    if (m_mode == EnemyMode::Pursuit || m_mode == EnemyMode::AttackRun)
        target +=
            lane * m_arena.enemies.approachSpread *
            glm::smoothstep(m_arena.enemies.breakDistance, m_arena.enemies.attackRange, distance);
    if (!m_self)
        m_self = m_world.idOf(m_object);
    auto velocity = m_object->getVelocity();
    const int steps = std::max(1, int(std::ceil(duration * 20 - 1e-5f)));
    const float dt = duration / steps;
    glm::vec3 localRate(0);
    // Stagger 20 Hz local avoidance decisions; full flight and firing still update every frame.
    m_separationTimer -= duration;
    if (m_separationTimer <= 0)
    {
        auto neighbors =
            m_world.nearbyTargets(position, m_arena.enemies.separationDistance * 1.5f, 32);
        if (neighbors.size() > 16)
        {
            // Steer against the nearest local threats rather than solving every dense-swarm pair.
            std::nth_element(neighbors.begin(), neighbors.begin() + 16, neighbors.end(),
                             [&](const auto &a, const auto &b)
                             {
                                 const auto da = a.position - position, db = b.position - position;
                                 return glm::dot(da, da) < glm::dot(db, db);
                             });
            neighbors.resize(16);
        }
        m_separation =
            EnemySeparation::avoidance(m_self, position, velocity, neighbors, m_arena.enemies);
        m_separationTimer += .05f;
        if (m_separationTimer < 0)
            m_separationTimer = 0;
    }
    const auto separation = m_separation;
    for (int step = 0; step < steps; ++step)
    {
        const auto forward = m_orientation * glm::vec3(0, 0, 1), delta = target - position;
        auto desired = glm::dot(delta, delta) > 1e-8f ? glm::normalize(delta) : forward;
        desired += separation * m_arena.enemies.separationStrength;
        if (glm::dot(desired, desired) < 1e-8f)
            desired = forward;
        desired = glm::normalize(desired);
        const float angle = std::acos(std::clamp(glm::dot(forward, desired), -1.f, 1.f));
        auto axis = glm::cross(forward, desired);
        if (glm::dot(axis, axis) < 1e-8f)
            axis = m_orientation * glm::vec3(0, 1, 0);
        localRate = glm::conjugate(m_orientation) * glm::normalize(axis) *
                    std::min(angle * 3.f, glm::radians(m_arena.enemies.turnRate));
        const auto response = ArenaBoundary::evaluate(position, m_arena);
        localRate = ArenaBoundary::correctTurn(m_orientation, localRate, response, m_arena);
        const float rotation = glm::length(localRate) * dt;
        if (rotation > 1e-8f)
            m_orientation =
                glm::normalize(m_orientation * glm::angleAxis(rotation, glm::normalize(localRate)));
        const float speed =
            m_mode == EnemyMode::BreakAway ? m_arena.enemies.breakSpeed : m_arena.enemies.speed;
        velocity += (m_orientation * glm::vec3(0, 0, speed) - velocity) *
                    (1 - std::exp(-m_arena.enemies.velocityResponse * dt));
        velocity += separation * m_arena.enemies.separationAcceleration * dt;
        velocity = ArenaBoundary::dampVelocity(velocity, response, dt, m_arena);
        position += velocity * dt;
    }
    m_object->setPosition(position);
    m_object->setVelocity(velocity);
    const float bank = std::clamp(-localRate.y * .25f, -.4f, .4f);
    m_object->setOrientation(m_orientation * glm::angleAxis(bank, glm::vec3(0, 0, 1)));
    aim = m_weapon ? m_weapon->aim() : WeaponAim{position, m_object->getForward(), velocity, 90, 4};
    const auto shot =
        ProjectileBallistics::intercept(aim, player->getPosition(), player->getVelocity());
    if (m_mode == EnemyMode::AttackRun && m_reaction <= 0 && shot)
    {
        const auto delta = shot->aimPoint - aim.muzzle;
        m_firing = glm::dot(delta, delta) > 1e-8f &&
                   glm::dot(glm::normalize(delta), m_object->getForward()) >
                       std::cos(glm::radians(m_arena.enemies.fireAngle));
    }
    if (m_weapon)
        m_weapon->setTrigger(m_firing);
}
