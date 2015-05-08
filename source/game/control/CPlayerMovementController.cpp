#include "CPlayerMovementController.h"

#include "game/CGameObject.h"
#include "input/IInputProvider.h"
#include "util/Global.h"
#include "math/TransformUtils.h"

#include "glfw/glfw3.h"

#include <glm/ext.hpp>


CPlayerMovementController::CPlayerMovementController(IInputProvider* inputProvider, float speedSide)
	:
	m_inputProvider(inputProvider),
	m_speedSide(speedSide)
{
	return;
}

CPlayerMovementController::~CPlayerMovementController()
{
	return;
}

void CPlayerMovementController::attach(CGameObject* object)
{
	m_object = object;
}

void CPlayerMovementController::detach()
{
	m_object = nullptr;
	m_rotationDegree = 0.f;
}

void CPlayerMovementController::setActive(bool state)
{
	m_active = state;
}

void CPlayerMovementController::update(float dtime)
{
	
	if (m_active && m_object != nullptr)
	{

		// Moving forward
		float forward = dtime * 10.f;
	
		// Rotation speed by deg / sec
		float rateOfRotation = 200.f;
		// Back rotation rate when no keys are pressed
		float backRotation = 500.f;

		// Y axis boundary for movement
		float minY = 5.f;
		float maxY = 20.f;

		glm::vec3 pos = m_object->getPosition();

		// Movement vector
		glm::vec3 dPos(0.f);

		// Move up
		if (m_inputProvider->isKeyPressed(GLFW_KEY_W))
		{
			// Y new position
			dPos.y = dtime * m_speedSide;
			if (dPos.y + pos.y > maxY)
			{
				// Move to limit
				dPos.y = maxY - pos.y;
			}
		}

		// Move down
		if (m_inputProvider->isKeyPressed(GLFW_KEY_S))
		{
			// Y new position
			dPos.y = -dtime * m_speedSide;
			if (dPos.y + pos.y < minY)
			{
				// Move to limit
				dPos.y = minY - pos.y;
			}
		}

		// Rotate left
		if (m_inputProvider->isKeyPressed(GLFW_KEY_D))
		{
			m_object->setPosition(glm::vec3(m_object->getPosition().x - forward, m_object->getPosition().y, m_object->getPosition().z - forward));
			m_rotationDegree += dtime * rateOfRotation;
			if (m_rotationDegree > 90.f)
			{
				
				m_rotationDegree = 90.f;
			}
		}
		else if (m_rotationDegree > 0.f)
		{
			m_rotationDegree -= dtime * backRotation;
			if (m_rotationDegree < 0.f)
			{
				m_rotationDegree = 0.f;
			}
		}

		// Rotate right
		if (m_inputProvider->isKeyPressed(GLFW_KEY_A))
		{
			m_object->setPosition(glm::vec3(m_object->getPosition().x + forward, m_object->getPosition().y, m_object->getPosition().z - forward));
			m_rotationDegree -= dtime * rateOfRotation;
			if (m_rotationDegree < -90.f)
			{
				m_rotationDegree = -90.f;
			}
		}
		else if (m_rotationDegree < 0.f)
		{
			m_rotationDegree += dtime * backRotation;
			if (m_rotationDegree > 0.f)
			{
				m_rotationDegree = 0.f;
			}
		}

		// Movement speed based on angle
		float dx = -m_rotationDegree * m_speedSide;

		if (dx < 0.f)
		{
			dPos.x = -std::sqrt(-dx) * dtime;
		}
		else if (dx > 0.f)
		{
			dPos.x = std::sqrt(dx) * dtime;
		}

		float rotationRad = m_rotationDegree * glm::pi<float>() / 180.f;

		// Update rotation
		m_object->setRotation(glm::vec3(m_object->getRotation().x, -rotationRad, m_object->getRotation().z));
		
		/*m_object->setRotation(glm::vec3(0.f, 0.f, rotationRad));*/

		// Update translation
		dPos.z += 10.f * dtime;
		m_object->setPosition(m_object->getPosition() + dPos);

	}
}

void CPlayerMovementController::receiveMessage(Message msg)
{

}

void CPlayerMovementController::pitch(float amount)
{
	m_up = TransformUtils::rotate(m_right, amount, m_up);
	m_forward = TransformUtils::rotate(m_right, amount, m_forward);
}

void CPlayerMovementController::roll(float amount)
{
	m_up = TransformUtils::rotate(m_forward, amount, m_up);
	m_right = TransformUtils::rotate(m_forward, amount, m_right);
}

void CPlayerMovementController::yaw(float amount)
{
	m_right = TransformUtils::rotate(m_up, amount, m_right);
	m_forward = TransformUtils::rotate(m_up, amount, m_forward);
}