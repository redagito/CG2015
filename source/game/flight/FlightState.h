#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

struct FlightState
{
    glm::quat orientation{1, 0, 0, 0}; // Physical attitude; automatic banking is visual only.
    glm::vec3 velocity{0}, angularVelocity{0};
    glm::vec2 steering{0}, pointer{0};
    float throttle = .5f, speed = 0, bank = 0, boost = 0;
    bool braking = false, boundaryAssist = false;
    float boundaryWarning = 0;
    bool boundaryVertical = false, boundaryFloor = false;
};
