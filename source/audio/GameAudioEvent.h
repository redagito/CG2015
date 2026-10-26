#pragma once
#include <glm/glm.hpp>

enum class GameAudioEventType
{
    Shot,
    Hit,
    Destroyed
};

struct GameAudioEvent
{
    GameAudioEventType type;
    glm::vec3 position;
};
