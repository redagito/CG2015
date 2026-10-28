#pragma once
#include <glm/glm.hpp>
struct PlayerFeedback
{
    float damage = 0, shield = 0, hitConfirm = 0;
    glm::vec3 incomingDirection{0, 0, -1};
};
