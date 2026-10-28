#pragma once
#include "audio/IAudioSystem.h"
class RecordingAudioSystem : public IAudioSystem
{
  public:
    std::string lastCue;
    bool alive = false;
    float gain = 0, pitch = 1;
    glm::vec3 position{0};
    unsigned starts = 0, stops = 0;
    void onAttach(IResourceManager *) override;
    void onDetach(IResourceManager *) override;
    void notify(EResourceType, ResourceId, EListenerEvent, IResourceManager *) override;
    void defineCue(const std::string &, const SoundCue &) override;
    VoiceHandle play(const std::string &, glm::vec3, float) override;
    bool playing(VoiceHandle) const override;
    void stop(VoiceHandle, float) override;
    void stopAll() override;
    void setPosition(VoiceHandle, glm::vec3) override;
    void setGain(VoiceHandle, float) override;
    void setPitch(VoiceHandle, float) override;
    void setListener(glm::vec3, glm::vec3, glm::vec3) override;
    void setMasterGain(float) override;
    void setBusGain(AudioBus, float) override;
    void update(float) override;
    unsigned activeVoices() const override;
    bool audibleDevice() const override;
    bool render(float *, std::uint64_t) override;
};
