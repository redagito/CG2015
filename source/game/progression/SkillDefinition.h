#pragma once
#include "SkillBranch.h"
#include "SkillId.h"
#include <string_view>
struct SkillDefinition
{
    SkillId id;
    std::string_view name, description;
    SkillId prerequisite;
    SkillBranch branch;
    unsigned slot;
    unsigned maxRank = 5;
};
