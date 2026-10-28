#pragma once
#include <cstddef>
struct CombatStatus
{
    float health = 0, maxHealth = 1;
    std::size_t defeated = 0, total = 0;
    float mothershipApproachRemaining = -1, mothershipTotalRemaining = -1;
};
