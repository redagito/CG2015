#pragma once

#include "game/CGameObject.h"

#include "GameObjectId.h"
#include "arena/CEnemyNeighborhood.h"
#include "combat/TargetSnapshot.h"
#include <list>
#include <memory>
#include <unordered_map>
#include <vector>

/**
 * \brief Stores game objects and manages object lifetime.
 */
class CGameWorld
{
  public:
    /**
     * \brief Updates all game objects.
     */
    void update(float dtime);
    void freezeSimulation();
    bool simulationFrozen() const;

    /**
     * \brief Adds game object.
     *
     * The object must be created on the heap and is owned by the game world.
     */
    GameObjectId addObject(CGameObject *object);
    const CGameObject *getObject(GameObjectId id) const;
    std::vector<TargetSnapshot> targets() const;
    void clear();
    GameObjectId idOf(const CGameObject *object) const;
    std::vector<TargetSnapshot> nearbyTargets(glm::vec3 position, float radius) const;

  private:
    // Private nested storage; snapshots never retain pointers into this list.
    struct Entry
    {
        GameObjectId id;
        std::unique_ptr<CGameObject> object;
    };
    std::unordered_map<GameObjectId, CGameObject *> m_lookup;
    mutable CEnemyNeighborhood m_neighborhood;
    mutable bool m_neighborhoodDirty = true;
    bool m_frozen = false;
    GameObjectId m_nextId = 1;
    std::list<Entry> m_objects;
};
