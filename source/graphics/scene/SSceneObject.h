#pragma once

#include <glm/glm.hpp>

#include "resource/ResourceConfig.h"

/**
* \brief Scene object data struct.
*/
struct SSceneObject
{
	SSceneObject();
	SSceneObject(ResourceId mesh, ResourceId material, const glm::vec3& position,
		const glm::vec3& rotation, const glm::vec3& scale);

	SSceneObject(ResourceId model, const glm::vec3& position,
		const glm::vec3& rotation, const glm::vec3& scale);

    ResourceId m_mesh = invalidResource;
	ResourceId m_material = invalidResource;
	ResourceId m_model = invalidResource;
	glm::vec3 m_position = glm::vec3(0.f);
	glm::vec3 m_rotation = glm::vec3(0.f);
	glm::vec3 m_scale = glm::vec3(1.f);
};