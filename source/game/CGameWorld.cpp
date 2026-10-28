#include "CGameWorld.h"
#include <cassert>
void CGameWorld::update(float dtime)
{
    if (m_frozen)
        return;
    m_neighborhood.rebuild(targets());
    m_neighborhoodDirty = false;
    // Update receivers before projectile sweeps, regardless of spawn order.
    // Shots born this frame are already at their muzzle and advance next frame.
    const auto lastExisting = m_nextId - 1;
    for (int phase = 0; phase < 2; ++phase)
    {
        auto iter = m_objects.begin();
        while (!m_frozen && iter != m_objects.end() && iter->id <= lastExisting)
        {
            if (!iter->object->isDeleteRequested() && iter->object->isProjectile() == (phase == 1))
                iter->object->update(dtime);
            if (iter->object->isDeleteRequested())
            {
                m_lookup.erase(iter->id);
                iter = m_objects.erase(iter);
            }
            else
                ++iter;
        }
    }
    m_neighborhoodDirty = true;
}
GameObjectId CGameWorld::addObject(CGameObject *object)
{
    assert(object);
    const auto id = m_nextId++;
    m_objects.push_back({id, std::unique_ptr<CGameObject>(object)});
    m_lookup.emplace(id, object);
    if (!object->getTargetName().empty())
        m_neighborhoodDirty = true;
    return id;
}
const CGameObject *CGameWorld::getObject(GameObjectId id) const
{
    const auto found = m_lookup.find(id);
    return found == m_lookup.end() ? nullptr : found->second;
}
std::vector<TargetSnapshot> CGameWorld::targets() const
{
    std::vector<TargetSnapshot> result;
    for (const auto &entry : m_objects)
    {
        const auto &object = *entry.object;
        if (!object.getTargetName().empty() && !object.isDead() && !object.isDeleteRequested())
            result.push_back(
                {entry.id, object.getTargetName(), object.getPosition(), object.getVelocity()});
    }
    return result;
}

void CGameWorld::clear()
{
    m_frozen = false;
    m_objects.clear();
    m_lookup.clear();
    m_neighborhood.rebuild({});
    m_neighborhoodDirty = true;
}

GameObjectId CGameWorld::idOf(const CGameObject *object) const
{
    for (const auto &entry : m_objects)
        if (entry.object.get() == object)
            return entry.id;
    return noGameObject;
}

std::vector<TargetSnapshot> CGameWorld::nearbyTargets(glm::vec3 position, float radius) const
{
    if (m_neighborhoodDirty)
    {
        m_neighborhood.rebuild(targets());
        m_neighborhoodDirty = false;
    }
    return m_neighborhood.nearby(position, radius);
}

void CGameWorld::freezeSimulation()
{
    // Safe inside a controller callback: no list mutation, and update stops before the next actor.
    m_frozen = true;
    for (auto &entry : m_objects)
    {
        entry.object->setVelocity(glm::vec3(0));
        if (entry.object->isProjectile())
            entry.object->setVisible(false);
    }
}
bool CGameWorld::simulationFrozen() const
{
    return m_frozen;
}
