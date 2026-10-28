#pragma once
#include "game/combat/TargetSnapshot.h"
#include <map>
#include <tuple>
#include <vector>
// Immutable per-step spatial snapshot shared by all enemy controllers.
class CEnemyNeighborhood
{
  public:
    void rebuild(const std::vector<TargetSnapshot> &targets);
    std::vector<TargetSnapshot> nearby(glm::vec3 position, float radius) const;

  private:
    static std::tuple<int, int, int> cell(glm::vec3 position);
    std::map<std::tuple<int, int, int>, std::vector<TargetSnapshot>> m_cells;
};
