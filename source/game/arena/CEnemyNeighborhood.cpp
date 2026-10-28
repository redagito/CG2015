#include "CEnemyNeighborhood.h"
#include <cmath>
std::tuple<int, int, int> CEnemyNeighborhood::cell(glm::vec3 position)
{
    return {int(std::floor(position.x / 64)), int(std::floor(position.y / 64)),
            int(std::floor(position.z / 64))};
}
void CEnemyNeighborhood::rebuild(const std::vector<TargetSnapshot> &targets)
{
    m_cells.clear();
    for (const auto &target : targets)
        m_cells[cell(target.position)].push_back(target);
}
std::vector<TargetSnapshot> CEnemyNeighborhood::nearby(glm::vec3 position, float radius) const
{
    std::vector<TargetSnapshot> result;
    const auto [lowX, lowY, lowZ] = cell(position - glm::vec3(radius));
    const auto [highX, highY, highZ] = cell(position + glm::vec3(radius));
    for (int x = lowX; x <= highX; ++x)
        for (int y = lowY; y <= highY; ++y)
            for (int z = lowZ; z <= highZ; ++z)
            {
                const auto found = m_cells.find({x, y, z});
                if (found == m_cells.end())
                    continue;
                for (const auto &target : found->second)
                    if (glm::distance(position, target.position) <= radius)
                        result.push_back(target);
            }
    return result;
}
