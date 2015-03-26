#include "CPlane.h"

CPlane::CPlane(glm::vec3 &v1, glm::vec3 &v2, glm::vec3 &v3) {

	set3Points(v1, v2, v3);
}

CPlane::CPlane() {}

CPlane::~CPlane() {}

void CPlane::set3Points(const glm::vec3 &v1, const glm::vec3 &v2, const glm::vec3 &v3) {

	glm::vec3 aux1;
	glm::vec3 aux2;

	aux1 = v1 - v2;
	aux2 = v3 - v1;

	normal = aux1 * aux2;

	normal = glm::normalize(normal);
	point = v2;
	d = -(glm::dot(normal, point));

}

void CPlane::setNormalAndPoint(glm::vec3 &normal, glm::vec3 &point) {

	this->normal = normal;
	this->normal = glm::normalize(this->normal);
	d = -(glm::dot(this->normal, point));

}

void CPlane::setCoefficients(float a, float b, float c, float d) {

	// set the normal vector
	normal.x = a;
	normal.y = b;
	normal.z = c;
	//compute the lenght of the vector
	float l = glm::length(normal);
	// normalize the vector
	normal.x = a / l;
	normal.y = b / l;
	normal.z = c / l;
	// and divide d by th length as well
	this->d = d / l;
}

float CPlane::distance(glm::vec3 &p) {


	return d + glm::dot(normal, p);
	
	
}


