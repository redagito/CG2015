#pragma once
#include "InterceptSolution.h"
#include "TargetSnapshot.h"
#include <optional>
struct TargetingSolution
{
    TargetSnapshot target;
    std::optional<InterceptSolution> intercept;
    float range = 0;
};
