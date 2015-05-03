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
	return;
}

void CGameObject::markDeleted()
{
	m_deleteRequested = true;
}

bool CGameObject::isDeleteRequested() const
{
	return m_deleteRequested;
}

void CGameObject::sendMessage(Message message) {
	for (auto& controller : m_controllers) 
	{
		controller->receiveMessage(message);
	}
}