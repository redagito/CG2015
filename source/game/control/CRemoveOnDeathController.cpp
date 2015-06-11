#include "CRemoveOnDeathController.h"

#include "game/CGameObject.h"



CRemoveOnDeathController::CRemoveOnDeathController()
:
m_object(nullptr),
m_active(true)
{
	return;
}

CRemoveOnDeathController::~CRemoveOnDeathController()
{
	return;
}

void CRemoveOnDeathController::attach(CGameObject* object)
{
	m_object = object;
}

void CRemoveOnDeathController::detach()
{
	m_object = nullptr;
}

void CRemoveOnDeathController::update(float dtime)
{
	if (m_object != nullptr && m_active && m_object->isDead())
	{
		// Mark for deletion
		m_object->markDeleted();
	}
}

void CRemoveOnDeathController::setActive(bool state)
{
	m_active = state;
}

void CRemoveOnDeathController::receiveMessage(Message msg)
{

}