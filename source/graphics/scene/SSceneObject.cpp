#include <algorithm>

#include "SSceneObject.h"

SSceneObject::SSceneObject()
{
    return;
}

SSceneObject::SSceneObject(ResourceId mesh, ResourceId material, const glm::vec3& position,
	const glm::quat& rotation, const glm::vec3& scale, bool visible, const CBoundingSphere& sphere)
	: m_mesh(mesh), m_material(material), m_position(position), m_rotation(rotation), m_scale(scale), m_visible(visible), m_localBoundingSphere(sphere)
{
	updateBoundingSphere();
	return;
}

SSceneObject::SSceneObject(ResourceId model, const glm::vec3& position,
	const glm::quat& rotation, const glm::vec3& scale, bool visible, const CBoundingSphere& sphere)
	: m_model(model), m_position(position), m_rotation(rotation), m_scale(scale), m_visible(visible), m_localBoundingSphere(sphere)
{
	updateBoundingSphere();
	return;
}

void SSceneObject::updateBoundingSphere()
{
	// Translate
    boundingSphere = m_localBoundingSphere;
	boundingSphere.setPosition(m_position + m_rotation * (boundingSphere.getPosition() * m_scale));
	// Scale sphere by highest scaling factor
	const auto magnitude = glm::abs(m_scale);
	boundingSphere.setRadius(boundingSphere.getRadius() * std::max({magnitude.x, magnitude.y, magnitude.z}));
}
