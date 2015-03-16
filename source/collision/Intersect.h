#pragma once

#include "CBoundingSphere.h"
#include "CFrustum.h"

bool intersects(const CFrustum& frustum, const CBoundingSphere& sphere);