#include "EnemyRoster.h"
#include "game/combat/CombatTuning.h"
#include "resource/IResourceManager.h"
#include <algorithm>
#include <cmath>
#include <json/json.h>
#include <limits>
#include <stdexcept>
#include <unordered_map>
namespace
{
float optionalNumber(const Json::Value &object, const char *key, float fallback)
{
    if (!object.isMember(key))
        return fallback;
    if (!object[key].isNumeric() || !std::isfinite(object[key].asFloat()))
        throw std::runtime_error(std::string("Invalid encounter number: ") + key);
    return object[key].asFloat();
}
std::string requiredString(const Json::Value &object, const char *key)
{
    if (!object[key].isString() || object[key].asString().empty())
        throw std::runtime_error(std::string("Missing enemy field: ") + key);
    return object[key].asString();
}
glm::vec3 vector(const Json::Value &value)
{
    if (!value.isArray() || value.size() != 3)
        throw std::runtime_error("Enemy vector needs three coordinates");
    glm::vec3 result;
    for (int axis = 0; axis < 3; ++axis)
    {
        if (!value[axis].isNumeric() || !std::isfinite(value[axis].asFloat()))
            throw std::runtime_error("Invalid enemy vector coordinate");
        result[axis] = value[axis].asFloat();
    }
    return result;
}
} // namespace
EnemyRoster EnemyRoster::load(IResourceManager &resources, const ArenaTuning &arena,
                              const WeaponTuning &weapon)
{
    Json::Value root;
    if (!resources.loadJsonDocument("data/game/defenders_of_cthedra/encounters.json", root))
        throw std::runtime_error("Prepared enemy roster is missing");
    return parse(root, arena, weapon);
}
EnemyRoster EnemyRoster::parse(const Json::Value &root, const ArenaTuning &arena,
                               const WeaponTuning &weapon)
{
    if (!root["enemy_types"].isArray() || !root["waves"].isArray())
        throw std::runtime_error("Encounter needs enemy_types and waves");
    EnemyRoster result;
    if (root.isMember("max_active"))
    {
        if (!root["max_active"].isUInt64() || root["max_active"].asUInt64() == 0)
            throw std::runtime_error("max_active must be positive");
        result.maxActive = std::size_t(root["max_active"].asUInt64());
    }
    std::unordered_map<std::string, std::size_t> ids;
    for (const auto &node : root["enemy_types"])
    {
        EnemyDefinition type;
        type.behavior = arena.enemies;
        type.weapon = weapon;
        type.weapon.color = {2, .3f, .06f};
        type.weapon.interval = arena.enemies.fireInterval;
        type.weapon.damage = arena.enemies.damage;
        type.id = requiredString(node, "id");
        type.name = requiredString(node, "name");
        type.mesh = requiredString(node, "mesh");
        type.material = requiredString(node, "material");
        if (!ids.emplace(type.id, result.types.size()).second)
            throw std::runtime_error("Duplicate enemy type: " + type.id);
        if (node.isMember("scale"))
            type.scale = vector(node["scale"]);
        for (int axis = 0; axis < 3; ++axis)
            if (type.scale[axis] < .05f || type.scale[axis] > 20)
                throw std::runtime_error("Enemy scale out of range");
        type.health = optionalNumber(node, "health", arena.enemies.health);
        if (type.health <= 0 || type.health > 100000)
            throw std::runtime_error("Enemy health out of range");
        if (node.isMember("engine_cue"))
            type.engineCue = requiredString(node, "engine_cue");
        const auto &behavior = node["behavior"];
        type.behavior.speed = optionalNumber(behavior, "speed", type.behavior.speed);
        type.behavior.breakSpeed = optionalNumber(behavior, "breakSpeed", type.behavior.breakSpeed);
        type.behavior.turnRate = optionalNumber(behavior, "turnRate", type.behavior.turnRate);
        type.behavior.velocityResponse =
            optionalNumber(behavior, "velocityResponse", type.behavior.velocityResponse);
        type.behavior.attackRange =
            optionalNumber(behavior, "attackRange", type.behavior.attackRange);
        type.behavior.fireAngle = optionalNumber(behavior, "fireAngle", type.behavior.fireAngle);
        type.behavior.breakDistance =
            optionalNumber(behavior, "breakDistance", type.behavior.breakDistance);
        type.behavior.breakDuration =
            optionalNumber(behavior, "breakDuration", type.behavior.breakDuration);
        type.behavior.separationDistance =
            optionalNumber(behavior, "separationDistance", type.behavior.separationDistance);
        type.behavior.separationStrength =
            optionalNumber(behavior, "separationStrength", type.behavior.separationStrength);
        type.behavior.separationAcceleration = optionalNumber(behavior, "separationAcceleration",
                                                              type.behavior.separationAcceleration);
        type.behavior.separationLookAhead =
            optionalNumber(behavior, "separationLookAhead", type.behavior.separationLookAhead);
        type.behavior.approachSpread =
            optionalNumber(behavior, "approachSpread", type.behavior.approachSpread);
        auto validation = arena;
        validation.enemies = type.behavior;
        validation.validate();
        const auto &gun = node["weapon"];
        if (gun.isMember("color"))
            type.weapon.color = vector(gun["color"]);
        type.weapon.projectileSpeed =
            optionalNumber(gun, "projectileSpeed", type.weapon.projectileSpeed);
        type.weapon.projectileLifetime =
            optionalNumber(gun, "projectileLifetime", type.weapon.projectileLifetime);
        type.weapon.interval = optionalNumber(gun, "interval", type.weapon.interval);
        type.weapon.damage = optionalNumber(gun, "damage", type.weapon.damage);
        if (gun.isMember("muzzles"))
        {
            if (!gun["muzzles"].isArray())
                throw std::runtime_error("Enemy muzzles must be an array");
            type.weapon.muzzles.clear();
            for (const auto &point : gun["muzzles"])
                type.weapon.muzzles.push_back(vector(point));
        }
        CombatTuning combat;
        combat.weapon = type.weapon;
        combat.validate();
        if (node.isMember("decoration"))
        {
            const auto &decoration = node["decoration"];
            type.decoration.mesh = requiredString(decoration, "mesh");
            type.decoration.material = requiredString(decoration, "material");
            type.decoration.scale =
                decoration.isMember("scale") ? vector(decoration["scale"]) : type.scale;
            for (int axis = 0; axis < 3; ++axis)
                if (type.decoration.scale[axis] <= 0 || type.decoration.scale[axis] > 20)
                    throw std::runtime_error("Decoration scale out of range");
        }
        result.types.push_back(std::move(type));
    }
    std::vector<std::size_t> ordinals(result.types.size(), 0);
    std::size_t sequence = 0;
    for (const auto &wave : root["waves"])
    {
        const auto id = requiredString(wave, "type");
        const auto found = ids.find(id);
        if (found == ids.end())
            throw std::runtime_error("Unknown enemy type: " + id);
        if (!wave["count"].isUInt64())
            throw std::runtime_error("Wave count must be a nonnegative integer");
        const auto count = wave["count"].asUInt64();
        if (count == 0)
            continue;
        if (count > result.spawns.max_size() - result.spawns.size())
            throw std::runtime_error("Encounter exceeds addressable capacity");
        const float delay = optionalNumber(wave, "delay", 0),
                    interval = optionalNumber(wave, "interval", 1);
        if (delay < 0 || interval < 0 || !std::isfinite(delay + interval * float(count - 1)))
            throw std::runtime_error("Invalid wave timing");
        const bool fixed = wave.isMember("offset");
        const auto offset = fixed ? vector(wave["offset"]) : glm::vec3(0);
        if (fixed && glm::any(glm::greaterThan(glm::abs(offset), glm::vec3(.9f))))
            throw std::runtime_error("Spawn offset is outside the arena");
        for (std::uint64_t i = 0; i < count; ++i)
        {
            const float phase = float(sequence++);
            const float angle = phase * 2.39996323f;
            const auto point =
                fixed ? offset
                      : glm::vec3(std::cos(angle) * .68f, std::sin(phase * 1.7f) * .45f,
                                  std::sin(angle) * .68f);
            result.spawns.push_back({found->second, ++ordinals[found->second],
                                     delay + interval * float(i), phase, point});
        }
    }
    std::stable_sort(result.spawns.begin(), result.spawns.end(),
                     [](const auto &a, const auto &b) { return a.time < b.time; });
    return result;
}
