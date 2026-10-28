#include "CFlightAudioController.h"
#include "CFlightController.h"
#include "audio/GameAudio.h"
#include "game/CGameObject.h"
#include "game/combat/FlightEngineSound.h"
#include <algorithm>
#include <cmath>
CFlightAudioController::CFlightAudioController(GameAudio *audio,
                                               std::shared_ptr<const CFlightController> flight,
                                               const EngineSoundTuning &tuning)
    : m_audio(audio), m_flight(std::move(flight)), m_tuning(tuning), m_gain(tuning.idleGain),
      m_pitch(tuning.idlePitch)
{
}
CFlightAudioController::~CFlightAudioController()
{
    detach();
}
void CFlightAudioController::attach(CGameObject *object)
{
    m_object = object;
}
void CFlightAudioController::detach()
{
    if (m_audio)
        m_audio->stopEmitter(m_voice);
    m_voice = 0;
    m_object = nullptr;
}
void CFlightAudioController::setActive(bool active)
{
    m_active = active;
    if (!active && m_audio)
        m_audio->stopEmitter(m_voice);
}
void CFlightAudioController::receiveMessage(Message)
{
}
void CFlightAudioController::update(float seconds)
{
    if (!m_active || !m_object || !m_audio || !std::isfinite(seconds) || seconds < 0)
        return;
    // The death burst continues after the ship is hidden; its engine must not.
    if (m_object->isDead())
    {
        if (m_voice)
            m_audio->stopEmitter(m_voice);
        m_voice = 0;
        return;
    }
    if (!m_audio->emitterActive(m_voice))
        m_voice = m_audio->startEmitter("player_engine", m_object->getPosition());
    const auto target = FlightEngineSound::levels(m_flight->state(), m_tuning);
    const float response = 1 - std::exp(-m_tuning.response * std::min(seconds, .25f));
    m_gain += (target.gain - m_gain) * response;
    m_pitch += (target.pitch - m_pitch) * response;
    m_audio->moveEmitter(m_voice, m_object->getPosition(), m_gain, m_pitch);
}
