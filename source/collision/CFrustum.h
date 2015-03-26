#pragma once

#include <glm/glm.hpp>

#include "CPlane.h"

class CPlane;

#include "CAABbox.h"

class CAABBox;

class CFrustum {

private:

	enum {
		TOP = 0,
		BOTTOM,
		LEFT,
		RIGHT,
		NEARP,
		FARP
	};

public:

	static enum { OUTSIDE, INTERSECT, INSIDE };

	CPlane pl[6];

	glm::vec3 ntl, ntr, nbl, nbr, ftl, ftr, fbl, fbr;
	
	float nearD, farD, ratio, angle, tang;
	float nw, nh, fw, fh;

	CFrustum::CFrustum();
	CFrustum::~CFrustum();

	/* Each time the perspective definitions change, for instance when a window is resized, 
	*  this function should be called as well.
	*/
	void CFrustum::setCamInternals(float angle, float ratio, float nearD, float farD);
	
	/* Each time the camera position or orientation changes, this function should be 
	*  called as well.
	*/
	void CFrustum::setCamDef(glm::vec3 &p, glm::vec3 &l, glm::vec3 &u);

	/* Check if a point is in the Frustum */
	int CFrustum::pointInFrustum(glm::vec3 &p);

	/* Check if the sphere is in the Frustum */
	int CFrustum::sphereInFrustum(glm::vec3 &p, float raio);

	/* Check if the box is in the Frustum*/
	int CFrustum::boxInFrustum(CAABBox &b);

};