#include "CWeaponController.h"

#include "game/CGameObject.h"
#include "CLinearMovementController.h"

#include <GLFW/glfw3.h>

CWeaponController::CWeaponController(IInputProvider* provider, CGameWorld* gameWorld, IScene* scene, ResourceId mesh, ResourceId material)
	:
	m_inputProvider(provider),
	m_gameWorld(gameWorld),
	m_scene(scene),
	m_mesh(mesh),
	m_material(material)
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
		m_weaponCooldown -= dtime;
		// Check for space key
		if (m_inputProvider->isKeyPressed(GLFW_KEY_SPACE) && m_weaponCooldown <= 0.f)
		{
			// Reset cooldown
			m_weaponCooldown = 0.3f;

			glm::vec3 direction = m_object->getForward();
			glm::vec3 position = m_object->getPosition();
			
			// Create game object
			CGameObject* bullet = new CGameObject;
			bullet->addController(std::make_shared<CLinearMovementController>(direction, 20.f));
			bullet->setPosition(position + direction * 2.f);
			bullet->setRotation(m_object->getRotation());
			
			// Create scene proxy
			CSceneObjectProxy* proxy = new CSceneObjectProxy(m_scene, m_scene->createObject(m_mesh, m_material, glm::vec3(0.f), glm::vec3(0.f), glm::vec3(1.f)));
			bullet->setSceneObject(proxy);
			
			// Add to world
			m_gameWorld->addObject(bullet);
		}
	}
}

void CWeaponController::setActive(bool state)
{
	m_active = state;
}

void CWeaponController::receiveMessage(Message msg)
{

}