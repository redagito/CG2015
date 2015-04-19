#include <cassert>

#include "CGameObject.h"

// Constructor initializes drawable and matrices
CGameObject::CGameObject()
{
	return;
}

CGameObject::~CGameObject()
{
	return;
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

void CGameObject::sendMessage() {
	for (auto contr: m_controllers) {
		//contr.reciveMessage(...);
	}
}