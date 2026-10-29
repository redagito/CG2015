#include "CHealthController.h"
#include "game/effects/CCombatEffects.h"

#include "audio/GameAudio.h"
#include "collision/CCollidable.h"
#include "game/CGameObject.h"
#include <algorithm>
#include <cmath>

CHealthController::CHealthController(float health, GameAudio *audio, CCombatEffects *effects,
                                     const DefenseTuning &defenses)
    : m_defenses(defenses), m_maxHealth(health), m_shield(defenses.shieldCapacity),
      m_health(health), m_active(true)
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
    syncDamageBudget();
}

void CHealthController::detach()
{
    m_object = nullptr;
}

void CHealthController::update(float seconds)
{
    if (!m_active || !m_object || !m_object->hasCollidable() || !std::isfinite(seconds) ||
        seconds < 0)
        return;
    const float dt = std::min(seconds, .25f);
    const float incoming = m_object->getCollidable()->getDamageReceived();
    if (m_health <= 0)
        return;
    m_sinceDamage += dt;
    if (incoming > 0)
    {
        m_sinceDamage = 0;
        const float reduced = incoming * (1 - std::clamp(m_defenses.armor, 0.f, .8f));
        const float absorbed = std::min(m_shield, reduced);
        m_shield -= absorbed;
        m_health -= reduced - absorbed;
        if (m_effects)
            m_effects->damage(*m_object, reduced, m_health <= 0,
                              m_object->getCollidable()->damageSource());
        if (m_audio)
            m_audio->emit({m_health <= 0 ? GameAudioEventType::Destroyed
                           : m_effects && m_effects->isPlayer(*m_object)
                               ? GameAudioEventType::PlayerHit
                               : GameAudioEventType::Hit,
                           m_object->getPosition()});
        if (m_health <= 0)
        {
            m_object->setDead();
            if (m_effects)
                m_object->setVisible(false);
        }
    }
    else
    {
        // Count only the part of this frame beyond each recovery delay.
        const float hullTime = std::clamp(m_sinceDamage - m_defenses.hullRepairDelay, 0.f, dt);
        const float shieldTime =
            std::clamp(m_sinceDamage - m_defenses.shieldRechargeDelay, 0.f, dt);
        heal(m_defenses.hullRegen * hullTime);
        m_shield =
            std::min(m_defenses.shieldCapacity, m_shield + m_defenses.shieldRegen * shieldTime);
    }
    syncDamageBudget();
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

float CHealthController::maxHealth() const
{
    return m_maxHealth;
}
float CHealthController::shield() const
{
    return m_shield;
}
float CHealthController::maxShield() const
{
    return m_defenses.shieldCapacity;
}
void CHealthController::heal(float amount)
{
    if (m_active && m_health > 0 && std::isfinite(amount) && amount > 0)
        m_health = std::min(m_maxHealth, m_health + amount);
    syncDamageBudget();
}
void CHealthController::syncDamageBudget()
{
    if (m_object && m_object->hasCollidable())
        m_object->getCollidable()->setHealthBudget(health(), m_shield, m_defenses.armor);
}
