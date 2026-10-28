#include "CFollowObjectController.h"
#include "game/CGameWorld.h"
#include <glm/ext.hpp>
CFollowObjectController::CFollowObjectController(const CGameWorld &world, GameObjectId target)
    : m_world(world), m_target(target)
{
}
void CFollowObjectController::attach(CGameObject *object)
{
    m_object = object;
}
void CFollowObjectController::detach()
{
    m_object = nullptr;
}
void CFollowObjectController::setActive(bool active)
{
    m_active = active;
}
void CFollowObjectController::receiveMessage(Message)
{
}
void CFollowObjectController::update(float seconds)
{
    if (!m_object || !m_active)
        return;
    const auto *target = m_world.getObject(m_target);
    if (!target || target->isDead())
    {
        m_object->markDeleted();
        return;
    }
    m_angle += seconds;
    m_object->setPosition(target->getPosition());
    m_object->setVelocity(target->getVelocity());
    m_object->setOrientation(target->getOrientation() *
                             glm::angleAxis(m_angle, glm::vec3(0, 0, 1)));
}
