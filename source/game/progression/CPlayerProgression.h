#pragma once
#include "SkillDefinition.h"
#include <array>
#include <cstdint>
#include <span>
struct FlightTuning;
struct WeaponTuning;
// Owned by one game session, never saved to disk or shared globally.
class CPlayerProgression
{
  public:
    void awardKill();
    std::uint64_t points() const;
    std::uint64_t earned() const;
    unsigned rank(SkillId skill) const;
    unsigned cost(SkillId skill) const;
    bool unlocked(SkillId skill) const;
    bool canBuy(SkillId skill) const;
    bool buy(SkillId skill);
    void apply(FlightTuning &flight, WeaponTuning &weapon) const;
    static std::span<const SkillDefinition> skills();

  private:
    std::uint64_t m_points = 0, m_earned = 0;
    std::array<unsigned, static_cast<std::size_t>(SkillId::Count)> m_ranks{};
};
