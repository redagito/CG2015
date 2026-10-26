#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>

struct UiText
{
    std::string text;
    glm::vec2 anchor{0.5f};
    float size = 24.f;
    glm::vec4 color{1.f};
    float tracking = 0.f;
    float maxWidth = 0.9f; // Fraction of viewport width; shrink text if necessary.
};
