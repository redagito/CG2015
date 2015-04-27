#include <cassert>

#include "CGameObject.h"

// Constructor initializes drawable and matrices
CGameObject::CGameObject()
:
m_translation(glm::mat4(1.f)), // Transformation matrices
m_rotation(glm::mat4(1.f)),
m_scale(glm::mat4(1.f)),
m_transformationChanged(false) // Dirty flag for lazy update
{
	return;
}

CGameObject::~CGameObject()
{
	return;
}


void CGameObject::setRotation(const glm::mat4& rotation)
{
	m_rotation = rotation;
	m_transformationChanged = true;
}

void CGameObject::setTranslation(const glm::mat4& translation)
{
	m_translation = translation;
	m_transformationChanged = true;
}

void CGameObject::setScale(const glm::mat4& scale)
{
	m_scale = scale;
	m_transformationChanged = true;
}

const glm::mat4& CGameObject::getRotation() const
{
	return m_rotation;
}

const glm::mat4& CGameObject::getTranslation() const
{
	return m_translation;
}

const glm::mat4& CGameObject::getScale() const
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

void CGameObject::sendMessage(Message* msg) {
	for (auto contr: m_controllers) {
		//contr->reciveMessage(msg);
	}
}