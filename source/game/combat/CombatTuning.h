#pragma once
#include "EngineSoundTuning.h"
#include "TargetingTuning.h"
#include "WeaponTuning.h"
class IResourceManager;
struct CombatTuning
{
    WeaponTuning weapon;
    TargetingTuning targeting;
    EngineSoundTuning engine;
    static CombatTuning load(IResourceManager &resources);
    void validate() const;
};
