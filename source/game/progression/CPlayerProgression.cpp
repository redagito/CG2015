#include "CPlayerProgression.h"
#include "game/combat/WeaponTuning.h"
#include "game/flight/FlightTuning.h"
#include <limits>
namespace
{
constexpr SkillDefinition definitions[] = {
    {SkillId::Damage, "FIREPOWER", "+20% damage per rank", SkillId::Count},
    {SkillId::FireRate, "RAPID FIRE", "+15% firing rate per rank", SkillId::Damage},
    {SkillId::Handling, "AGILITY", "+10% turning per rank", SkillId::Count},
    {SkillId::Acceleration, "RESPONSIVENESS", "+15% acceleration per rank", SkillId::Handling},
    {SkillId::Speed, "ENGINE POWER", "+10% flight speed per rank", SkillId::Count},
    {SkillId::Boost, "AFTERBURNER", "+12% boost speed per rank", SkillId::Speed}};
}
void CPlayerProgression::awardKill()
{
    if (m_earned < std::numeric_limits<std::uint64_t>::max())
        ++m_earned;
    if (m_points < std::numeric_limits<std::uint64_t>::max())
        ++m_points;
}
std::uint64_t CPlayerProgression::points() const
{
    return m_points;
}
std::uint64_t CPlayerProgression::earned() const
{
    return m_earned;
}
unsigned CPlayerProgression::rank(SkillId id) const
{
    const auto index = static_cast<std::size_t>(id);
    return index < m_ranks.size() ? m_ranks[index] : 0;
}
unsigned CPlayerProgression::cost(SkillId id) const
{
    return rank(id) + 1;
}
bool CPlayerProgression::unlocked(SkillId id) const
{
    const auto index = static_cast<std::size_t>(id);
    if (index >= m_ranks.size())
        return false;
    const auto prerequisite = definitions[index].prerequisite;
    return prerequisite == SkillId::Count || rank(prerequisite) > 0;
}
bool CPlayerProgression::canBuy(SkillId id) const
{
    const auto index = static_cast<std::size_t>(id);
    return index < m_ranks.size() && unlocked(id) && rank(id) < definitions[index].maxRank &&
           m_points >= cost(id);
}
bool CPlayerProgression::buy(SkillId id)
{
    if (!canBuy(id))
        return false;
    m_points -= cost(id);
    ++m_ranks[static_cast<std::size_t>(id)];
    return true;
}
void CPlayerProgression::apply(FlightTuning &flight, WeaponTuning &weapon) const
{
    // Apply to freshly loaded base tuning once per run; never compound last run's multipliers.
    weapon.damage *= 1 + .2f * rank(SkillId::Damage);
    weapon.interval /= 1 + .15f * rank(SkillId::FireRate);
    const float handling = 1 + .1f * rank(SkillId::Handling);
    flight.pitchRate *= handling;
    flight.yawRate *= handling;
    flight.rollRate *= handling;
    const float response = 1 + .15f * rank(SkillId::Acceleration);
    flight.acceleration *= response;
    flight.deceleration *= response;
    flight.velocityResponse *= response;
    const float speed = 1 + .1f * rank(SkillId::Speed);
    flight.minSpeed *= speed;
    flight.maxSpeed *= speed;
    flight.initialSpeed *= speed;
    flight.brakeSpeed *= speed;
    flight.boostSpeed *= speed * (1 + .12f * rank(SkillId::Boost));
}
std::span<const SkillDefinition> CPlayerProgression::skills()
{
    return definitions;
}
