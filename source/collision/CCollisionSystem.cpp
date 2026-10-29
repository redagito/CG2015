#include "CCollisionSystem.h"

#include "CCollidable.h"
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>

unsigned int CCollisionSystem::getNewGroupId()
{
    // Add new collision group
    m_entities.push_back(std::list<CCollidable *>());
    // Return the group id
    return m_entities.size() - 1;
}

// Cleanup
CCollisionSystem::~CCollisionSystem()
{
    for (unsigned int i = 0; i < m_entities.size(); ++i)
    {
        for (auto iter = m_entities[i].begin(); iter != m_entities[i].end(); ++iter)
        {
            delete (*iter);
        }
    }
}

CCollidable *CCollisionSystem::add(const CAABBox &box, unsigned int groupId)
{
    if (groupId >= m_entities.size())
    {
        SPDLOG_ERROR("Group id {} does not exist", groupId);
        throw std::runtime_error("CCollisionSystem.add: Invalid goup id");
    }
    // Create new entity and add to the collision group
    invalidateQueries();
    CCollidable *entity = new CCollidable(groupId, box, m_nextId++, this);
    m_entities[groupId].push_back(entity);
    return entity;
}

// Removes collidable entity
void CCollisionSystem::remove(CCollidable *entity)
{
    invalidateQueries();
    m_entities[entity->getGroupId()].remove(entity);
    delete entity;
}

// Test all entities for collision
void CCollisionSystem::update()
{
    invalidateQueries();
    for (unsigned int i = 0; i < m_entities.size(); ++i)
    {
        auto iter = m_entities[i].begin();
        while (iter != m_entities[i].end())
        {
            // Delete if requested
            if ((*iter)->deleteRequested())
            {
                CCollidable *temp = (*iter);
                iter = m_entities[i].erase(iter);
                delete temp;
            }
            else
            {
                if ((*iter)->isCollidable() && (*iter)->getDamage() > 0)
                {
                    testCollision((*iter), i);
                }
                ++iter;
            }
        }
    }
}

// Tests collidable entity against all entities not in the group
void CCollisionSystem::testCollision(CCollidable *entity, unsigned int group)
{
    // Everything gets tested twice, make this faster
    for (unsigned int i = 0; i < m_entities.size(); ++i)
    {
        if (i != group)
        {
            auto iter = m_entities[i].begin();
            while (iter != m_entities[i].end())
            {
                // Delete if requested
                if ((*iter)->deleteRequested())
                {
                    CCollidable *temp = (*iter);
                    iter = m_entities[i].erase(iter);
                    delete temp;
                }
                else
                {
                    if ((*iter)->isCollidable())
                    {
                        if (collides(entity->getAABBox(), (*iter)->getAABBox()))
                        {
                            // Since every collision is tested both ways, damage only happens from
                            // first entity to second entity
                            (*iter)->receiveDamage(entity->getDamage(),
                                                   entity->getAABBox().getMid());
                        }
                    }
                    ++iter;
                }
            }
        }
    }
}
CollisionHit CCollisionSystem::trace(glm::vec3 start, glm::vec3 end, glm::vec3 halfWidths,
                                     unsigned ignoredGroup, float motionFraction,
                                     std::span<const std::uint64_t> excluded) const
{
    ++m_queryStats.traces;
    CollisionHit closest;
    const auto projectileDelta = end - start;
    const auto lowQuery = glm::min(start, end) - halfWidths,
               highQuery = glm::max(start, end) + halfWidths;
    for (auto *entity : candidates(lowQuery, highQuery, ignoredGroup))
    {
        ++m_queryStats.receivers;
        if (entity->getGroupId() == ignoredGroup || !entity->isCollidable() ||
            entity->deleteRequested() ||
            std::find(excluded.begin(), excluded.end(), entity->id()) != excluded.end())
            continue;
        const auto &current = entity->getAABBox(), &previous = entity->previousBounds();
        // Sweep in the receiver's moving frame, not against only its final position.
        const auto delta =
            projectileDelta - (current.getMid() - previous.getMid()) * motionFraction;
        const auto extents =
            glm::max(current.getHalfWidths(), previous.getHalfWidths()) + halfWidths;
        const auto low = previous.getMid() - extents, high = previous.getMid() + extents;
        float enter = 0, exit = closest.fraction;
        bool intersects = true;
        glm::vec3 normal(0);
        for (int axis = 0; axis < 3; ++axis)
        {
            if (std::abs(delta[axis]) < 1e-8f)
            {
                if (start[axis] < low[axis] || start[axis] > high[axis])
                {
                    intersects = false;
                    break;
                }
            }
            else
            {
                float first = (low[axis] - start[axis]) / delta[axis],
                      last = (high[axis] - start[axis]) / delta[axis];
                if (first > last)
                    std::swap(first, last);
                if (first > enter)
                {
                    enter = first;
                    normal = glm::vec3(0);
                    normal[axis] = delta[axis] > 0 ? -1.f : 1.f;
                }
                exit = std::min(exit, last);
                if (enter > exit)
                {
                    intersects = false;
                    break;
                }
            }
        }
        const bool winsTie = !closest.entity ||
                             entity->getGroupId() > closest.entity->getGroupId() ||
                             (entity->getGroupId() == closest.entity->getGroupId() &&
                              entity->id() > closest.entity->id());
        if (intersects && (enter < closest.fraction || (enter == closest.fraction && winsTie)))
            closest = {entity, enter, normal};
    }
    return closest;
}

