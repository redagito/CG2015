#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>

class CPlane {
	
private:
	glm::vec3 normal;
	glm::vec3 point;
	float d;

public:
	CPlane(glm::vec3 &v1, glm::vec3 &v2, glm::vec3 &v3);
	CPlane();
	~CPlane();

	// Ordering of points defines normal vector
	// Clockwise/Counterclockwise ordering rule is used to calculate normal vector
	void set3Points(const glm::vec3 &v1, const glm::vec3 &v2, const glm::vec3 &v3);

	void setNormalAndPoint(glm::vec3 &normal, glm::vec3 &point);
	void setCoefficients(float a, float b, float c, float d);
	float distance(glm::vec3 &p);

};