#pragma once
#include "audio/AudioTypes.h"
#include "game/IController.h"
#include <glm/glm.hpp>
#include <string>

class GameAudio;

// Keeps a looping voice attached to an object's transform and lifetime.
class CAudioEmitterController : public IController
{
  public:
    CAudioEmitterController(GameAudio *audio, std::string cue);
    ~CAudioEmitterController();
    void attach(CGameObject *object) override;
    void detach() override;
    void update(float seconds) override;
    void setActive(bool active) override;
    void receiveMessage(Message message) override;

  private:
    GameAudio *m_audio = nullptr;
    CGameObject *m_object = nullptr;
    std::string m_cue;
    VoiceHandle m_voice = 0;
    bool m_active = true;
};
