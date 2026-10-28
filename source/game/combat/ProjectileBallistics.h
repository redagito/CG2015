#pragma once
#include "InterceptSolution.h"
#include "ProjectileLaunch.h"
#include "WeaponAim.h"
#include <optional>
class CGameObject;
class ProjectileBallistics
{
  public:
    static glm::vec3 muzzlePosition(const CGameObject &ship, glm::vec3 localOffset);
    static ProjectileLaunch launch(const WeaponAim &aim);
    static std::optional<InterceptSolution> intercept(const WeaponAim &aim,
                                                      glm::vec3 targetPosition,
                                                      glm::vec3 targetVelocity);
};
