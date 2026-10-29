#include "game/combat/WeaponTuning.h"
#include "game/encounter/CWaveDirector.h"
#include "game/flight/FlightTuning.h"
#include "game/progression/CPlayerProgression.h"
#include "game/progression/CSkillTreeScreen.h"
#include "resource/Resource.h"
#include "support/TestInput.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
namespace
{
void require(bool value, const char *message)
{
    if (!value)
        throw std::runtime_error(message);
}
} // namespace
int main()
{
    try
    {
        CPlayerProgression progression;
        require(!progression.buy(SkillId::Damage), "Upgrade bought without points");
        for (int i = 0; i < 500; ++i)
            progression.awardKill();
        require(progression.points() == 500 && progression.earned() == 500,
                "Kills did not award one point each");
        require(!progression.buy(SkillId::FireRate) && progression.points() == 500,
                "Prerequisite or failed-purchase accounting broken");
        require(!progression.buy(SkillId::Count), "Invalid skill was purchased");
        for (const auto &skill : CPlayerProgression::skills())
            for (unsigned rank = 1; rank <= 5; ++rank)
            {
                require(progression.cost(skill.id) == rank, "Rank cost does not grow");
                require(progression.buy(skill.id), "Valid ranked purchase failed");
            }
        require(progression.points() == 140 && !progression.buy(SkillId::Damage),
                "Rank cap or total spending broken");
        FlightTuning flight;
        WeaponTuning weapon;
        progression.apply(flight, weapon);
        flight.validate();
        require(std::abs(weapon.damage - 100) < .001f && std::abs(weapon.interval - .15f) < .001f &&
                    std::abs(flight.pitchRate - 140) < .001f && flight.maxSpeed == 64 &&
                    flight.boostSpeed > 167 && flight.acceleration == 45,
                "Skill ranks did not improve actual run tuning");
        require(CPlayerProgression::skills().size() == 24 && weapon.volleySize == 11 &&
                    weapon.modifiers.targets == 6 && weapon.modifiers.splashRadius == 33 &&
                    weapon.projectileSpeed == 180 && weapon.projectileLifetime == 7,
                "Expanded offensive skills did not reach their configured ranks");
        require(weapon.volleyDamageMultiplier == 2 &&
                    std::abs(weapon.criticalChance - .25f) < .001f &&
                    weapon.criticalMultiplier == 2,
                "Volley damage or critical scaling exceeds the intended budget");
        const auto recovery = progression.combatUpgrades();
        require(recovery.killRepair == 10 && recovery.killRepairPerSecond == 10 &&
                    std::abs(recovery.lifesteal - .05f) < .001f && recovery.lifestealPerSecond == 5,
                "Combat healing does not have bounded maximum-rank recovery rates");
        const auto defense = progression.defenses();
        require(defense.hullMultiplier == 1.5f && std::abs(defense.armor - .2f) < .001f &&
                    defense.shieldCapacity == 150 && defense.shieldRegen == 25 &&
                    defense.hullRegen == 2 && defense.shieldRechargeDelay == 4 &&
                    defense.hullRepairDelay == 7,
                "Defensive skill tuning is incorrect");
        std::array<std::array<bool, 6>, 4> layout{};
        for (const auto &skill : CPlayerProgression::skills())
        {
            require(!layout[unsigned(skill.branch)][skill.slot],
                    "Two skills share one card position");
            layout[unsigned(skill.branch)][skill.slot] = true;
        }
        for (const auto &branch : layout)
            for (bool occupied : branch)
                require(occupied, "A skill page has an empty node");
        FlightTuning retryFlight;
        WeaponTuning retryWeapon;
        progression.apply(retryFlight, retryWeapon);
        require(retryWeapon.damage == weapon.damage && retryFlight.maxSpeed == flight.maxSpeed,
                "Retry compounded upgrades");
        CPlayerProgression newSession;
        require(newSession.points() == 0 && newSession.rank(SkillId::Damage) == 0,
                "New session inherited progression");
        CPlayerProgression menuProgress;
        for (int i = 0; i < 5; ++i)
            menuProgress.awardKill();
        CSkillTreeScreen tree(menuProgress);
        TestInput input;
        input.fire = true;
        tree.onEnter(input);
        input.cursor = input.viewport * glm::vec2(.18f, .37f);
        require(tree.update(input) && menuProgress.points() == 5, "Held click purchased on entry");
        input.fire = false;
        tree.update(input);
        input.fire = true;
        tree.update(input);
        tree.update(input);
        require(menuProgress.rank(SkillId::Damage) == 1 && menuProgress.points() == 4,
                "Click purchased more than once");
        input.fire = false;
        tree.update(input);
        input.cursor = input.viewport * glm::vec2(.18f, .60f);
        input.fire = true;
        tree.update(input);
        require(menuProgress.rank(SkillId::FireRate) == 1, "Unlocked child node cannot be clicked");
        input.fire = false;
        tree.update(input);
        input.viewport = {640, 960};
        input.cursor = input.viewport * glm::vec2(.86f, .235f);
        input.fire = true;
        tree.update(input);
        input.fire = false;
        tree.update(input);
        input.cursor = input.viewport * glm::vec2(.5f, .37f);
        input.fire = true;
        tree.update(input);
        require(menuProgress.rank(SkillId::Speed) == 1,
                "Resized tree hit regions differ from drawing");
        input.fire = false;
        tree.update(input);
        input.cursor = input.viewport * glm::vec2(.5f, .82f);
        input.fire = true;
        require(!tree.update(input), "Retry button did not request a fresh run");
        tree.onEnter(input);
        input.fire = false;
        tree.update(input);
        input.targetNext = true;
        tree.update(input);
        input.targetNext = false;
        tree.update(input);
        input.enter = true;
        const auto pointsBeforeKeyboard = menuProgress.points();
        tree.update(input);
        require(menuProgress.points() < pointsBeforeKeyboard,
                "Keyboard selection did not buy an available node");
        input.enter = false;
        tree.update(input);
        input.retry = true;
        require(!tree.update(input), "Keyboard retry did not work");
        CPlayerProgression keyboardProgress;
        for (int i = 0; i < 8; ++i)
            keyboardProgress.awardKill();
        CSkillTreeScreen keyboardTree(keyboardProgress);
        TestInput keyboard;
        keyboardTree.onEnter(keyboard);
        keyboard.skillPage = 3;
        keyboardTree.update(keyboard);
        keyboard.enter = true;
        keyboardTree.update(keyboard);
        keyboardTree.update(keyboard);
        require(keyboardProgress.rank(SkillId::MaxHull) == 1,
                "Keyboard branch selection bought the wrong node or repeated");
        std::unique_ptr<IResourceManager> resources(createResourceManager());
        const auto arena = ArenaTuning::load(*resources);
        const auto roster = EnemyRoster::load(*resources, arena, WeaponTuning{});
        bool hasArmored = false, hasElite = false;
        for (const auto &type : roster.types)
            if (type.id == "armored" || type.id == "elite")
            {
                hasArmored |= type.id == "armored";
                hasElite |= type.id == "elite";
                const auto shots = [&](float damage)
                { return std::ceil(type.health / (damage * (1 - type.armor))); };
                require(shots(50) > shots(weapon.damage) && shots(weapon.damage) > 1 &&
                            shots(weapon.damage * weapon.criticalMultiplier) < shots(weapon.damage),
                        "Damage and critical upgrades do not matter against priority targets");
            }
        require(hasArmored && hasElite, "Campaign has no distinct priority enemy roles");
        require(arena.mothership.approachSeconds == 60 && arena.mothership.waitSeconds == 5,
                "Final wave does not allow one minute plus five playable grace seconds");
        CWaveDirector waves(roster);
        require(waves.waveCount() == 10 && waves.wave() == 0 && !waves.advance() &&
                    arena.halfExtents.x == 800,
                "Campaign is not ten gated waves in the expanded arena");
        std::size_t kills = 0;
        for (std::size_t wave = 0; wave < 10; ++wave)
        {
            require(waves.wave() == wave && waves.finalWave() == (wave == 9),
                    "Final-wave gating is wrong");
            const auto count = waves.remaining();
            require(count == (std::size_t(10) << wave), "Wave population does not double from ten");
            for (std::size_t i = 0; i < count; ++i)
            {
                require(!waves.advance(), "Wave advanced with surviving enemies");
                waves.recordDefeat();
                ++kills;
            }
            require(waves.advance() == (wave < 9), "Clear did not advance exactly one wave");
        }
        require(waves.complete() && kills == roster.spawns.size() && kills == 10230,
                "Campaign completion or roster count wrong");
        std::cout << "Progression: earned points, costs/prerequisites/caps, real stat upgrades, "
                     "session reset, menu input and ten-wave gating passed.\n";
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
