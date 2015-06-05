#include "CFrustum.h"

#include <math.h>

#include <glm/ext.hpp>

const float gradToRad = glm::pi<float>() / 180.0f;

CFrustum::CFrustum() {}

CFrustum::~CFrustum() {}

void CFrustum::setFromCameraParameters(float angle, float ratio, float nearD, float farD, glm::vec3 &pos, glm::vec3 &look, glm::vec3 &u) 
{
	// Compute width and height of the near and far plane sections
	float tang = tan(angle* gradToRad * 0.5f);
	// Near plane height
	float nh = nearD * tang;
	// Near plane width
	float nw = nh * ratio;
	// Far plane height
	float fh = farD  * tang;
	// Far plane width
	float fw = fh * ratio;

	// Compute the Z axis of camera
	// This axis points in the opposite direction from the looking direction.
	glm::vec3 z = glm::normalize(pos - look);

	// X axis of camera with given "up" vector and Z axis
	glm::vec3 x = glm::normalize(u * z);
	
	// The real "up" vector is the cross product of Z and X
	glm::vec3 y = glm::cross(z, x);

	// Compute the centers of the near and far planes
	// Near center
	glm::vec3 nc = pos - z * nearD;
	// Far center
	glm::vec3 fc = pos - z * farD;

	// Compute the 4 corners of the frustum on the near plane
	glm::vec3 ntl = nc + y * nh - x * nw;
	glm::vec3 ntr = nc + y * nh + x * nw;
	glm::vec3 nbl = nc - y * nh - x * nw;
	glm::vec3 nbr = nc - y * nh + x * nw;

	// Compute the 4 corners of the frustum on the far plane
	glm::vec3 ftl = fc + y * fh - x * fw;
	glm::vec3 ftr = fc + y * fh + x * fw;
	glm::vec3 fbl = fc - y * fh - x * fw;
	glm::vec3 fbr = fc - y * fh + x * fw;

	// Compute the six planes
	m_planes[TOP].set3Points(ntr, ntl, ftl);
	m_planes[BOTTOM].set3Points(nbl, nbr, fbr);
	m_planes[LEFT].set3Points(ntl, nbl, fbl);
	m_planes[RIGHT].set3Points(nbr, ntr, fbr);
	m_planes[NEARP].set3Points(ntl, ntr, nbr);
	m_planes[FARP].set3Points(ftr, ftl, fbl);
}


bool CFrustum::isInside(glm::vec3 &p) const
{
	for (int i = 0; i < 6; i++) 
	{
		if (m_planes[i].distance(p) < 0.f)
		{
			return false;
		}
	}
	return true;
}

bool CFrustum::isInsideOrIntersects(const CBoundingSphere& sphere) const
{
	for (unsigned int i = 0; i < 6; i++) 
	{
		float distance = m_planes[i].distance(sphere.getPosition());
		if (distance < -sphere.getRadius())
		{
			// Outside
			return false;
		}
		else if (distance < sphere.getRadius())
		{
			// Intersects
			return true;
		}
	}
	// Inside
	return true;
}