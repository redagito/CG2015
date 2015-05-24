#pragma once

#include <glm/glm.hpp>

#include "graphics/IScene.h"

class CSceneObjectProxy
{
public:
	CSceneObjectProxy();
	CSceneObjectProxy(IScene* scene, SceneObjectId id);
	
	bool init(IScene* scene, SceneObjectId id);

	const glm::vec3& getPosition() const;
	const glm::quat& getRotation() const;
	const glm::vec3& getScale() const;

	void setPosition(const glm::vec3& position);
	void setRotation(const glm::vec3& rotation);
	void setRotation(const glm::quat& rotation);
	void setScale(const glm::vec3& scale);

private:
	void getUpdate() const;
	void sendUpdate();

	mutable ResourceId m_mesh;
	mutable ResourceId m_material;
	mutable glm::vec3 m_position;
	mutable glm::quat m_rotation;
	mutable glm::vec3 m_scale;

	bool m_init = false;
	IScene* m_scene = nullptr;
	SceneObjectId m_objectId = invalidObject;
};