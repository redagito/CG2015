#include "CWaveDirector.h"
#include <algorithm>
#include <stdexcept>
CWaveDirector::CWaveDirector(const EnemyRoster &roster)
{
    for (const auto &spawn : roster.spawns)
    {
        if (spawn.wave >= m_counts.size())
            m_counts.resize(spawn.wave + 1);
        ++m_counts[spawn.wave];
    }
    if (m_counts.empty() || std::find(m_counts.begin(), m_counts.end(), 0) != m_counts.end())
        throw std::runtime_error("Encounter waves must be nonempty and contiguous");
}
void CWaveDirector::recordDefeat()
{
    if (m_defeated < m_counts[m_current])
        ++m_defeated;
}
bool CWaveDirector::advance()
{
    if (remaining() != 0 || finalWave())
        return false;
    ++m_current;
    m_defeated = 0;
    return true;
}
std::size_t CWaveDirector::wave() const
{
    return m_current;
}
std::size_t CWaveDirector::waveCount() const
{
    return m_counts.size();
}
std::size_t CWaveDirector::remaining() const
{
    return m_counts[m_current] - m_defeated;
}
bool CWaveDirector::finalWave() const
{
    return m_current + 1 == m_counts.size();
}
bool CWaveDirector::complete() const
{
    return finalWave() && remaining() == 0;
}
