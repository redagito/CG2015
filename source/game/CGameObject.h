#pragma once

#include <list>
#include <memory>

#include <glm/glm.hpp>

#include "graphics/proxy/CSceneObjectProxy.h"

#include "Message.h"

class IController;

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
	* \brief Sets rotation matrix.
	*/
	void setRotation(const glm::mat4& rotation);

	/**
	* \brief Sets translation matrix.
	*/
	void setTranslation(const glm::mat4& translation);

	/**
	* \brief Sets scale matrix.
	*/
	void setScale(const glm::mat4& scale);

	/**
	* \brief Set scene object.
	*/
	void setSceneObject(CSceneObjectProxy* proxy);

	/**
	* \brief Returns rotation matrix
	*/
	const glm::mat4& getRotation() const;

	/**
	* \brief Returns translation matrix
	*/
	const glm::mat4& getTranslation() const;

	/**
	* \brief Returns scale matrix
	*/
	const glm::mat4& getScale() const;

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
	void sendMessage(Message msg);

protected:
	std::unique_ptr<CSceneObjectProxy> m_sceneObject; /**< Scene object. */
	glm::mat4 m_rotation; /**< Rotation matrix. */
	glm::mat4 m_translation; /**< Translation matrix. */
	glm::mat4 m_scale; /**< Scale matrix. */
	bool m_transformationChanged;
	bool m_deleteRequested = false; /**< Deletion of this object is requested. */
	std::list<std::shared_ptr<IController>> m_controllers; /**Controller objects. */
};
