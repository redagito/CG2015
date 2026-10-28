#pragma once
#include "audio/AudioTypes.h"
#include "game/IController.h"
#include "game/combat/EngineSoundTuning.h"
#include <memory>
class GameAudio;
class CFlightController;
class CFlightAudioController : public IController
{
  public:
    CFlightAudioController(GameAudio *audio, std::shared_ptr<const CFlightController> flight,
                           const EngineSoundTuning &tuning);
    ~CFlightAudioController();
    void attach(CGameObject *object) override;
    void detach() override;
    void update(float seconds) override;
    void setActive(bool active) override;
    void receiveMessage(Message message) override;

  private:
    GameAudio *m_audio;
    std::shared_ptr<const CFlightController> m_flight;
    EngineSoundTuning m_tuning;
    CGameObject *m_object = nullptr;
    VoiceHandle m_voice = 0;
    float m_gain, m_pitch;
    bool m_active = true;
};
