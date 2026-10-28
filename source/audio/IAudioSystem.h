#pragma once
#include "resource/IResourceListener.h"
#include <cstdint>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>

#include "AudioTypes.h"
#include "SoundCue.h"

class IAudioSystem : public IResourceListener
{
  public:
    virtual ~IAudioSystem();
    virtual void defineCue(const std::string &, const SoundCue &) = 0;
    virtual VoiceHandle play(const std::string &, glm::vec3 position = {}, float fadeIn = 0) = 0;
    virtual bool playing(VoiceHandle) const = 0;
    virtual void stop(VoiceHandle, float fadeSeconds = 0) = 0;
    virtual void stopAll() = 0;
    virtual void setPosition(VoiceHandle, glm::vec3) = 0;
    virtual void setGain(VoiceHandle, float) = 0;
    virtual void setPitch(VoiceHandle, float) = 0;
    virtual void setListener(glm::vec3 position, glm::vec3 forward, glm::vec3 up) = 0;
    virtual void setMasterGain(float) = 0;
    virtual void setBusGain(AudioBus, float) = 0;
    virtual void update(float seconds) = 0;
    virtual unsigned activeVoices() const = 0;
    virtual bool audibleDevice() const = 0;
    // Deterministic offline mixing for tests/tools. Device mode owns its callback.
    virtual bool render(float *stereo, std::uint64_t frames) = 0;
};
std::unique_ptr<IAudioSystem> createAudioSystem(AudioOutput output = AudioOutput::Device);
