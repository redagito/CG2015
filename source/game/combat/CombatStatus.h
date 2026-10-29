#pragma once
#include <cstddef>
#include <cstdint>
struct CombatStatus
{
    float health = 0, maxHealth = 1;
    std::size_t defeated = 0, total = 0;
    float mothershipApproachRemaining = -1, mothershipTotalRemaining = -1;
    std::size_t wave = 0, waveCount = 0, waveRemaining = 0;
    std::uint64_t points = 0;
    bool finalGraceActive = false;
    float shield = 0, maxShield = 0;
};
