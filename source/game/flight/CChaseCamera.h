#pragma once
#include "FlightState.h"
#include "FlightTuning.h"
#include "graphics/camera/CCamera.h"

class CChaseCamera : public CCamera
{
  public:
    explicit CChaseCamera(const FlightTuning &tuning);
    void follow(glm::vec3 position, const FlightState &flight, float seconds, float aspect);
    void reset();
    void addImpact(float strength);

  private:
    FlightTuning m_tuning;
    glm::vec3 m_eye{0}, m_eyeVelocity{0}, m_target{0}, m_targetVelocity{0};
    glm::vec3 m_previousShip{0}, m_up{0, 1, 0};
    float m_fieldOfView;
    float m_trauma = 0, m_shakeTime = 0;
    bool m_initialized = false;
    static void spring(glm::vec3 &position, glm::vec3 &velocity, glm::vec3 target, float frequency,
                       float seconds);
};
