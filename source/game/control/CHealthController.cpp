#include "CHealthController.h"

#include "audio/GameAudio.h"
#include "collision/CCollidable.h"
#include "game/CGameObject.h"

CHealthController::CHealthController(float health, GameAudio *audio)
    : m_health(health), m_active(true)
{
    m_audio = audio;
    return;
}

CHealthController::~CHealthController()
{
    return;
}

void CHealthController::attach(CGameObject *object)
{
    m_object = object;
}

void CHealthController::detach()
{
    m_object = nullptr;
}

void CHealthController::update(float dtime)
{
    if (m_active && m_object != nullptr && m_object->hasCollidable())
    {
        // Consume damage once. Dead objects must not emit repeated death cues.
        const float damage = m_object->getCollidable()->getDamageReceived();
        if (damage <= 0 || m_health <= 0)
            return;
        m_health -= damage;
        if (m_audio)
            m_audio->emit({m_health > 0 ? GameAudioEventType::Hit : GameAudioEventType::Destroyed,
                           m_object->getPosition()});
        if (m_health <= 0.f)
        {
            // Set object into death state
            m_object->setDead();
        }
    }
}

void CHealthController::setActive(bool state)
{
    m_active = state;
}

void CHealthController::receiveMessage(Message msg)
{
}
