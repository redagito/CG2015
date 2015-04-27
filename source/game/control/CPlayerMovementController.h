#pragma once

#include "game/IController.h"
#include "game/Message.h"

/**
* \brief Player side movement.
*/
class CPlayerMovementController : public IController
{
public:
	CPlayerMovementController(float speedSide);

	/**
	* \brief Destructor for cleanup.
	*/
	~CPlayerMovementController();

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
	void reciveMessage(Message* msg);

private:
	CGameObject* m_object; /**< Controlled game object. */
	float m_speedSide; /**< Side movement speed. */
	float m_rotationDegree; /**< Current rotation in degree. */
	bool m_active; /**< Active state flag. */
};