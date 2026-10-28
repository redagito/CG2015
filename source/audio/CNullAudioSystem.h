#pragma once
#include "IAudioSystem.h"

class CNullAudioSystem final : public IAudioSystem
{
    IResourceManager *manager = nullptr;

  public:
    ~CNullAudioSystem() override;
    void onAttach(IResourceManager *m) override;
    void onDetach(IResourceManager *m) override;
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
    bool render(float *p, std::uint64_t n) override;
};
