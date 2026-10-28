#pragma once
#include "ArenaTuning.h"
#include "BoundaryResponse.h"
#include <glm/gtc/quaternion.hpp>
class ArenaBoundary
{
  public:
    static BoundaryResponse evaluate(glm::vec3 position, const ArenaTuning &arena);
    static glm::vec3 correctTurn(glm::quat orientation, glm::vec3 pilotRate,
                                 const BoundaryResponse &response, const ArenaTuning &arena);
    static glm::vec3 dampVelocity(glm::vec3 velocity, const BoundaryResponse &response,
                                  float seconds, const ArenaTuning &arena);
    static glm::vec3 safePoint(glm::vec3 point, const ArenaTuning &arena);
};
