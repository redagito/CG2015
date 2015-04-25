#pragma once

class IController;

#include <glm/glm.hpp>
#include "Message.h"

#include <list>
#include <memory>

/**
* \brief Game object storing relevant data.
*/
class CGameObject
{
public:
	/**
	* \brief Default constructor.
	*/
	CGameObject();

	/**
	* \brief Resource cleanup.
	*/
	~CGameObject();

	/**
	* \brief Updates object and returns state
	*
	* Updates the object with time in seconds.
	*/
	void update(float dtime);

	/**
	* \brief Marks this object for deletion.
	*/
	void markDeleted();

	/**
	* \brief Returns delete request flag.
	* True if object should be deleted.
	*/
	bool isDeleteRequested() const;

	/**
	* \brief Send state change messages to all controllers in Object
	*/
	void sendMessage(Message* msg);
	

protected:
	bool m_deleteRequested = false; /**< Deletion of this object is requested. */
	std::list<std::shared_ptr<IController>> m_controllers; /**Controller objects. */
};
