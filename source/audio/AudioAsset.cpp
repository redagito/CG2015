#include "AudioAsset.h"
#include "AudioAssetState.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <fstream>
#include <miniaudio.h>
#include <stdexcept>
#include <thread>

AudioAsset::AudioAsset() : m_impl(std::make_unique<AudioAssetState>())
{
}
AudioAsset::~AudioAsset() = default;
std::shared_ptr<AudioAsset> AudioAsset::load(const std::string &file, bool stream,
                                             unsigned channels)
{
    if (channels != 1 && channels != 2)
        throw std::runtime_error("Audio requires one or two output channels");
    auto asset = std::shared_ptr<AudioAsset>(new AudioAsset);
    auto &data = *asset->m_impl;
    data.stream = stream;
    data.channels = channels;
    std::ifstream input(file, std::ios::binary | std::ios::ate);
    if (!input)
        throw std::runtime_error("Cannot read audio: " + file);
    const auto size = input.tellg();
    if (size <= 0 || size > 64 * 1024 * 1024)
        throw std::runtime_error("Invalid or oversized audio: " + file);
    data.encoded.resize(static_cast<size_t>(size));
    input.seekg(0);
    if (!input.read(reinterpret_cast<char *>(data.encoded.data()), size))
        throw std::runtime_error("Incomplete audio: " + file);
    auto config = ma_decoder_config_init(ma_format_f32, channels, 48000);
    if (ma_decoder_init_memory(data.encoded.data(), data.encoded.size(), &config, &data.decoder) !=
        MA_SUCCESS)
        throw std::runtime_error("Unsupported or corrupt audio: " + file);
    data.initialized = true;
    ma_uint64 length = 0;
    if (ma_decoder_get_length_in_pcm_frames(&data.decoder, &length) != MA_SUCCESS || !length)
        throw std::runtime_error("Audio has no frames: " + file);
    data.length = length;
    if (stream)
    {
        data.ring.resize(AudioAssetState::capacity * channels);
        data.fill();
        // The object address is stable; destruction stops/joins before freeing it.
        data.worker = std::jthread(
            [p = &data](std::stop_token stop)
            {
                try
                {
                    while (!stop.stop_requested())
                    {
                        p->fill();
                        std::this_thread::sleep_for(std::chrono::milliseconds(2));
                    }
                }
                catch (...)
                {
                    ++p->underruns;
                } // Callback gets silence if a later decode fails.
            });
    }
    else
    {
        if (length > 48000 * 30)
            throw std::runtime_error("Long audio must be streamed: " + file);
        data.pcm.resize(length * channels);
        ma_uint64 got = 0;
        if (ma_decoder_read_pcm_frames(&data.decoder, data.pcm.data(), length, &got) !=
                MA_SUCCESS ||
            got != length)
            throw std::runtime_error("Audio decode failed: " + file);
        ma_decoder_uninit(&data.decoder);
        data.initialized = false;
        data.encoded.clear();
    }
    return asset;
}
unsigned AudioAsset::channels() const
{
    return m_impl->channels;
}
std::uint64_t AudioAsset::frames() const
{
    return m_impl->length;
}
bool AudioAsset::streaming() const
{
    return m_impl->stream;
}
const std::vector<float> &AudioAsset::samples() const
{
    return m_impl->pcm;
}
std::uint64_t AudioAsset::bufferedFrames() const
{
    // Snapshot the consumer first so concurrent playback cannot underflow the count.
    const auto consumed = m_impl->read.load(std::memory_order_acquire);
    return m_impl->written.load(std::memory_order_acquire) - consumed;
}
std::uint64_t AudioAsset::underruns() const
{
    return m_impl->underruns.load();
}
std::uint64_t AudioAsset::readStream(float *output, std::uint64_t count) const
{
    auto &data = *m_impl;
    const auto r = data.read.load(std::memory_order_relaxed);
    const auto available = std::min(count, data.written.load(std::memory_order_acquire) - r);
    for (std::uint64_t frame = 0; frame < available; ++frame)
        if (output)
            std::memcpy(output + frame * data.channels,
                        &data.ring[((r + frame) % AudioAssetState::capacity) * data.channels],
                        data.channels * sizeof(float));
    data.read.store(r + available, std::memory_order_release);
    if (available < count)
    {
        ++data.underruns;
        if (output)
            std::fill(output + available * data.channels, output + count * data.channels, 0.f);
    }
    return count; // A looping stream stays alive even if its producer briefly falls behind.
}

unsigned AudioAsset::sampleRate() const
{
    return 48000;
}
