#pragma once

#include <vector>
#include <glm/glm.hpp>

class CBoundingSphere
{
public:
	void setPosition(const glm::vec3& position);
	void setRadius(float radius);

	const glm::vec3& setPosition() const;
	float getRadius() const;

	static CBoundingSphere create(const std::vector<float>& vertices);

private:
	glm::vec3 m_position = glm::vec3(0.f);
	float m_radius = 0.f;
};