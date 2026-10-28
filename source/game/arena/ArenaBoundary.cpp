#include "ArenaBoundary.h"
#include <algorithm>
#include <cmath>
#include <glm/ext.hpp>
BoundaryResponse ArenaBoundary::evaluate(glm::vec3 position, const ArenaTuning &arena)
{
    BoundaryResponse result;
    result.clearance = 1e10f;
    const auto local = position - arena.center;
    for (int axis = 0; axis < 3; ++axis)
    {
        const float clearance = arena.halfExtents[axis] - std::abs(local[axis]);
        result.clearance = std::min(result.clearance, clearance);
        const float weight = glm::smoothstep(
            0.f, 1.f,
            std::clamp((arena.warningMargin - clearance) / arena.warningMargin, 0.f, 1.f));
        result.inward[axis] = -glm::sign(local[axis]) * weight;
        result.strength = std::max(result.strength, weight);
    }
    return result;
}
glm::vec3 ArenaBoundary::correctTurn(glm::quat orientation, glm::vec3 pilotRate,
                                     const BoundaryResponse &response, const ArenaTuning &arena)
{
    if (response.strength <= 0 || glm::dot(response.inward, response.inward) < 1e-8f)
        return pilotRate;
    const auto forward = orientation * glm::vec3(0, 0, 1),
               desired = glm::normalize(response.inward);
    const float angle = std::acos(std::clamp(glm::dot(forward, desired), -1.f, 1.f));
    auto axis = glm::cross(forward, desired);
    if (glm::dot(axis, axis) < 1e-8f)
        axis = orientation * glm::vec3(0, 1, 0);
    const auto localAxis = glm::conjugate(orientation) * glm::normalize(axis);
    const auto correction =
        localAxis * std::min(angle * arena.returnResponse, glm::radians(arena.returnRate));
    const float blend = glm::smoothstep(.15f, .85f, response.strength);
    const auto assisted = glm::mix(pilotRate, correction, blend);
    return {assisted.x, assisted.y, pilotRate.z}; // Preserve deliberate roll while returning.
}
glm::vec3 ArenaBoundary::dampVelocity(glm::vec3 velocity, const BoundaryResponse &response,
                                      float seconds, const ArenaTuning &arena)
{
    if (response.strength <= 0)
        return velocity;
    const auto outward = -glm::normalize(response.inward);
    const float speed = glm::dot(velocity, outward);
    if (speed > 0)
        velocity -= outward * speed *
                    (1 - std::exp(-arena.velocityDamping * response.strength * response.strength *
                                  seconds));
    return velocity;
}
glm::vec3 ArenaBoundary::safePoint(glm::vec3 point, const ArenaTuning &arena)
{
    const auto safe = arena.halfExtents - glm::vec3(arena.warningMargin);
    return glm::clamp(point, arena.center - safe, arena.center + safe);
}
