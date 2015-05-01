#include "CPlayerMovementController.h"

#include "game/CGameObject.h"
#include "input\IInputProvider.h"
#include "util/Global.h"

#include "glfw/glfw3.h"

#include <glm/ext.hpp>


CPlayerMovementController::CPlayerMovementController(float speedSide)
:
m_speedSide(speedSide),
m_active(true),
m_object(nullptr),
m_rotationDegree(0.f)
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

void CPlayerMovementController::update(float mtime)
{
	if (m_active && m_object != nullptr)
	{
		// Rotation speed by degree / sec
		float rateOfRotation = 300.f;
		// Back rotation when no keys are pressed
		float backRotation = 500.f;

		// X axis boundary for movement
		float minX = -20.f;
		float maxX = 20.f;
		// Y axis boundary for movement
		float minY = 3.f;
		float maxY = 20.f;

		glm::vec4 temp = (m_object->getTranslation() * glm::vec4(0.f, 0.f, 0.f, 1.f));
		glm::vec3 pos = glm::vec3(temp.x / temp.w, temp.y / temp.w, temp.z / temp.w);

		// Movement vector
		glm::vec3 dPos(0.f);

		// Move up
		if (glfwGetKey(Global::window, GLFW_KEY_UP))
		{
			// Y new position
			dPos.y = mtime * m_speedSide;
			if (dPos.y + pos.y > maxY)
			{
				// Move to limit
				dPos.y = maxY - pos.y;
			}
		}

		// Move down
		if (glfwGetKey(Global::window, GLFW_KEY_DOWN))
		{
			// Y new position
			dPos.y = -mtime * m_speedSide;
			if (dPos.y + pos.y < minY)
			{
				// Move to limit
				dPos.y = minY - pos.y;
			}
		}

		// Rotate left
		if (glfwGetKey(Global::window, GLFW_KEY_RIGHT))
		{
			m_rotationDegree += mtime * rateOfRotation;
			if (m_rotationDegree > 90.f)
			{
				m_rotationDegree = 90.f;
			}
		}
		else if (m_rotationDegree > 0.f)
		{
			m_rotationDegree -= mtime * backRotation;
			if (m_rotationDegree < 0.f)
			{
				m_rotationDegree = 0.f;
			}
		}

		// Rotate right
		if (glfwGetKey(Global::window, GLFW_KEY_LEFT))
		{
			m_rotationDegree -= mtime * rateOfRotation;
			if (m_rotationDegree < -90.f)
			{
				m_rotationDegree = -90.f;
			}
		}
		else if (m_rotationDegree < 0.f)
		{
			m_rotationDegree += mtime * backRotation;
			if (m_rotationDegree > 0.f)
			{
				m_rotationDegree = 0.f;
			}
		}

		// Movement speed based on angle
		float dx = -m_rotationDegree * m_speedSide;

		// Restrict movement
		if (pos.x > minX && dx < 0.f)
		{
			dPos.x = -std::sqrt(-dx) * mtime;
			if (dPos.x + pos.x < minX)
			{
				dPos.x = minX - pos.x;
			}
		}
		else if (pos.x < maxX && dx > 0.f)
		{
			dPos.x = std::sqrt(dx) * mtime;
			if (dPos.x + pos.x > maxX)
			{
				dPos.x = maxX - pos.x;
			}
		}

		// Update rotation
		m_object->setRotation(glm::rotate(m_rotationDegree, glm::vec3(0.f, 0.f, 1.f)));

		// Update translation
		m_object->setTranslation(glm::translate(m_object->getTranslation(), dPos));
	}
}