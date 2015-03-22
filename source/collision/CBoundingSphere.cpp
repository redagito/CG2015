#include "CBoundingSphere.h";

void CBoundingSphere::setPosition(const glm::vec3& position)
{
	m_position = position;
}

void CBoundingSphere::setRadius(float radius)
{
	m_radius = radius;
}

const glm::vec3& CBoundingSphere::setPosition() const
{
	return m_position;
}

float CBoundingSphere::getRadius() const
{
	return m_radius;
}

CBoundingSphere CBoundingSphere::create(const std::vector<float>& vertices)
{
	// TODO Implement
	CBoundingSphere sphere;
	return sphere;
}