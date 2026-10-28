#pragma once
#include "EnemyTuning.h"
#include "game/combat/TargetSnapshot.h"
#include <vector>
class EnemySeparation
{
  public:
    static glm::vec3 avoidance(GameObjectId self, glm::vec3 position, glm::vec3 velocity,
                               const std::vector<TargetSnapshot> &neighbors,
                               const EnemyTuning &tuning);
};
