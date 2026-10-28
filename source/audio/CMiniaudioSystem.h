#pragma once
#include "AudioVoice.h"
#include "IAudioSystem.h"
#include <array>
#include <random>
#include <unordered_map>

// Owns the mixer/device and live voices. Control methods run on the game thread;
// the device callback only reads prepared PCM and mixes the node graph.
class CMiniaudioSystem final : public IAudioSystem
{
  public:
    explicit CMiniaudioSystem(AudioOutput output);
    ~CMiniaudioSystem() override;
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

  private:
    static void limit(float *, std::uint64_t);
    static void callback(ma_device *, void *, const void *, ma_uint32);
    AudioVoice *find(VoiceHandle) const;
    void cleanup();
    ma_engine engine{};
    ma_device device{};
    bool initialized = false;
    bool deviceReady = false;
    AudioOutput output;
    std::array<ma_sound_group, 4> buses{};
    unsigned initializedBuses = 0;
    std::unordered_map<ResourceId, std::shared_ptr<const AudioAsset>> clips;
    std::unordered_map<std::string, SoundCue> cues;
    std::vector<std::unique_ptr<AudioVoice>> voices;
    IResourceManager *manager = nullptr;
    VoiceHandle next = 1;
    std::mt19937 random{0xC72015};
};
