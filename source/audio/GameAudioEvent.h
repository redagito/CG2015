#pragma once
#include <glm/glm.hpp>

enum class GameAudioEventType
{
    Shot,
    Hit,
    PlayerHit,
    Destroyed,
    MothershipCharge,
    MothershipBeam
};

struct GameAudioEvent
{
    GameAudioEventType type;
    glm::vec3 position;
};
