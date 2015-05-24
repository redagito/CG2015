#include "CCollidable.h"

CCollidable::CCollidable(unsigned int group, const CAABBox& box)
:
m_groupId(group),
m_damageDealt(0.f),
m_damageReceived(0.f),
m_box(box),
m_boxTransformed(box),
m_deleteMe(false),
m_collidable(true)
{
	return;
}

float CCollidable::getDamage() const
{
	return m_damageDealt;
}

// Returns accumuated damage and resets damage counter
float CCollidable::getDamageReceived()
{
	float temp = m_damageReceived;
	m_damageReceived = 0.f;
	return temp;
}

// Returns group id
unsigned int CCollidable::getGroupId() const
{
	return m_groupId;
}

// Add damage to the internal counter
void CCollidable::receiveDamage(float damage)
{
	m_damageReceived += damage;
}

// Sets the damage dealt by this entity
void CCollidable::setDamage(float damage)
{
	m_damageDealt = damage;
}

const CAABBox& CCollidable::getAABBox() const
{
	return m_boxTransformed;
}

void CCollidable::setScale(const glm::vec3& scale)
{
	m_boxTransformed.setHalfWidths(scale);
	return;
}

void CCollidable::setTranslation(const glm::vec3& translate)
{
	m_boxTransformed.setMid(translate);
	return;
}

// Sets collidable state
void CCollidable::setCollidable(bool state)
{
	m_collidable = state;
}

// Returns collidable state
bool CCollidable::isCollidable() const
{
	return m_collidable;
}

bool CCollidable::deleteRequested() const
{
	return m_deleteMe;
}

void CCollidable::markDeleted()
{
	m_deleteMe = true;
}