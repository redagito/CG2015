#include <cmath>

#include "CBoundingSphere.h"

void CBoundingSphere::setPosition(const glm::vec3& position)
{
	m_position = position;
}

void CBoundingSphere::setRadius(float radius)
{
	m_radius = radius;
}

const glm::vec3& CBoundingSphere::getPosition() const
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

bool collides(const CBoundingSphere& sphere1, const CBoundingSphere& sphere2)
{
	float distance = glm::distance(sphere1.getPosition(), sphere2.getPosition());
	return distance < (sphere1.getRadius() + sphere2.getRadius());
}