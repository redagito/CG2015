#include "CSimpleWaypointController.h"

#include "game/CGameObject.h"
#include "math/TransformUtils.h"

#include "game/state/AGameState.h"

#include <GLFW/glfw3.h>

#include <glm/ext.hpp>

CSimpleWaypointController::CSimpleWaypointController(const glm::vec3 &end, float speed,
                                                     AGameState *state)
    : m_end(end), m_speed(speed), m_gameState(state)
{
    return;
}

CSimpleWaypointController::~CSimpleWaypointController()
{
    return;
}

void CSimpleWaypointController::attach(CGameObject *object)
{
    m_object = object;
}

void CSimpleWaypointController::detach()
{
    m_object = nullptr;
}

void CSimpleWaypointController::setActive(bool state)
{
    m_active = state;
    if (m_object && !state)
        m_object->setVelocity(glm::vec3(0));
}

void CSimpleWaypointController::update(float dtime)
{
    if (!m_object || !m_active || dtime <= 0)
        return;
    const auto position = m_object->getPosition(), delta = m_end - position;
    const float distance = glm::length(delta);
    if (distance < 1e-5f)
    {
        m_object->setVelocity(glm::vec3(0));
        m_idleTimer -= dtime;
        if (m_idleTimer <= 0 && m_gameState)
            m_gameState->triggerStateTransition("lose");
        return;
    }
    const auto next = position + delta / distance * glm::min(m_speed * dtime, distance);
    m_object->setVelocity((next - position) / dtime);
    m_object->setPosition(next);
}

void CSimpleWaypointController::receiveMessage(Message msg)
{
}