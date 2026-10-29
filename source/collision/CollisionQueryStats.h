#pragma once
#include <cstdint>
struct CollisionQueryStats
{
    std::uint64_t traces = 0, receivers = 0, indexBuilds = 0;
};
