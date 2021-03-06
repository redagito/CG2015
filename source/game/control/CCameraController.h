#pragma once

#include <glm/glm.hpp>

#include "game/IController.h"
#include "game/Message.h"

#include "graphics/camera/IControllableCamera.h"

/**
* \brief Controls camera movement based on the attached game object.
*/
class CCameraController : public IController
{
public:
	/**
	* \brief Sets camera.
	*/
	CCameraController(IControllableCamera* camera);

	/**
	* \brief Destructor for cleanup.
	*/
	~CCameraController();

	/**
	* \brief On attach to object.
	*/
	void attach(CGameObject* object);

	/**
	* \brief On detaching from game object.
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
	IControllableCamera* m_camera = nullptr; /**< Controlled camera. */
	CGameObject* m_object = nullptr; /**< Controlled game object. */
	bool m_active = true; /**< Active state flag. */
	glm::vec3 m_prevLookAt;
	bool m_first = true;
	float m_speed = 5.f;
};