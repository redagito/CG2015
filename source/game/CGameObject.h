#pragma once

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

protected:
	bool m_deleteRequested = false; /**< Deletion of this object is requested. */
};
