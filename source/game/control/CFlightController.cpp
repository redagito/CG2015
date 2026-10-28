#include "CFlightController.h"
#include "game/CGameObject.h"
#include "game/arena/ArenaBoundary.h"
#include "input/IInputProvider.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <glm/gtx/quaternion.hpp>

CFlightController::CFlightController(IInputProvider &input, const FlightTuning &tuning,
                                     const ArenaTuning *arena)
    : m_input(input), m_tuning(tuning)
{
    m_tuning.validate();
    if (arena)
    {
        arena->validate();
        m_arena = *arena;
    }
}
void CFlightController::attach(CGameObject *object)
{
    m_object = object;
    m_state = {};
    m_state.orientation = object->getOrientation();
    m_state.speed = m_tuning.initialSpeed;
    m_state.throttle =
        (m_tuning.initialSpeed - m_tuning.minSpeed) / (m_tuning.maxSpeed - m_tuning.minSpeed);
    m_state.velocity = object->getForward() * m_state.speed;
    object->setVelocity(m_state.velocity);
}
void CFlightController::detach()
{
    m_object = nullptr;
}
void CFlightController::setActive(bool active)
{
    m_active = active;
}
void CFlightController::receiveMessage(Message)
{
}
const FlightState &CFlightController::state() const
{
    return m_state;
}

void CFlightController::update(float seconds)
{
    if (!m_active || !m_object || m_object->isDead() || !std::isfinite(seconds) || seconds <= 0)
        return;
    m_state.pointer = m_input.pointerOffset();
    glm::vec2 stick = m_state.pointer / m_tuning.pointerRadius;
    const float radius = glm::length(stick);
    m_state.steering =
        radius > m_tuning.deadZone
            ? stick / radius *
                  std::pow(
                      std::clamp((radius - m_tuning.deadZone) / (1 - m_tuning.deadZone), 0.f, 1.f),
                      m_tuning.steeringExponent)
            : glm::vec2(0);
    if (m_tuning.invertY)
        m_state.steering.y *= -1;
    const float throttleInput =
        float(m_input.isKeyPressed(GLFW_KEY_W)) - float(m_input.isKeyPressed(GLFW_KEY_S));
    const float rollInput =
        float(m_input.isKeyPressed(GLFW_KEY_D)) - float(m_input.isKeyPressed(GLFW_KEY_A));
    const bool brake = m_input.isKeyPressed(GLFW_KEY_SPACE);
    const bool boost = !brake && m_input.isKeyPressed(GLFW_KEY_LEFT_SHIFT);
    m_state.braking = brake;
    // Small simulation steps keep response and momentum consistent across frame rates.
    const float duration = std::min(seconds, .25f);
    const int steps = std::max(1, int(std::ceil(duration * 120)));
    const float dt = duration / steps;
    auto position = m_object->getPosition();
    for (int step = 0; step < steps; ++step)
    {
        m_state.throttle =
            std::clamp(m_state.throttle + throttleInput * m_tuning.throttleRate * dt, 0.f, 1.f);
        const float targetSpeed =
            brake   ? m_tuning.brakeSpeed
            : boost ? m_tuning.boostSpeed
                    : glm::mix(m_tuning.minSpeed, m_tuning.maxSpeed, m_state.throttle);
        const float acceleration =
            targetSpeed > m_state.speed ? m_tuning.acceleration : m_tuning.deceleration;
        m_state.speed +=
            std::clamp(targetSpeed - m_state.speed, -acceleration * dt, acceleration * dt);
        const float turnScale = brake   ? m_tuning.brakeTurnMultiplier
                                : boost ? m_tuning.boostTurnMultiplier
                                        : 1;
        // Local +Z is forward; the chase view's screen-right points along local -X.
        glm::vec3 targetRate = glm::radians(glm::vec3(m_state.steering.y * m_tuning.pitchRate,
                                                      -m_state.steering.x * m_tuning.yawRate,
                                                      rollInput * m_tuning.rollRate)) *
                               turnScale;
        BoundaryResponse boundary;
        if (m_arena)
        {
            boundary = ArenaBoundary::evaluate(position, *m_arena);
            targetRate =
                ArenaBoundary::correctTurn(m_state.orientation, targetRate, boundary, *m_arena);
            m_state.boundaryWarning = boundary.strength;
            m_state.boundaryAssist = boundary.strength > .65f;
        }
        m_state.angularVelocity +=
            (targetRate - m_state.angularVelocity) * (1 - std::exp(-m_tuning.angularResponse * dt));
        const float angle = glm::length(m_state.angularVelocity) * dt;
        if (angle > 1e-8f)
            m_state.orientation =
                glm::normalize(m_state.orientation *
                               glm::angleAxis(angle, glm::normalize(m_state.angularVelocity)));
        m_state.bank += (glm::radians(m_tuning.autoBank) * m_state.steering.x - m_state.bank) *
                        (1 - std::exp(-m_tuning.bankResponse * dt));
        m_state.boost += (float(boost) - m_state.boost) * (1 - std::exp(-4 * dt));
        const auto forward = m_state.orientation * glm::vec3(0, 0, 1);
        m_state.velocity += (forward * m_state.speed - m_state.velocity) *
                            (1 - std::exp(-m_tuning.velocityResponse * dt));
        if (m_arena)
            m_state.velocity =
                ArenaBoundary::dampVelocity(m_state.velocity, boundary, dt, *m_arena);
        position += m_state.velocity * dt;
    }
    m_object->setVelocity(m_state.velocity);
    m_object->setPosition(position);
    m_object->setOrientation(m_state.orientation *
                             glm::angleAxis(m_state.bank, glm::vec3(0, 0, 1)));
}
