#pragma once
#include <glm/glm.hpp>
struct MothershipTuning
{
    float approachSeconds = 60;
    float waitSeconds = 5;
    float scale = 400;
    glm::vec3 start{0, 750, 1400};
    glm::vec3 destination{0, 750, 0};
};
