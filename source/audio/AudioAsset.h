#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

// Prepared on resource workers. Streaming clips retain compressed bytes and a
// bounded two-second PCM ring; the audio callback never decodes or opens files.
struct AudioAssetState;

class AudioAsset
{
  public:
    static std::shared_ptr<AudioAsset> load(const std::string &, bool stream, unsigned channels);
    ~AudioAsset();
    AudioAsset(const AudioAsset &) = delete;
    unsigned channels() const;
    unsigned sampleRate() const;
    std::uint64_t frames() const;
    bool streaming() const;
    const std::vector<float> &samples() const;
    std::uint64_t readStream(float *output, std::uint64_t count) const;
    std::uint64_t bufferedFrames() const;
    std::uint64_t underruns() const;

  private:
    AudioAsset();
    std::unique_ptr<AudioAssetState> m_impl;
};
