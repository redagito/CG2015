#include "CWeaponController.h"

#include "game/CGameObject.h"

CWeaponController::CWeaponController()
{
	return;
}

CWeaponController::~CWeaponController()
{

}

void CWeaponController::attach(CGameObject* object)
{
	m_object = object;
}

void CWeaponController::detach()
{
	m_object = nullptr;
}

void CWeaponController::update(float dtime)
{
	if (m_active && m_object != nullptr)
	{
		
	}
}

void CWeaponController::setActive(bool state)
{
	m_active = state;
}

void CWeaponController::receiveMessage(Message msg)
{

}