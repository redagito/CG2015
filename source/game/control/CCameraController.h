#pragma once

class CCamera;

#include "game/IController.h"
#include "game/Message.h"

/**
* \brief Controls camera movement based on the attached game object.
*/
class CCameraController : public IController
{
public:
	/**
	* \brief Sets camera.
	*/
	CCameraController(CCamera* camera);

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
	void reciveMessage(Message* msg);

private:
	CCamera* m_camera; /**< Controlled camera. */
	CGameObject* m_object; /**< Controlled game object. */
	bool m_active; /**< Active state flag. */
};