#include "CAudioEmitterController.h"
#include "audio/GameAudio.h"
#include "game/CGameObject.h"
#include <algorithm>
#include <cmath>

CAudioEmitterController::CAudioEmitterController(GameAudio *audio, std::string cue,
                                                 bool respondToMotion)
    : m_audio(audio), m_cue(std::move(cue)), m_respondToMotion(respondToMotion)
{
}

CAudioEmitterController::~CAudioEmitterController()
{
    detach();
}

void CAudioEmitterController::attach(CGameObject *object)
{
    m_object = object;
    m_previousPosition = object->getPosition();
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
    if (!m_audio->emitterActive(m_voice))
        m_voice = m_audio->startEmitter(m_cue, position);

    // Movement is kinematic, so use speed as a gentle engine-load cue, not RPM.
    const float speed = seconds > 0 ? glm::length(position - m_previousPosition) / seconds : 0.f;
    const float target = m_respondToMotion ? .3f + .7f * std::min(speed / 20.f, 1.f) : 1.f;
    m_gain += (target - m_gain) * (1.f - std::exp(-4.f * std::max(seconds, 0.f)));
    m_audio->moveEmitter(m_voice, position, m_gain);
    m_previousPosition = position;
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
