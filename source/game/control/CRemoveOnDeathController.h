#pragma once

#include "game/IController.h"

#include <string>

class CGamePlayState;

/**
* \brief Checks the object state for death state and trigger explosion effect.
* TODO This should actually react to an onDeath event.
* TODO This actually can play any sound on death.
*/
class CRemoveOnDeathController : public IController
{
public:
	/**
	* \brief Sets sound system and sound name.
	*/
	CRemoveOnDeathController(CGamePlayState* state);

	/**
	* \brief Destructor for cleanup.
	*/
	~CRemoveOnDeathController();

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

	void setActive(bool active);

	void receiveMessage(Message msg);

private:
	CGameObject* m_object; /**< Controlled game object. */
	bool m_active;
	CGamePlayState* m_gameState = nullptr;
};