#pragma once

#include "game/IController.h"
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
    CHealthController(float health, GameAudio *audio = nullptr, CCombatEffects *effects = nullptr);

    /**
     * \brief Destructor for cleanup.
     */
    ~CHealthController();
    float health() const;

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
    CCombatEffects *m_effects = nullptr;
    GameAudio *m_audio = nullptr;
    CGameObject *m_object = nullptr; /**< Controlled game object. */
    float m_health;                  /**< Health value. */
    bool m_active;                   /**< Cative flag. */
};
