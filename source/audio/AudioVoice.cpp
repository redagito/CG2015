#include "AudioVoice.h"
#include "resource/IResourceManager.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>

AudioVoice::~AudioVoice()
{
    if (initialized)
        ma_sound_uninit(&sound);
    if (sourceInitialized)
        ma_data_source_uninit(&source.base);
}
