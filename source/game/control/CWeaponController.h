#pragma once

#include "game/IController.h"
#include "game/Message.h"
#include "input/IInputProvider.h"
#include "game/CGameWorld.h"
#include "graphics/IScene.h"

/**
* \brief Controls weapons, creates projectiles.
*/
class CWeaponController : public IController
{
public:
	CWeaponController(IInputProvider* provider, CGameWorld* gameWorld, IScene* scene, ResourceId mesh, ResourceId material);
	~CWeaponController();

	void attach(CGameObject* object);
	void detach();
	void update(float dtime);
	void setActive(bool state);
	void receiveMessage(Message msg);

private:
	CGameObject* m_object = nullptr; /**< Controlled game object. */
	bool m_active = true; /**< Active state flag. */
	IInputProvider* m_inputProvider = nullptr;
	float m_weaponCooldown = 0.f;
	CGameWorld* m_gameWorld = nullptr;
	IScene* m_scene = nullptr;
	ResourceId m_mesh = invalidResource;
	ResourceId m_material = invalidResource;
};