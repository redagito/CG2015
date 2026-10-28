#include "EnemySeparation.h"
#include <algorithm>
#include <cmath>
#include <glm/geometric.hpp>
glm::vec3 EnemySeparation::avoidance(GameObjectId self, glm::vec3 position, glm::vec3 velocity,
                                     const std::vector<TargetSnapshot> &neighbors,
                                     const EnemyTuning &tuning)
{
    glm::vec3 steering(0);
    for (const auto &other : neighbors)
    {
        if (other.id == self)
            continue;
        const auto offset = position - other.position, relative = velocity - other.velocity;
        const float distance = glm::length(offset);
        if (distance > tuning.separationDistance * 3)
            continue;
        const float relativeSpeed = glm::dot(relative, relative);
        const float time = relativeSpeed > 1e-6f
                               ? std::clamp(-glm::dot(offset, relative) / relativeSpeed, 0.f,
                                            tuning.separationLookAhead)
                               : 0;
        const auto closest = offset + relative * time;
        const float predicted = glm::length(closest);
        const float proximity = std::max(0.f, 1 - distance / tuning.separationDistance);
        const float collision =
            std::max(0.f, 1 - predicted / tuning.separationDistance) / (1 + time);
        if (proximity <= 0 && collision <= 0)
            continue;
        auto direction = proximity >= collision ? offset : closest;
        if (glm::dot(direction, direction) < 1e-6f)
        {
            // Coincident/head-on ships need a deterministic lateral choice, not normalize(0).
            const auto low = std::min(self, other.id), high = std::max(self, other.id);
            const float seed = float((low * 17 + high * 31) % 101);
            direction = {std::cos(seed * .7f), std::sin(seed * 1.3f), std::sin(seed * .5f)};
            if (relativeSpeed > 1e-6f)
                direction -= relative * (glm::dot(direction, relative) / relativeSpeed);
            if (glm::dot(direction, direction) < 1e-6f)
                direction = std::abs(relative.y) < .9f * glm::length(relative) ? glm::vec3(0, 1, 0)
                                                                               : glm::vec3(1, 0, 0);
            if (self > other.id)
                direction = -direction;
        }
        steering += glm::normalize(direction) * std::max(proximity, collision);
    }
    const float length = glm::length(steering);
    return length > 1 ? steering / length : steering;
}
