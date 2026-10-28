#pragma once
#include "EnemyRoster.h"
#include <vector>
class CWaveDirector
{
  public:
    explicit CWaveDirector(const EnemyRoster &roster);
    void recordDefeat();
    bool advance();
    std::size_t wave() const;
    std::size_t waveCount() const;
    std::size_t remaining() const;
    bool finalWave() const;
    bool complete() const;

  private:
    std::vector<std::size_t> m_counts;
    std::size_t m_current = 0, m_defeated = 0;
};
