#pragma once
#include "SkillId.h"
#include <string_view>
struct SkillDefinition
{
    SkillId id;
    std::string_view name, description;
    SkillId prerequisite;
    unsigned maxRank = 5;
};
