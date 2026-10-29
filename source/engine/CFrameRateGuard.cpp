#include "CFrameRateGuard.h"
#include <cmath>
#include <stdexcept>
CFrameRateGuard::CFrameRateGuard(double minimumFps) : m_minimum(minimumFps)
{
    if (!std::isfinite(minimumFps) || minimumFps <= 0)
        throw std::invalid_argument("Invalid minimum FPS");
}
void CFrameRateGuard::reset()
{
    m_elapsed = 0;
    m_frames = 0;
    m_failed = false;
    m_lastFps = 0;
}
bool CFrameRateGuard::record(double seconds, bool activeGameplay)
{
    if (!activeGameplay)
    {
        reset();
        return true;
    }
    if (!std::isfinite(seconds) || seconds <= 0)
        throw std::invalid_argument("Invalid frame duration");
    if (m_failed)
        return false;
    m_elapsed += seconds;
    ++m_frames;
    if (m_elapsed >= 1)
    {
        m_lastFps = m_frames / m_elapsed;
        m_failed = m_lastFps + 1e-9 < m_minimum;
        m_elapsed = 0;
        m_frames = 0;
    }
    return !m_failed;
}
double CFrameRateGuard::lastFps() const
{
    return m_lastFps;
}
