#pragma once

#include "CBoundingSphere.h"
#include "CFrustum.h"

bool collides(const CFrustum& frustum, const CBoundingSphere& sphere);