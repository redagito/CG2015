#pragma once

#include <glm/glm.hpp>

#include "CPlane.h"
#include "CBoundingSphere.h"

class CFrustum 
{
public:

	CFrustum();
	~CFrustum();

	/**
	* \brief Calculates frustum planes from camera parameters.
	*/
	void setFromCameraParameters(float viewAngle, float screenRatio, float nearZ, float farZ, glm::vec3 &position, glm::vec3 &look, glm::vec3 &up);

	/**
	* \brief Calculates frustum planes from inverse view projection matrix.
	*/
	void setFromInverseViewProjection(const glm::mat4& invViewProj);

	/**
	* \brief Checks if a point is in the Frustum.
	*/
	bool isInside(glm::vec3& point) const;

	/**
	* \brief Checks if the sphere is outside of the Frustum.
	* \brief Returns true if the sphere either is completely inside or intersects the frustum and false otherwise.
	*/
	bool isInsideOrIntersects(const CBoundingSphere& sphere) const;

private:

	enum PlanePosition
	{
		TOP = 0,
		BOTTOM,
		LEFT,
		RIGHT,
		NEARP,
		FARP
	};

	CPlane m_planes[6];
};