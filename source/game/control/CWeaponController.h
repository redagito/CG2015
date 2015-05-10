#pragma once

#include "game/IController.h"
#include "game/Message.h"

/**
* \brief Controls weapons, creates projectiles.
*/
class CWeaponController : public IController
{
public:
	CWeaponController();
	~CWeaponController();

	void attach(CGameObject* object);
	void detach();
	void update(float dtime);
	void setActive(bool state);
	void receiveMessage(Message msg);

private:
	CGameObject* m_object = nullptr; /**< Controlled game object. */
	bool m_active = true; /**< Active state flag. */
};