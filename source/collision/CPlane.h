#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>

class CPlane {
	
public:
	glm::vec3 normal;
	glm::vec3 point;
	float d;

	CPlane::CPlane(glm::vec3 &v1, glm::vec3 &v2, glm::vec3 &v3);
	CPlane::CPlane();
	CPlane::~CPlane();

	void set3Points(glm::vec3 &v1, glm::vec3 &v2, glm::vec3 &v3);
	void setNormalAndPoint(glm::vec3 &normal, glm::vec3 &point);
	void setCoefficients(float a, float b, float c, float d);
	float distance(glm::vec3 &p);

};