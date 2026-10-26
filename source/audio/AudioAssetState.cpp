#include "AudioAssetState.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <stdexcept>

AudioAssetState::~AudioAssetState()
{
    worker.request_stop();
    if (worker.joinable())
        worker.join();
    if (initialized)
        ma_decoder_uninit(&decoder);
}

void AudioAssetState::fill()
{
    if (page.empty())
        page.resize(4096 * channels);
    // Single producer: publish only complete frames after copying their samples.
    // The audio callback advances read after consuming them, so unread data is
    // never overwritten. No allocation or decoder work happens on the callback.
    bool soughtWithoutProgress = false;
    while (true)
    {
        const auto w = written.load(std::memory_order_relaxed),
                   r = read.load(std::memory_order_acquire);
        const auto count = std::min<std::uint64_t>(4096, capacity - (w - r));
        if (!count)
            break;
        ma_uint64 got = 0;
        const auto result = ma_decoder_read_pcm_frames(&decoder, page.data(), count, &got);
        if (result != MA_SUCCESS && result != MA_AT_END)
            throw std::runtime_error("Audio stream decode failed");
        for (std::uint64_t frame = 0; frame < got; ++frame)
            std::memcpy(&ring[((w + frame) % capacity) * channels], &page[frame * channels],
                        channels * sizeof(float));
        written.store(w + got, std::memory_order_release);
        if (got < count)
        {
            if (!got && soughtWithoutProgress)
                throw std::runtime_error("Audio stream decoder made no progress");
            soughtWithoutProgress = got == 0;
            if (ma_decoder_seek_to_pcm_frame(&decoder, 0) != MA_SUCCESS)
                throw std::runtime_error("Audio stream loop seek failed");
            if (!got && length == 0)
                throw std::runtime_error("Empty audio stream");
        }
    }
}
