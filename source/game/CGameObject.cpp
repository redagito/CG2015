#include <cassert>

#include "CGameObject.h"
#include "IController.h"

// Constructor initializes drawable and matrices
CGameObject::CGameObject()
{
	return;
}

CGameObject::~CGameObject()
{
	return;
}


void CGameObject::setRotation(const glm::vec3& rotation)
{
	m_rotation = rotation;
	m_transformationChanged = true;
}

void CGameObject::setPosition(const glm::vec3& position)
{
	m_position = position;
	m_transformationChanged = true;
}

void CGameObject::setScale(const glm::vec3& scale)
{
	m_scale = scale;
	m_transformationChanged = true;
}

const glm::vec3& CGameObject::getRotation() const
{
	return m_rotation;
}

void CGameObject::addController(const std::shared_ptr<IController>& controller)
{
	assert(controller != nullptr);
	controller->attach(this);
	m_controllers.push_back(controller);
}

void CGameObject::setSceneObject(CSceneObjectProxy* proxy)
{
	assert(proxy != nullptr);
	m_sceneObject.reset(proxy);
}

const glm::vec3& CGameObject::getPosition() const
{
	return m_position;
}

const glm::vec3& CGameObject::getScale() const
{
	return m_scale;
}

void CGameObject::update(float dtime)
{
	// Update object controllers
	for (auto& controller : m_controllers)
	{
		controller->update(dtime);
	}
	// Update scene object
	if (m_transformationChanged)
	{
		// TODO Slow?
		m_sceneObject->setPosition(getPosition());
		m_sceneObject->setRotation(getRotation());
		m_sceneObject->setScale(getScale());
	}
}

void CGameObject::markDeleted()
{
	m_deleteRequested = true;
}

bool CGameObject::isDeleteRequested() const
{
	return m_deleteRequested;
}

void CGameObject::sendMessage(Message message) 
{
	for (auto& controller : m_controllers) 
	{
		controller->receiveMessage(message);
	}
}