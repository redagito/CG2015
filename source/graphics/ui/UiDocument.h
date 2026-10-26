#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>

// Anchors are normalized viewport coordinates, measured from the top left.
// Sizes use a 1280x720 design canvas and scale uniformly to fit the viewport.
struct UiText {
    std::string text;
    glm::vec2 anchor{0.5f};
    float size=24.f;
    glm::vec4 color{1.f};
    float tracking=0.f;
    float maxWidth=0.9f; // Fraction of viewport width; shrink text if necessary.
};
struct UiRect {
    glm::vec2 anchor{0.5f};
    glm::vec2 size{1.f};
    glm::vec4 color{1.f};
    glm::vec2 offset{0.f}; // Design-space offset, scaled with size.
};
struct UiDocument {
    std::vector<UiRect> rectangles;
    std::vector<UiText> texts;
    bool empty() const { return rectangles.empty() && texts.empty(); }
};
