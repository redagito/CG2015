#include "CCameraController.h"

#include "game/CGameObject.h"

CCameraController::CCameraController(IControllableCamera* camera)
	:
	m_camera(camera)
{
	return;
}

CCameraController::~CCameraController()
{
	
}

void CCameraController::attach(CGameObject* object)
{
	m_object = object;
}

void CCameraController::detach()
{
	m_object = nullptr;
	m_camera = nullptr;
}

void CCameraController::update(float dtime)
{
	if (m_active && m_object != nullptr && m_camera != nullptr)
	{
		// Update camera movement
		glm::vec3 pos = m_object->getPosition();
		glm::vec3 forward = m_object->getForward();

		// Where the camera should be
		glm::vec3 camPos(pos.x - forward.x * 15.f, pos.y * 0.9f + 5.f, pos.z - forward.z * 15.f);

		m_camera->setPosition(camPos);
		m_camera->lookAt(pos + forward * 5.f, glm::vec3(0.f, 1.f, 0.f));
	}
}

void CCameraController::setActive(bool state)
{
	m_active = state;
}

void CCameraController::receiveMessage(Message msg) 
{
	
}