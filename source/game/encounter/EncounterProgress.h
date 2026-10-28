#pragma once
#include <cstddef>
struct EncounterProgress
{
    std::size_t total = 0, defeated = 0;
    void recordDefeat();
    bool complete() const;
};
