#pragma once
#include "SFittedShadow.h"
class ICamera;
SFittedShadow fitDirectionalShadow(const ICamera &, glm::vec3, float, unsigned);
