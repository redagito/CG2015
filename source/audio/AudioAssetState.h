#pragma once
#include <atomic>
#include <cstdint>
#include <miniaudio.h>
#include <thread>
#include <vector>

// Decoder and producer state. Only the ring's atomic indices cross into playback.
struct AudioAssetState
{

    ma_decoder decoder{};
    bool initialized = false, stream = false;
    unsigned channels = 0;
    std::uint64_t length = 0;
    std::vector<unsigned char> encoded;
    std::vector<float> pcm;
    std::vector<float> ring;
    std::vector<float> page;
    static constexpr std::uint64_t capacity = 96000;
    std::atomic<std::uint64_t> read{0}, written{0}, underruns{0};
    std::jthread worker;
    ~AudioAssetState();
    void fill();
};
