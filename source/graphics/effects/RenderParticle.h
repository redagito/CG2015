#pragma once
#include "ParticleKind.h"
#include <glm/glm.hpp>
struct RenderParticle
{
    // Beam: position is the start, velocity is the full endpoint delta, halfSize.x is width.
    glm::vec3 position{0}, velocity{0};
    glm::vec4 color{1};
    glm::vec2 halfSize{1};
    float rotation = 0;
    ParticleKind kind = ParticleKind::Glow;
};
