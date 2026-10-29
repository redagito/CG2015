#pragma once

#include "game/IController.h"
#include "game/combat/DefenseTuning.h"
class GameAudio;
class CCombatEffects;

/**
 * \brief Stores health state and marks object for deletion on death.
 * TODO This should actually send an onDeath event to the object.
 */
class CHealthController : public IController
{
  public:
    /**
     * \brief Sets health.
     */
    CHealthController(float health, GameAudio *audio = nullptr, CCombatEffects *effects = nullptr,
                      const DefenseTuning &defenses = {});

    /**
     * \brief Destructor for cleanup.
     */
    ~CHealthController();
    float health() const;
    float maxHealth() const;
    float shield() const;
    float maxShield() const;
    void heal(float amount);

    /**
     * \brief On attach to object.
     */
    void attach(CGameObject *object);

    /**
     * \brief On dettaching from game object.
     */
    void detach();

    /**
     * \brief Updates controller.
     */
    void update(float dtime);

    /**
     * \brief Active state.
     */
    void setActive(bool state);

    void receiveMessage(Message msg);

  private:
    void syncDamageBudget();
    CCombatEffects *m_effects = nullptr;
    GameAudio *m_audio = nullptr;
    CGameObject *m_object = nullptr; /**< Controlled game object. */
    DefenseTuning m_defenses;
    float m_maxHealth = 0, m_shield = 0, m_sinceDamage = 0;
    float m_health; /**< Health value. */
    bool m_active;  /**< Cative flag. */
};
