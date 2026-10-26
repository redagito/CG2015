#pragma once
#include "AudioDataSource.h"
#include "AudioTypes.h"
#include <string>

struct AudioVoice
{
    AudioDataSource source;
    ma_sound sound{};
    VoiceHandle id = 0;
    std::string cue;
    int priority = 0;
    bool initialized = false;
    bool sourceInitialized = false;
    float stopRemaining = -1, baseGain = 1;
    ~AudioVoice();
};
