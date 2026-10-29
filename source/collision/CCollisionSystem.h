#pragma once

#include "CAABBox.h"
#include "CollisionHit.h"
#include "CollisionQueryStats.h"
#include <cstdint>
#include <span>
#include <unordered_map>
#include <vector>

#include <list>

class CCollidable;

class CCollisionSystem
{
  public:
    /**
     * \brief Cleanup of resources.
     */
    ~CCollisionSystem();

    /**
     * \brief Creates a new collidable and adds it to the specified group.
     *
     * A collidable entity has a bounding box and a transformation matrix.
     * Collidables in the same group are not tested for collision.
     * \param The untransformed bounding box for the entity
     */
    CCollidable *add(const CAABBox &box, unsigned int group);

    /**
     * \brief Remove collidable entity
     */
    void remove(CCollidable *entity);

    /**
     * \brief Tests the entities for collision
     */
    void update();

    /**
     * \brief Creates and returns new collision group id
     */
    unsigned int getNewGroupId();

    void beginStep();
    void invalidateQueries();
    const CollisionQueryStats &queryStats() const;
    CollisionHit trace(glm::vec3 start, glm::vec3 end, glm::vec3 halfWidths, unsigned ignoredGroup,
                       float motionFraction = 1,
                       std::span<const std::uint64_t> excluded = {}) const;

  private:
    void buildQueries() const;
    std::vector<CCollidable *> candidates(glm::vec3 low, glm::vec3 high,
                                          unsigned ignoredGroup) const;
    static std::uint64_t cellKey(int x, int y, int z);
    std::uint64_t m_nextId = 1;
    mutable CollisionQueryStats m_queryStats;
    mutable bool m_queriesDirty = true;
    mutable std::vector<std::unordered_map<std::uint64_t, std::vector<CCollidable *>>> m_groupCells;
    mutable std::uint64_t m_querySerial = 0;
    mutable std::vector<CCollidable *> m_largeReceivers;

    /**
     * \brief Tests the entity for collision with all other entities not in the group
     */
    void testCollision(CCollidable *entity, unsigned int groupId);

    std::vector<std::list<CCollidable *>>
        m_entities; /**< Stores entities by group id (vector index) in lists. */
};