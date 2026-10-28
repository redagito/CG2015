#include "CMothershipController.h"
#include "game/CGameObject.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

CMothershipController::CMothershipController(glm::vec3 destination, const MothershipTuning &tuning,
                                             std::function<void()> onDeadline)
    : m_destination(destination), m_tuning(tuning), m_onDeadline(std::move(onDeadline))
{
    if (!std::isfinite(tuning.approachSeconds) || tuning.approachSeconds <= 0 ||
        !std::isfinite(tuning.waitSeconds) || tuning.waitSeconds < 0)
        throw std::invalid_argument("Invalid mothership timing");
}
void CMothershipController::attach(CGameObject *object)
{
    m_object = object;
    if (object)
        m_start = object->getPosition();
    m_elapsed = 0;
    m_finished = false;
}
void CMothershipController::detach()
{
    m_object = nullptr;
}
void CMothershipController::setActive(bool active)
{
    m_active = active;
    if (!active && m_object)
        m_object->setVelocity(glm::vec3(0));
}
void CMothershipController::receiveMessage(Message)
{
}
void CMothershipController::update(float seconds)
{
    if (!m_object || !m_active || m_finished || !std::isfinite(seconds) || seconds <= 0)
        return;
    m_elapsed =
        std::min(m_elapsed + seconds, double(m_tuning.approachSeconds) + m_tuning.waitSeconds);
    // Interpolate from the initial position; an arrival step also consumes its wait-time remainder.
    const float fraction = float(std::min(1.0, m_elapsed / m_tuning.approachSeconds));
    m_object->setPosition(glm::mix(m_start, m_destination, fraction));
    m_object->setVelocity(approachRemaining() > 0
                              ? (m_destination - m_start) / m_tuning.approachSeconds
                              : glm::vec3(0));
    if (totalRemaining() <= 0)
    {
        m_finished = true;
        if (m_onDeadline)
            m_onDeadline();
    }
}
float CMothershipController::approachRemaining() const
{
    return float(std::max(0.0, double(m_tuning.approachSeconds) - m_elapsed));
}
float CMothershipController::totalRemaining() const
{
    return float(
        std::max(0.0, double(m_tuning.approachSeconds) + m_tuning.waitSeconds - m_elapsed));
}
