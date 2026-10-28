#include "CombatTuning.h"
#include "resource/IResourceManager.h"
#include <cmath>
#include <json/json.h>
#include <stdexcept>
#include <string>

CombatTuning CombatTuning::load(IResourceManager &resources)
{
    Json::Value root;
    if (!resources.loadJsonDocument("data/game/defenders_of_cthedra/combat.json", root))
        throw std::runtime_error("Prepared combat configuration is missing");
    CombatTuning result;
    const auto number = [](const Json::Value &section, const char *key)
    {
        if (!section[key].isNumeric())
            throw std::runtime_error(std::string("Missing numeric combat setting: ") + key);
        return section[key].asFloat();
    };
    const auto &weapon = root["weapon"], &targeting = root["targeting"], &engine = root["engine"];
    if (weapon["color"].isArray() && weapon["color"].size() == 3)
        result.weapon.color = {weapon["color"][0].asFloat(), weapon["color"][1].asFloat(),
                               weapon["color"][2].asFloat()};
    result.weapon.projectileSpeed = number(weapon, "projectileSpeed");
    result.weapon.projectileLifetime = number(weapon, "projectileLifetime");
    result.weapon.damage = number(weapon, "damage");
    result.weapon.interval = number(weapon, "interval");
    if (!weapon["muzzles"].isArray())
        throw std::runtime_error("muzzles must be an array");
    result.weapon.muzzles.clear();
    for (const auto &offset : weapon["muzzles"])
    {
        if (!offset.isArray() || offset.size() != 3 || !offset[0].isNumeric() ||
            !offset[1].isNumeric() || !offset[2].isNumeric())
            throw std::runtime_error("Each muzzle needs three model-space coordinates");
        result.weapon.muzzles.emplace_back(offset[0].asFloat(), offset[1].asFloat(),
                                           offset[2].asFloat());
    }
    result.targeting.range = number(targeting, "range");
    result.targeting.acquisitionAngle = number(targeting, "acquisitionAngle");
    result.engine.idleGain = number(engine, "idleGain");
    result.engine.fullGain = number(engine, "fullGain");
    result.engine.boostGain = number(engine, "boostGain");
    result.engine.idlePitch = number(engine, "idlePitch");
    result.engine.fullPitch = number(engine, "fullPitch");
    result.engine.boostPitch = number(engine, "boostPitch");
    result.engine.response = number(engine, "response");
    result.validate();
    return result;
}
void CombatTuning::validate() const
{
    const auto range = [](const char *name, float value, float low, float high)
    {
        if (!std::isfinite(value) || value < low || value > high)
            throw std::runtime_error(std::string("Invalid combat setting: ") + name);
    };
    for (int axis = 0; axis < 3; ++axis)
        range("weapon color", weapon.color[axis], 0, 8);
    range("projectileSpeed", weapon.projectileSpeed, 1, 500);
    range("projectileLifetime", weapon.projectileLifetime, .1f, 10);
    range("damage", weapon.damage, 1, 1000);
    range("interval", weapon.interval, .05f, 2);
    if (weapon.muzzles.empty() || weapon.muzzles.size() > 8)
        throw std::runtime_error("Need 1..8 muzzle offsets");
    for (const auto &offset : weapon.muzzles)
        for (int axis = 0; axis < 3; ++axis)
            range("muzzle", offset[axis], -50, 50);
    range("target range", targeting.range, 10, 1000);
    range("acquisition angle", targeting.acquisitionAngle, 1, 89);
    range("idleGain", engine.idleGain, 0, 2);
    range("fullGain", engine.fullGain, 0, 2);
    range("boostGain", engine.boostGain, 0, 2);
    range("idlePitch", engine.idlePitch, .5f, 2);
    range("fullPitch", engine.fullPitch, .5f, 2);
    range("boostPitch", engine.boostPitch, .5f, 2);
    range("engine response", engine.response, .1f, 30);
    if (engine.idleGain > engine.fullGain || engine.fullGain > engine.boostGain ||
        engine.idlePitch > engine.fullPitch || engine.fullPitch > engine.boostPitch)
        throw std::runtime_error("Engine idle/full/boost levels must increase in order");
}
