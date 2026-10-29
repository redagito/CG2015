#pragma once
#include <cstddef>
// Measures non-overlapping one-second windows of actual completed gameplay frames.
class CFrameRateGuard
{
  public:
    explicit CFrameRateGuard(double minimumFps = 30);
    bool record(double seconds, bool activeGameplay);
    void reset();
    double lastFps() const;

  private:
    double m_minimum, m_elapsed = 0, m_lastFps = 0;
    std::size_t m_frames = 0;
    bool m_failed = false;
};
