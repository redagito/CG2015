#include "CMiniaudioSystem.h"
#include "CNullAudioSystem.h"
#include <spdlog/spdlog.h>

std::unique_ptr<IAudioSystem> createAudioSystem(AudioOutput output)
{
    if (output == AudioOutput::Silent)
        return std::make_unique<CNullAudioSystem>();
    try
    {
        return std::make_unique<CMiniaudioSystem>(output);
    }
    catch (const std::exception &error)
    {
        if (output == AudioOutput::Offline)
            throw;
        SPDLOG_WARN("Audio disabled: {}", error.what());
        return std::make_unique<CNullAudioSystem>();
    }
}
