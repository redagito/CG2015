#include <math.h>

#include "CFrustum.h"

#include <glm/ext.hpp>

#include "math/CTransformer.h"

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
	m_planes[Top].set3Points(ntr, ntl, ftl);
	m_planes[Bottom].set3Points(nbl, nbr, fbr);
	m_planes[Left].set3Points(ntl, nbl, fbl);
	m_planes[Right].set3Points(nbr, ntr, fbr);
	m_planes[Near].set3Points(ntl, ntr, nbr);
	m_planes[Far].set3Points(ftr, ftl, fbl);
}

void CFrustum::setFromInverseViewProjection(const glm::mat4& invViewProj)
{
	// Setup frustum from invViewProj matrix (transform from projection space
	// into world space).

	// Point positions
	const unsigned int topLeftFar = 0;
	const unsigned int topRightFar = 1;
	const unsigned int bottomLeftFar = 2;
	const unsigned int bottomRightFar = 3;
	const unsigned int topLeftNear = 4;
	const unsigned int topRightNear = 5;
	const unsigned int bottomLeftNear = 6;
	const unsigned int bottomRightNear = 7;

	// Frustum corners in projection space
	glm::vec4 points[8];
	points[topLeftFar] = glm::vec4(-1.0f, 1.0f, 1.0f, 1.f);
	points[topRightFar] = glm::vec4(1.0f, 1.0f, 1.0f, 1.f);
	points[bottomLeftFar] = glm::vec4(-1.0f, -1.0f, 1.0f, 1.f);
	points[bottomRightFar] = glm::vec4(1.0f, -1.0f, 1.0f, 1.f);
	points[topLeftNear] = glm::vec4(-1.0f, 1.0f, -1.0f, 1.f);
	points[topRightNear] = glm::vec4(1.0f, 1.0f, -1.0f, 1.f);
	points[bottomLeftNear] = glm::vec4(-1.0f, -1.0f, -1.0f, 1.f);
	points[bottomRightNear] = glm::vec4(1.0f, -1.0f, -1.0, 1.f);

	// Calculate frustum corners in world space
	for (unsigned int i = 0; i < 8; i++)
	{
		// Transform to world space
		points[i] = points[i] * invViewProj;
		points[i] /= points[i].w;
	}

	// Set frustum planes
	m_planes[Near].set3Points(glm::vec3(points[topRightNear]), glm::vec3(points[topLeftNear]), glm::vec3(points[bottomLeftNear]));
	m_planes[Far].set3Points(glm::vec3(points[topLeftFar]), glm::vec3(points[topRightFar]), glm::vec3(points[bottomRightFar]));
	m_planes[Left].set3Points(glm::vec3(points[bottomLeftFar]), glm::vec3(points[bottomLeftNear]), glm::vec3(points[topLeftNear]));
	m_planes[Right].set3Points(glm::vec3(points[topRightFar]), glm::vec3(points[topRightNear]), glm::vec3(points[bottomRightNear]));
	m_planes[Top].set3Points(glm::vec3(points[topLeftNear]), glm::vec3(points[topRightNear]), glm::vec3(points[topRightFar]));
	m_planes[Bottom].set3Points(glm::vec3(points[bottomLeftFar]), glm::vec3(points[bottomRightFar]), glm::vec3(points[bottomRightNear]));
}

void CFrustum::setFromViewProjection(const glm::mat4& view, const glm::mat4& proj)
{
	// Handles transformation calculations
	CTransformer transformer;
	transformer.setViewMatrix(view);
	transformer.setProjectionMatrix(proj);
	setFromInverseViewProjection(transformer.getInverseViewProjectionMatrix());
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
		if (distance < -sphere.getRadius()) // Is this really correct??
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