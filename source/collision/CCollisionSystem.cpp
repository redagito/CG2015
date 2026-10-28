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
    CCollidable *entity = new CCollidable(groupId, box);
    m_entities[groupId].push_back(entity);
    return entity;
}

// Removes collidable entity
void CCollisionSystem::remove(CCollidable *entity)
{
    m_entities[entity->getGroupId()].remove(entity);
    delete entity;
}

// Test all entities for collision
void CCollisionSystem::update()
{
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
                if ((*iter)->isCollidable())
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
                                     unsigned ignoredGroup, float motionFraction) const
{
    CollisionHit closest;
    const auto projectileDelta = end - start;
    for (unsigned group = 0; group < m_entities.size(); ++group)
    {
        if (group == ignoredGroup)
            continue;
        for (auto *entity : m_entities[group])
        {
            if (!entity->isCollidable() || entity->deleteRequested())
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
            if (intersects && enter <= closest.fraction)
                closest = {entity, enter, normal};
        }
    }
    return closest;
}

void CCollisionSystem::beginStep()
{
    for (const auto &group : m_entities)
        for (auto *entity : group)
            if (!entity->deleteRequested())
                entity->capturePreviousBounds();
}