void CCollisionSystem::beginStep()
{
    m_queryStats = {};
    invalidateQueries();
    for (const auto &group : m_entities)
        for (auto *entity : group)
            if (!entity->deleteRequested())
                entity->capturePreviousBounds();
}

void CCollisionSystem::invalidateQueries()
{
    m_queriesDirty = true;
}
std::uint64_t CCollisionSystem::cellKey(int x, int y, int z)
{
    const auto part = [](int value) { return std::uint64_t(value) & 0x1fffff; };
    return part(x) | (part(y) << 21) | (part(z) << 42);
}
void CCollisionSystem::buildQueries() const
{
    if (!m_queriesDirty)
        return;
    ++m_queryStats.indexBuilds;
    m_groupCells.resize(m_entities.size());
    for (auto &cells : m_groupCells)
        cells.clear();
    m_largeReceivers.clear();
    for (std::size_t group = 0; group < m_entities.size(); ++group)
        for (auto *entity : m_entities[group])
        {
            if (!entity->isCollidable() || entity->deleteRequested())
                continue;
            const auto &a = entity->previousBounds(), &b = entity->getAABBox();
            const auto low =
                glm::ivec3(glm::floor((glm::min(a.getMid(), b.getMid()) -
                                       glm::max(a.getHalfWidths(), b.getHalfWidths())) /
                                      32.f));
            const auto high =
                glm::ivec3(glm::floor((glm::max(a.getMid(), b.getMid()) +
                                       glm::max(a.getHalfWidths(), b.getHalfWidths())) /
                                      32.f));
            const auto count = glm::dvec3(high - low + 1);
            if (count.x * count.y * count.z > 4096)
            {
                m_largeReceivers.push_back(entity);
                continue;
            }
            for (int x = low.x; x <= high.x; ++x)
                for (int y = low.y; y <= high.y; ++y)
                    for (int z = low.z; z <= high.z; ++z)
                        m_groupCells[group][cellKey(x, y, z)].push_back(entity);
        }
    m_queriesDirty = false;
}
std::vector<CCollidable *> CCollisionSystem::candidates(glm::vec3 from, glm::vec3 to,
                                                        unsigned ignoredGroup) const
{
    buildQueries();
    const auto low = glm::ivec3(glm::floor(from / 32.f)), high = glm::ivec3(glm::floor(to / 32.f));
    const auto count = glm::dvec3(high - low + 1);
    std::vector<CCollidable *> result;
    if (count.x * count.y * count.z > 4096)
    {
        for (std::size_t group = 0; group < m_entities.size(); ++group)
            if (group != ignoredGroup)
                for (auto *entity : m_entities[group])
                    if (entity->isCollidable() && !entity->deleteRequested())
                        result.push_back(entity);
        return result;
    }
    const auto serial = ++m_querySerial;
    const auto append = [&](CCollidable *entity)
    {
        if (entity->getGroupId() != ignoredGroup && entity->m_queryStamp != serial)
        {
            entity->m_queryStamp = serial;
            result.push_back(entity);
        }
    };
    for (auto *entity : m_largeReceivers)
        append(entity);
    // Reject a whole friendly group before visiting its cells; deduplicate in constant time.
    for (std::size_t group = 0; group < m_groupCells.size(); ++group)
    {
        if (group == ignoredGroup)
            continue;
        for (int x = low.x; x <= high.x; ++x)
            for (int y = low.y; y <= high.y; ++y)
                for (int z = low.z; z <= high.z; ++z)
                {
                    const auto found = m_groupCells[group].find(cellKey(x, y, z));
                    if (found == m_groupCells[group].end())
                        continue;
                    for (auto *entity : found->second)
                        append(entity);
                }
    }
    return result;
}

const CollisionQueryStats &CCollisionSystem::queryStats() const
{
    return m_queryStats;
}
