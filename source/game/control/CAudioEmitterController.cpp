#include "CAudioEmitterController.h"
#include "audio/GameAudio.h"
#include "game/CGameObject.h"

CAudioEmitterController::CAudioEmitterController(GameAudio *audio, std::string cue)
    : m_audio(audio), m_cue(std::move(cue))
{
}

CAudioEmitterController::~CAudioEmitterController()
{
    detach();
}

void CAudioEmitterController::attach(CGameObject *object)
{
    m_object = object;
}

void CAudioEmitterController::detach()
{
    if (m_audio)
        m_audio->stopEmitter(m_voice);
    m_voice = 0;
    m_object = nullptr;
}

void CAudioEmitterController::update(float seconds)
{
    if (!m_audio || !m_object || !m_active)
        return;
    const auto position = m_object->getPosition();
    if (!m_voice || !m_audio->emitterActive(m_voice))
    {
        m_voiceRetry -= seconds;
        if (m_voiceRetry > 0)
            return;
        m_voice = m_audio->startEmitter(m_cue, position);
        if (!m_voice)
        {
            m_voiceRetry = .25f;
            return;
        }
    }

    m_audio->moveEmitter(m_voice, position);
}

void CAudioEmitterController::setActive(bool active)
{
    m_active = active;
    if (!active && m_audio)
        m_audio->stopEmitter(m_voice);
}

void CAudioEmitterController::receiveMessage(Message)
{
}
