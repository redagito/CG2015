#include "SSceneObject.h"

SSceneObject::SSceneObject()
{
    return;
}

SSceneObject::SSceneObject(ResourceId mesh, ResourceId material, const glm::vec3& position,
	const glm::quat& rotation, const glm::vec3& scale)
	: m_mesh(mesh), m_material(material), m_position(position), m_rotation(rotation), m_scale(scale)
{
	return;
}

SSceneObject::SSceneObject(ResourceId model, const glm::vec3& position,
	const glm::quat& rotation, const glm::vec3& scale)
	: m_model(model), m_position(position), m_rotation(rotation), m_scale(scale)
{
	return;
}