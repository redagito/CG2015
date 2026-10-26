#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "IGameState.h"
#include "graphics/IGraphicsSystem.h"
#include "input/IInputProvider.h"
class GameAudio;

/**
 * \brief Manages game objects
 *
 * High level system for managing and updating the game objects and logic
 */
class CGameSystem
{
  public:
    /**
     * \brief Default constructor.
     */
    CGameSystem();

    /**
     * \brief Cleans up all game objects.
     */
    ~CGameSystem();

    /**
     * \brief Updates the game objects
     */
    bool update(float dtime);
    const std::string &activeStateName() const
    {
        return m_activeName;
    }

    /**
     * \brief Initialize game system with startup config file.
     */
    bool init(const std::string &initialState, IGraphicsSystem *graphicsSystem,
              IInputProvider *inputProvider, IResourceManager *resourceManager,
              GameAudio *audio = nullptr);

    /**
     * \brief Add game state by name and initializes the state.
     * \return True if the state could be initialized and added. False otherwise.
     * State must be heap allocated, ownership is transferred to the game system.
     */
    bool addState(const std::string &name, IGameState *state);

    /**
     * \brief Removes state from list and deletes state object.
     */
    void removeState(const std::string &name);

  private:
    GameAudio *m_audio = nullptr;
    IGraphicsSystem *m_graphics = nullptr;
    IInputProvider *m_input = nullptr;
    IResourceManager *m_resources = nullptr;
    std::unordered_set<std::string> m_initialized;
    std::string m_activeName;
    std::unordered_map<std::string, std::unique_ptr<IGameState>>
        m_gameStates;          /**< Game states stored by name. */
    IGameState *m_activeState; /**< Current game state. */
};
