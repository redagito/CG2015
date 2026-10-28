#pragma once
#include "game/GameObjectId.h"
#include <glm/glm.hpp>
#include <string>
struct TargetSnapshot
{
    GameObjectId id = noGameObject;
    std::string name;
    glm::vec3 position{0}, velocity{0};
};
