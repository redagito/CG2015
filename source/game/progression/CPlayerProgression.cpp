#include "CPlayerProgression.h"
#include "game/combat/WeaponTuning.h"
#include "game/flight/FlightTuning.h"
#include <limits>
namespace
{
constexpr SkillDefinition definitions[] = {
    {SkillId::Damage, "FIREPOWER", "+20% damage per rank", SkillId::Count, SkillBranch::Gunnery, 0},
    {SkillId::FireRate, "RAPID FIRE", "+20% firing rate per rank", SkillId::Damage,
     SkillBranch::Gunnery, 1},
    {SkillId::Handling, "AGILITY", "+15% turning per rank", SkillId::Count, SkillBranch::Flight, 0},
    {SkillId::Acceleration, "RESPONSIVENESS", "+25% thrust response per rank", SkillId::Handling,
     SkillBranch::Flight, 1},
    {SkillId::Speed, "ENGINE POWER", "+20% flight speed per rank", SkillId::Count,
     SkillBranch::Flight, 2},
    {SkillId::Boost, "AFTERBURNER", "+20% boost speed per rank", SkillId::Speed,
     SkillBranch::Flight, 3},
    {SkillId::Multishot, "MULTISHOT", "+2 shots; +20% volley damage/rank", SkillId::Damage,
     SkillBranch::Gunnery, 2},
    {SkillId::ProjectileSpeed, "HIGH VELOCITY", "+20% shot speed per rank", SkillId::Count,
     SkillBranch::Ordnance, 0},
    {SkillId::ProjectileLifetime, "LONG RANGE", "+15% shot lifetime per rank",
     SkillId::ProjectileSpeed, SkillBranch::Ordnance, 1},
    {SkillId::Piercing, "PIERCING", "+1 target per shot per rank", SkillId::ProjectileSpeed,
     SkillBranch::Ordnance, 2},
    {SkillId::ExplosiveRounds, "EXPLOSIVE ROUNDS", "Impact splash; +10% per rank",
     SkillId::Piercing, SkillBranch::Ordnance, 3},
    {SkillId::BlastRadius, "BLAST RADIUS", "+3 splash radius per rank", SkillId::ExplosiveRounds,
     SkillBranch::Ordnance, 4},
    {SkillId::CriticalChance, "CRITICAL CHANCE", "+5% crit chance per rank", SkillId::FireRate,
     SkillBranch::Gunnery, 3},
    {SkillId::CriticalDamage, "CRITICAL POWER", "+10% crit multiplier per rank",
     SkillId::CriticalChance, SkillBranch::Gunnery, 4},
    {SkillId::MaxHull, "REINFORCED HULL", "+10% hull capacity per rank", SkillId::Count,
     SkillBranch::Defense, 0},
    {SkillId::Armor, "ARMOR", "4% damage reduction per rank", SkillId::MaxHull,
     SkillBranch::Defense, 1},
    {SkillId::Shield, "SHIELD GENERATOR", "+30 shield capacity per rank", SkillId::Armor,
     SkillBranch::Defense, 2},
    {SkillId::ShieldRegen, "SHIELD RECHARGE", "+5 shield/sec/rank after 4s", SkillId::Shield,
     SkillBranch::Defense, 3},
    {SkillId::HullRegen, "REPAIR NANITES", "+0.4 hull/sec/rank after 7s", SkillId::MaxHull,
     SkillBranch::Defense, 4},
    {SkillId::KillRepair, "SALVAGE REPAIR", "+2 hull/kill/rank; capped per sec", SkillId::HullRegen,
     SkillBranch::Defense, 5},
    {SkillId::Lifesteal, "ENERGY SIPHON", "Heal 1% direct hull damage/rank",
     SkillId::CriticalDamage, SkillBranch::Gunnery, 5},
    {SkillId::BoostControl, "BOOST VECTORING", "+6% boost turn factor/rank", SkillId::Boost,
     SkillBranch::Flight, 4},
    {SkillId::BrakeTurn, "COMBAT BRAKES", "+12% brake turn factor/rank", SkillId::Handling,
     SkillBranch::Flight, 5},
    {SkillId::ChainReaction, "CHAIN REACTION", "Direct kills explode; +30 dmg/rank",
     SkillId::BlastRadius, SkillBranch::Ordnance, 5}};
static_assert(std::size(definitions) == static_cast<std::size_t>(SkillId::Count));
} // namespace
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
    weapon.interval /= 1 + .2f * rank(SkillId::FireRate);
    weapon.volleySize = 1 + 2 * rank(SkillId::Multishot);
    weapon.volleyDamageMultiplier = 1 + .2f * rank(SkillId::Multishot);
    weapon.modifiers.targets = 1 + rank(SkillId::Piercing);
    weapon.criticalChance = .05f * rank(SkillId::CriticalChance);
    weapon.criticalMultiplier = 1.5f + .1f * rank(SkillId::CriticalDamage);
    weapon.projectileSpeed *= 1 + .2f * rank(SkillId::ProjectileSpeed);
    weapon.projectileLifetime *= 1 + .15f * rank(SkillId::ProjectileLifetime);
    if (rank(SkillId::ExplosiveRounds) > 0)
    {
        weapon.modifiers.splashRadius =
            8 + 2 * rank(SkillId::ExplosiveRounds) + 3 * rank(SkillId::BlastRadius);
        weapon.modifiers.splashFraction = .3f + .1f * rank(SkillId::ExplosiveRounds);
    }
    flight.boostTurnMultiplier += .06f * rank(SkillId::BoostControl);
    flight.brakeTurnMultiplier += .12f * rank(SkillId::BrakeTurn);
    const float handling = 1 + .15f * rank(SkillId::Handling);
    flight.pitchRate *= handling;
    flight.yawRate *= handling;
    flight.rollRate *= handling;
    const float response = 1 + .25f * rank(SkillId::Acceleration);
    flight.acceleration *= response;
    flight.deceleration *= response;
    flight.velocityResponse *= response;
    const float speed = 1 + .2f * rank(SkillId::Speed);
    flight.minSpeed *= speed;
    flight.maxSpeed *= speed;
    flight.initialSpeed *= speed;
    flight.brakeSpeed *= speed;
    flight.boostSpeed *= speed * (1 + .2f * rank(SkillId::Boost));
}
std::span<const SkillDefinition> CPlayerProgression::skills()
{
    return definitions;
}

DefenseTuning CPlayerProgression::defenses() const
{
    return {1 + .1f * rank(SkillId::MaxHull), .04f * rank(SkillId::Armor),
            30.f * rank(SkillId::Shield), 5.f * rank(SkillId::ShieldRegen),
            .4f * rank(SkillId::HullRegen)};
}
PlayerUpgradeTuning CPlayerProgression::combatUpgrades() const
{
    const auto chains = rank(SkillId::ChainReaction);
    return {2.f * rank(SkillId::KillRepair),
            .01f * rank(SkillId::Lifesteal),
            30.f * chains,
            chains ? 10.f + 2.f * chains : 0,
            2.f * rank(SkillId::KillRepair),
            float(rank(SkillId::Lifesteal))};
}
