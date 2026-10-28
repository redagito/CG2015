#include <cassert>

#include "CGameSystem.h"
#include "audio/GameAudio.h"
#include "game/progression/CPlayerProgression.h"
#include "resource/IResourceManager.h"

#include <spdlog/spdlog.h>

CGameSystem::~CGameSystem()
{
    SPDLOG_DEBUG("Game system destroyed.");
}

CGameSystem::CGameSystem()
    : m_progression(std::make_shared<CPlayerProgression>()), m_activeState(nullptr)
{
    SPDLOG_DEBUG("Game system created.");
}

// Updates game objects
bool CGameSystem::update(float dtime)
{
    if (m_resources)
        m_resources->pumpAsyncLoads(2.0);
    // Exit if state not set
    if (m_activeState == nullptr)
    {
        SPDLOG_ERROR("No active state is set.");
        return false;
    }

    // Update current state
    const bool keepState = m_activeState->update(dtime);
    if (m_audio)
        m_audio->update(dtime);
    if (!keepState)
    {
        // State ended
        m_activeState->onExit();
        // Check state exists
        if (m_gameStates.count(m_activeState->getNextState()) == 0)
        {
            // State transition error
            return false;
        }
        // Set next state
        const auto next = m_activeState->getNextState();
        auto *destination = m_gameStates.at(next).get();
        if (!m_initialized.contains(next) || destination->reinitializeOnEntry())
        {
            if (!destination->init(m_graphics, m_input, m_resources))
                return false;
            m_initialized.insert(next);
        }
        m_activeState = destination;
        m_activeName = next;
        if (m_audio)
        {
            m_audio->configure(*m_resources);
            m_audio->enterState(next);
        }
        m_activeState->onEnter();
    }
    // Update success
    return true;
}

bool CGameSystem::init(const std::string &initialState, IGraphicsSystem *graphicsSystem,
                       IInputProvider *inputProvider, IResourceManager *resourceManager,
                       GameAudio *audio)
{
    m_audio = audio;
    SPDLOG_DEBUG("Initializing game system.");
    SPDLOG_DEBUG("Game system initial state set to {}.", initialState.c_str());

    m_graphics = graphicsSystem;
    m_input = inputProvider;
    m_resources = resourceManager;
    if (!m_gameStates.contains(initialState))
        return false;
    // Other states assemble their scenes from prepared resources on first entry.
    if (!m_gameStates.at(initialState)->init(graphicsSystem, inputProvider, resourceManager))
        return false;
    m_initialized.insert(initialState);

    // Set initial state active
    assert(m_gameStates.count(initialState) != 0);
    m_activeState = m_gameStates.at(initialState).get();
    m_activeName = initialState;

    // On enter state trigger
    m_activeState->onEnter();
    return true;
}

bool CGameSystem::addState(const std::string &name, IGameState *state)
{
    // Check for duplicates
    if (m_gameStates.count(name) != 0)
    {
        // State with name already exists
        SPDLOG_WARN("Overwriting game state {}.", name.c_str());
    }

    // Add state
    m_initialized.erase(name);
    if (m_activeName == name)
    {
        m_activeState = nullptr;
        m_activeName.clear();
    }
    state->setProgression(m_progression);
    m_gameStates[name] = std::unique_ptr<IGameState>(state);
    SPDLOG_DEBUG("Added game state {} to game system.", name.c_str());
    return true;
}

void CGameSystem::removeState(const std::string &name)
{
    if (m_gameStates.count(name) == 0)
    {
        SPDLOG_WARN("Could not remove state {}, the state does not exist.", name.c_str());
        return;
    }
    m_initialized.erase(name);
    if (m_activeName == name)
    {
        m_activeState = nullptr;
        m_activeName.clear();
    }
    m_gameStates.erase(name);
    SPDLOG_DEBUG("Removed game state {} from game system.", name.c_str());
}

const std::shared_ptr<CPlayerProgression> &CGameSystem::progression() const
{
    return m_progression;
}
