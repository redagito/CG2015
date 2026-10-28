#pragma once
#include <cstddef>
class IResourceManager;
struct CombatEffectsTuning
{
    std::size_t maxParticles = 2048;
    float trailInterval = .025f, trailLifetime = .13f, shieldDuration = .45f, shieldRadius = 3.6f;
    float cameraKick = .45f, resultDelay = 1;
    static CombatEffectsTuning load(IResourceManager &resources);
};
