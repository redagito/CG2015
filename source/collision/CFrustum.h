#pragma once

#include <glm/glm.hpp>

#include "CPlane.h"
#include "CAABBox.h"

class CFrustum {

private:

	enum PlanePosition {
		TOP = 0,
		BOTTOM,
		LEFT,
		RIGHT,
		NEARP,
		FARP
	};

public:

	enum IntersectionResult { OUTSIDE, INTERSECT, INSIDE };

	CPlane pl[6];

	glm::vec3 ntl, ntr, nbl, nbr, ftl, ftr, fbl, fbr;
	
	float nearD, farD, ratio, angle, tang;
	float nw, nh, fw, fh;

	CFrustum();
	~CFrustum();

	/* Each time the perspective definitions change, for instance when a window is resized, 
	*  this function should be called as well.
	*/
	void setCamInternals(float angle, float ratio, float nearD, float farD);
	
	/* Each time the camera position or orientation changes, this function should be 
	*  called as well.
	*/
	void setCamDef(glm::vec3 &p, glm::vec3 &l, glm::vec3 &u);

	/* Check if a point is in the Frustum */
	int pointInFrustum(glm::vec3 &p);

	/* Check if the sphere is in the Frustum */
	int sphereInFrustum(glm::vec3 &p, float raio);

	/* Check if the box is in the Frustum*/
	int oxInFrustum(CAABBox &b);

};