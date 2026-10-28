#include <cassert>

#include "CGameObject.h"
#include "IController.h"

#include "collision/CCollidable.h"

// Constructor initializes drawable and matrices
CGameObject::CGameObject() : m_collidable(nullptr) // Collidable
{
    return;
}

CGameObject::~CGameObject()
{
    return;
}

void CGameObject::setRotation(const glm::vec3 &rotation)
{
    m_rotation = rotation;
    // Update forward vector
    m_orientation = glm::normalize(glm::quat(rotation));
    m_forward = m_orientation * glm::vec3(0, 0, 1);
    m_transformationChanged = true;
}

void CGameObject::setPosition(const glm::vec3 &position)
{
    m_position = position;
    m_transformationChanged = true;
}

void CGameObject::setScale(const glm::vec3 &scale)
{
    m_scale = scale;
    m_transformationChanged = true;
}

const glm::vec3 &CGameObject::getRotation() const
{
    return m_rotation;
}

void CGameObject::addController(const std::shared_ptr<IController> &controller)
{
    assert(controller != nullptr);
    controller->attach(this);
    m_controllers.push_back(controller);
}

void CGameObject::setSceneObject(CSceneObjectProxy *proxy)
{
    assert(proxy != nullptr);
    m_sceneObject.reset(proxy);
    // Publish the object's real spawn pose immediately, before its first simulation tick.
    m_sceneObject->setPosition(m_position);
    m_sceneObject->setRotation(m_orientation);
    m_sceneObject->setScale(m_scale);
}

const glm::vec3 &CGameObject::getPosition() const
{
    return m_position;
}

const glm::vec3 &CGameObject::getScale() const
{
    return m_scale;
}

const glm::vec3 &CGameObject::getForward() const
{
    return m_forward;
}

void CGameObject::update(float dtime)
{
    // Update object controllers
    for (auto &controller : m_controllers)
    {
        if (m_deleteRequested)
            break;
        controller->update(dtime);
    }

    // Update attached objects if tranformation of the game object has changed
    if (m_transformationChanged)
    {

        // Update scene object
        if (m_sceneObject != nullptr)
        {
            // TODO Slow?
            m_sceneObject->setPosition(getPosition());
            m_sceneObject->setRotation(m_orientation);
            m_sceneObject->setScale(getScale());
        }
        // Update collidable
        if (hasCollidable())
        {
            m_collidable->setTransform(getPosition(), getOrientation(), getScale());
        }
    }
}

void CGameObject::markDeleted()
{
    m_deleteRequested = true;
    if (m_sceneObject != nullptr)
    {
        m_sceneObject->setVisibility(false);
    }
    if (hasCollidable())
    {
        m_collidable->markDeleted();
    }
}

bool CGameObject::isDeleteRequested() const
{
    return m_deleteRequested;
}

void CGameObject::sendMessage(Message message)
{
    for (auto &controller : m_controllers)
    {
        controller->receiveMessage(message);
    }
}

void CGameObject::setCollidable(CCollidable *entity)
{
    assert(entity != nullptr);
    if (m_collidable != entity)
    {
        m_collidable = entity;
        // Update
        m_collidable->setTransform(getPosition(), getOrientation(), getScale());
    }
}

CCollidable *CGameObject::getCollidable() const
{
    assert(m_collidable != nullptr);
    return m_collidable;
}

bool CGameObject::hasCollidable() const
{
    return m_collidable != nullptr;
}

bool CGameObject::isDead() const
{
    return m_dead;
}

void CGameObject::setDead()
{
    m_dead = true;
}
void CGameObject::setOrientation(const glm::quat &orientation)
{
    m_orientation = glm::normalize(orientation);
    m_rotation = glm::eulerAngles(m_orientation); // Compatibility for legacy enemy controllers.
    m_forward = m_orientation * glm::vec3(0, 0, 1);
    m_transformationChanged = true;
}
const glm::quat &CGameObject::getOrientation() const
{
    return m_orientation;
}

void CGameObject::setVelocity(glm::vec3 velocity)
{
    m_velocity = velocity;
}
const glm::vec3 &CGameObject::getVelocity() const
{
    return m_velocity;
}
void CGameObject::setTargetName(std::string name)
{
    m_targetName = std::move(name);
}
const std::string &CGameObject::getTargetName() const
{
    return m_targetName;
}

void CGameObject::setProjectile(bool projectile)
{
    m_projectile = projectile;
}
bool CGameObject::isProjectile() const
{
    return m_projectile;
}

void CGameObject::setVisible(bool visible)
{
    if (m_sceneObject)
        m_sceneObject->setVisibility(visible);
}
