#pragma once
#include "EnemyDecoration.h"
#include "game/arena/EnemyTuning.h"
#include "game/combat/WeaponTuning.h"
#include <string>
struct EnemyDefinition
{
    std::string id, name, mesh, material, engineCue = "enemy_engine";
    glm::vec3 scale{.5f};
    float health = 100;
    EnemyTuning behavior;
    WeaponTuning weapon;
    EnemyDecoration decoration;
};
