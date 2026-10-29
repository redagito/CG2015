#pragma once
#include "game/combat/TargetSnapshot.h"
#include <cstdint>
#include <tuple>
#include <unordered_map>
#include <vector>
// Spatial snapshots shared by enemy steering and incremental spawn placement.
class CEnemyNeighborhood
{
  public:
    void insert(const TargetSnapshot &target);
    bool hasNearby(glm::vec3 position, float radius) const;
    void rebuild(const std::vector<TargetSnapshot> &targets);
    std::vector<TargetSnapshot> nearby(glm::vec3 position, float radius,
                                       std::size_t limit = 0) const;

  private:
    static std::tuple<int, int, int> cell(glm::vec3 position);
    static std::uint64_t key(int x, int y, int z);
    std::unordered_map<std::uint64_t, std::vector<TargetSnapshot>> m_cells;
};
