#include "IGameState.h"

IGameState::~IGameState()
{
    return;
}
void IGameState::setProgression(std::shared_ptr<CPlayerProgression> progression)
{
    m_progression = std::move(progression);
}
bool IGameState::reinitializeOnEntry() const
{
    return false;
}
