#include "CEnemyNeighborhood.h"
#include <algorithm>
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
        insert(target);
}
std::vector<TargetSnapshot> CEnemyNeighborhood::nearby(glm::vec3 position, float radius,
                                                       std::size_t limit) const
{
    std::vector<TargetSnapshot> result;
    result.reserve(limit ? limit : 64);
    const auto [cx, cy, cz] = cell(position);
    const int rings = int(std::ceil(radius / 64));
    // Search the local cell first. Swarm steering takes a bounded local sample; exact callers use
    // limit=0.
    for (int ring = 0; ring <= rings; ++ring)
        for (int x = -ring; x <= ring; ++x)
            for (int y = -ring; y <= ring; ++y)
                for (int z = -ring; z <= ring; ++z)
                {
                    if (std::max({std::abs(x), std::abs(y), std::abs(z)}) != ring)
                        continue;
                    const auto found = m_cells.find(key(cx + x, cy + y, cz + z));
                    if (found == m_cells.end())
                        continue;
                    for (const auto &target : found->second)
                    {
                        const auto delta = position - target.position;
                        if (glm::dot(delta, delta) > radius * radius)
                            continue;
                        result.push_back(target);
                        if (limit && result.size() >= limit)
                            return result;
                    }
                }
    return result;
}

void CEnemyNeighborhood::insert(const TargetSnapshot &target)
{
    const auto [x, y, z] = cell(target.position);
    m_cells[key(x, y, z)].push_back(target);
}
bool CEnemyNeighborhood::hasNearby(glm::vec3 position, float radius) const
{
    const auto [lowX, lowY, lowZ] = cell(position - glm::vec3(radius));
    const auto [highX, highY, highZ] = cell(position + glm::vec3(radius));
    for (int x = lowX; x <= highX; ++x)
        for (int y = lowY; y <= highY; ++y)
            for (int z = lowZ; z <= highZ; ++z)
            {
                const auto found = m_cells.find(key(x, y, z));
                if (found == m_cells.end())
                    continue;
                for (const auto &target : found->second)
                {
                    const auto delta = position - target.position;
                    if (glm::dot(delta, delta) < radius * radius)
                        return true;
                }
            }
    return false;
}

std::uint64_t CEnemyNeighborhood::key(int x, int y, int z)
{
    // Signed cells fit comfortably in 21 bits per axis throughout the supported arena volume.
    const auto component = [](int value) { return std::uint64_t(value) & 0x1fffff; };
    return component(x) | (component(y) << 21) | (component(z) << 42);
}
