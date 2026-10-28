#include "ProjectileBallistics.h"
#include "game/CGameObject.h"
#include <algorithm>
#include <cmath>
#include <glm/gtx/quaternion.hpp>

glm::vec3 ProjectileBallistics::muzzlePosition(const CGameObject &ship, glm::vec3 localOffset)
{
    return ship.getPosition() + ship.getOrientation() * (localOffset * ship.getScale());
}
ProjectileLaunch ProjectileBallistics::launch(const WeaponAim &aim)
{
    ProjectileLaunch launch;
    launch.position = aim.muzzle;
    launch.velocity = aim.inheritedVelocity + aim.forward * aim.speed;
    const auto direction = glm::dot(launch.velocity, launch.velocity) > 1e-8f
                               ? glm::normalize(launch.velocity)
                               : aim.forward;
    // The bundled projectile's long axis is +Y, not the ship's +Z.
    launch.orientation = glm::rotation(glm::vec3(0, 1, 0), direction);
    return launch;
}
std::optional<InterceptSolution> ProjectileBallistics::intercept(const WeaponAim &aim,
                                                                 glm::vec3 targetPosition,
                                                                 glm::vec3 targetVelocity)
{
    const glm::dvec3 r = glm::dvec3(targetPosition) - glm::dvec3(aim.muzzle);
    const glm::dvec3 v = glm::dvec3(targetVelocity) - glm::dvec3(aim.inheritedVelocity);
    const double speed = aim.speed, a = glm::dot(v, v) - speed * speed, b = 2 * glm::dot(r, v),
                 c = glm::dot(r, r);
    if (!std::isfinite(a) || !std::isfinite(b) || !std::isfinite(c) || speed <= 0 || c < 1e-8)
        return {};
    double time = -1;
    if (std::abs(a) < 1e-10 * std::max(1.0, glm::dot(v, v) + speed * speed))
    {
        if (std::abs(b) > 1e-10)
            time = -c / b;
    }
    else
    {
        const double discriminant = b * b - 4 * a * c;
        if (discriminant < 0)
            return {};
        const double q = -.5 * (b + std::copysign(std::sqrt(discriminant), b));
        const double first = q / a, second = std::abs(q) > 1e-12 ? c / q : -1;
        if (first > 1e-6)
            time = first;
        if (second > 1e-6 && (time < 0 || second < time))
            time = second;
    }
    if (!std::isfinite(time) || time <= 1e-6 || time > aim.lifetime)
        return {};
    InterceptSolution result;
    result.time = float(time);
    // Compensate for inherited shooter velocity; this is not simply the future target position.
    result.aimPoint = glm::vec3(glm::dvec3(aim.muzzle) + r + v * time);
    result.impactPoint = glm::vec3(glm::dvec3(targetPosition) + glm::dvec3(targetVelocity) * time);
    return result;
}
