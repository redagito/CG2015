#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>

struct UiRect
{
    glm::vec2 anchor{0.5f};
    glm::vec2 size{1.f};
    glm::vec4 color{1.f};
    glm::vec2 offset{0.f}; // Design-space offset, scaled with size.
};
