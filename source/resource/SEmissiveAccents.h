#pragma once
#include <glm/glm.hpp>
namespace Json
{
class Value;
}

// Optional hull markings in the mesh's own coordinates. Zero strength disables them.
struct SEmissiveAccents
{
    glm::vec3 color{1.f, .02f, .006f};
    float strength = 0;
    float hullStripOffset = 2, hullStripWidth = .15f;
    glm::vec2 hullZRange{-3, 3};
    float engineZMax = -2, engineHalfWidth = 2;
    float engineCenterY = 0, engineHalfHeight = 1;
    bool load(const Json::Value &root);
    bool valid() const;
};
