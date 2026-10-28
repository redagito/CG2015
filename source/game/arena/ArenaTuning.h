#pragma once
#include "EnemyTuning.h"
#include "MothershipTuning.h"
#include <glm/glm.hpp>
class IResourceManager;
struct ArenaTuning
{
    glm::vec3 center{0, 100, 0}, halfExtents{800, 90, 800};
    float warningMargin = 45, returnRate = 150, returnResponse = 3, velocityDamping = 18;
    EnemyTuning enemies;
    MothershipTuning mothership;
    static ArenaTuning load(IResourceManager &resources);
    void validate() const;
};
