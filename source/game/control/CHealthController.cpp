#include "CHealthController.h"
#include "game/effects/CCombatEffects.h"

#include "audio/GameAudio.h"
#include "collision/CCollidable.h"
#include "game/CGameObject.h"

CHealthController::CHealthController(float health, GameAudio *audio, CCombatEffects *effects)
    : m_health(health), m_active(true)
{
    m_audio = audio;
    m_effects = effects;
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
        if (m_effects)
            m_effects->damage(*m_object, damage, m_health <= 0,
                              m_object->getCollidable()->damageSource());
        if (m_audio)
            m_audio->emit({m_health <= 0 ? GameAudioEventType::Destroyed
                           : m_effects && m_effects->isPlayer(*m_object)
                               ? GameAudioEventType::PlayerHit
                               : GameAudioEventType::Hit,
                           m_object->getPosition()});
        if (m_health <= 0.f)
        {
            // Set object into death state
            m_object->setDead();
            if (m_effects)
                m_object->setVisible(false);
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

float CHealthController::health() const
{
    return m_health > 0 ? m_health : 0;
}
