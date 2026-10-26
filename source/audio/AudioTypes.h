#pragma once
#include <cstdint>

using VoiceHandle = std::uint64_t;
enum class AudioBus
{
    Music,
    Effects,
    Engines,
    Ui
};
enum class AudioOutput
{
    Device,
    Silent,
    Offline
};
