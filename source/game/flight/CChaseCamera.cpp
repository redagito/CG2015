#include "CChaseCamera.h"
#include <algorithm>
#include <cmath>
#include <glm/ext.hpp>

CChaseCamera::CChaseCamera(const FlightTuning &tuning)
    : m_tuning(tuning), m_fieldOfView(tuning.fieldOfView)
{
    m_tuning.validate();
    setProjection(glm::perspective(glm::radians(m_fieldOfView), 4.f / 3.f, .5f, 1000.f));
    setPosition(glm::vec3(0));
    setView(glm::mat4(1));
}
void CChaseCamera::reset()
{
    m_initialized = false;
    m_trauma = 0;
    m_shakeTime = 0;
}
void CChaseCamera::spring(glm::vec3 &position, glm::vec3 &velocity, glm::vec3 target,
                          float frequency, float seconds)
{
    // Exact critically damped spring for a fixed target: stable even on a long frame.
    const float omega = glm::two_pi<float>() * frequency, decay = std::exp(-omega * seconds);
    const auto offset = position - target, term = velocity + omega * offset;
    position = target + (offset + term * seconds) * decay;
    velocity = (velocity - omega * term * seconds) * decay;
}
void CChaseCamera::follow(glm::vec3 position, const FlightState &flight, float seconds,
                          float aspect)
{
    if (!std::isfinite(seconds) || seconds < 0 || !std::isfinite(aspect) || aspect <= 0)
        return;
    const float dt = std::min(seconds, .25f);
    const auto forward = flight.orientation * glm::vec3(0, 0, 1);
    const auto right = flight.orientation * glm::vec3(-1, 0, 0);
    const auto shipUp = flight.orientation * glm::vec3(0, 1, 0);
    const auto desiredEye =
        position - forward * m_tuning.cameraDistance + shipUp * m_tuning.cameraHeight;
    const auto desiredTarget =
        position + forward * m_tuning.lookAhead +
        (right * flight.steering.x - shipUp * flight.steering.y) * m_tuning.anticipation;
    const bool snap =
        !m_initialized || glm::distance(position, m_previousShip) > m_tuning.resetDistance;
    if (snap)
    {
        m_eye = desiredEye;
        m_target = desiredTarget;
        m_eyeVelocity = m_targetVelocity = glm::vec3(0);
        m_up = shipUp;
        m_fieldOfView = m_tuning.fieldOfView + flight.boost * m_tuning.boostFieldOfView;
        m_initialized = true;
    }
    else
    {
        spring(m_eye, m_eyeVelocity, desiredEye, m_tuning.cameraFrequency, dt);
        spring(m_target, m_targetVelocity, desiredTarget, m_tuning.cameraFrequency, dt);
        m_up = glm::mix(m_up, shipUp, 1 - std::exp(-m_tuning.cameraRotationResponse * dt));
        m_fieldOfView +=
            (m_tuning.fieldOfView + flight.boost * m_tuning.boostFieldOfView - m_fieldOfView) *
            (1 - std::exp(-m_tuning.fieldOfViewResponse * dt));
    }
    m_previousShip = position;
    auto viewForward = m_target - m_eye;
    if (glm::dot(viewForward, viewForward) < 1e-8f)
        viewForward = forward;
    viewForward = glm::normalize(viewForward);
    auto up = m_up - viewForward * glm::dot(m_up, viewForward);
    // A roll or teleport can temporarily align the smoothed up and view axes.
    if (glm::dot(up, up) < 1e-6f)
        up = shipUp - viewForward * glm::dot(shipUp, viewForward);
    if (glm::dot(up, up) < 1e-6f)
    {
        const auto fallback =
            std::abs(viewForward.y) < .9f ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
        up = fallback - viewForward * glm::dot(fallback, viewForward);
    }
    m_trauma *= std::exp(-8 * dt);
    m_shakeTime += dt;
    const auto normalizedUp = glm::normalize(up),
               cameraRight = glm::normalize(glm::cross(viewForward, normalizedUp));
    const auto eyeKick =
        (cameraRight * std::sin(m_shakeTime * 71.f) + normalizedUp * std::sin(m_shakeTime * 89.f)) *
        .06f * m_trauma;
    const auto lookKick = (cameraRight * std::sin(m_shakeTime * 43.f) * .7f +
                           normalizedUp * std::sin(m_shakeTime * 57.f) * .45f) *
                          m_trauma;
    setPosition(m_eye + eyeKick);
    setView(glm::lookAt(m_eye + eyeKick, m_eye + viewForward * m_tuning.lookAhead + lookKick,
                        normalizedUp));
    // Reticle projection and rendering use this same frame's aspect and FOV.
    setProjection(glm::perspective(glm::radians(m_fieldOfView), aspect, .5f, 4000.f));
}

void CChaseCamera::addImpact(float strength)
{
    if (std::isfinite(strength))
        m_trauma = std::clamp(m_trauma + strength, 0.f, 1.f);
}
