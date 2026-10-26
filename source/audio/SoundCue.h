#pragma once
#include "AudioTypes.h"
#include "resource/ResourceConfig.h"
#include <vector>

struct SoundCue
{
    std::vector<ResourceId> clips;
    AudioBus bus = AudioBus::Effects;
    float gain = 1, pitchVariation = 0, minDistance = 10, maxDistance = 900;
    bool loop = false, spatial = false;
    unsigned maxInstances = 4;
    int priority = 0;
};
