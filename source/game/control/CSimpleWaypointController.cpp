#include "CSimpleWaypointController.h"

#include "game/CGameObject.h"
#include "math/TransformUtils.h"

#include "glfw/glfw3.h"

#include <glm/ext.hpp>


CSimpleWaypointController::CSimpleWaypointController(const glm::vec3& start, const glm::vec3& end, float speed)
:
m_start(start),
m_end(end),
m_speed(speed)
{
	return;
}

CSimpleWaypointController::~CSimpleWaypointController()
{
	return;
}

void CSimpleWaypointController::attach(CGameObject* object)
{
	m_object = object;
}

void CSimpleWaypointController::detach()
{
	m_object = nullptr;
	
}

void CSimpleWaypointController::setActive(bool state)
{
	m_active = state;
}

void CSimpleWaypointController::update(float dtime)
{

	if (m_active && m_object != nullptr)
	{

		if (m_start.x <= m_end.x) {
			m_start.x += m_speed * dtime;
		}
		if (m_start.y <= m_end.y) {
			m_start.y += m_speed * dtime;
		}
		if (m_start.z <= m_end.z) {
			m_start.z += m_speed * dtime;
		}

		if (m_start.x >= m_end.x) {
			m_start.x -= m_speed * dtime;
		}
		if (m_start.y >= m_end.y) {
			m_start.y -= m_speed * dtime;
		}
		if (m_start.z >= m_end.z) {
			m_start.z -= m_speed * dtime;
		}

		m_object->setPosition(glm::vec3(m_start.x, m_start.y, m_start.z));
	}
	
}

void CSimpleWaypointController::receiveMessage(Message msg)
{

}