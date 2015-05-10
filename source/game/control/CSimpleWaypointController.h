#pragma once

#include <glm/glm.hpp>

#include "game/IController.h"
#include "game/Message.h"

/**
* \brief Point to point movement.
*/
class CSimpleWaypointController : public IController
{
public:
	CSimpleWaypointController(const glm::vec3& start, const glm::vec3& end, float speed);

	/**
	* \brief Destructor for cleanup.
	*/
	~CSimpleWaypointController();

	/**
	* \brief On attach to object.
	*/
	void attach(CGameObject* object);

	/**
	* \brief On dettaching from game object.
	*/
	void detach();

	/**
	* \brief Updates controller.
	*/
	void update(float dtime);

	/**
	* \brief Sets active state.
	*/
	void setActive(bool state);

	/**
	* \brief Racive state from other objects
	*/
	void receiveMessage(Message msg);

private:
	CGameObject* m_object = nullptr; /**< Controlled game object. */
	float m_speed = 0.f; /**< Side movement speed. */
	bool m_active = true; /**< Active state flag. */
    glm::vec3 m_start;
	glm::vec3 m_end;
};