#pragma once
#include "GameAudioEvent.h"
#include "IAudioSystem.h"

class IResourceManager;

// Translates game events and state changes into sound cues. No backend types or
// file names are exposed to weapon, health or emitter controllers.
class GameAudio
{
  public:
    explicit GameAudio(IAudioSystem &system);
    ~GameAudio();
    void configure(IResourceManager &resources);
    void enterState(const std::string &state);
    void emit(GameAudioEvent event);
    void update(float seconds);
    VoiceHandle startEmitter(const std::string &cue, glm::vec3 position);
    bool emitterActive(VoiceHandle handle) const;
    void moveEmitter(VoiceHandle handle, glm::vec3 position, float gain = 1);
    void stopEmitter(VoiceHandle handle);
    void stopEmitters();
    void setListener(glm::vec3 position, glm::vec3 forward, glm::vec3 up);

  private:
    IAudioSystem &m_system;
    bool m_configured = false;
    VoiceHandle m_music = 0;
    std::string m_musicCue;
    std::vector<VoiceHandle> m_emitters;
    std::vector<GameAudioEvent> m_events;
};
