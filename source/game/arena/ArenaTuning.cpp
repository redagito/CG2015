#include "ArenaTuning.h"
#include "resource/IResourceManager.h"
#include <cmath>
#include <json/json.h>
#include <stdexcept>
#include <string>
ArenaTuning ArenaTuning::load(IResourceManager &resources)
{
    Json::Value root;
    if (!resources.loadJsonDocument("data/game/defenders_of_cthedra/arena.json", root))
        throw std::runtime_error("Prepared arena configuration missing");
    const auto number = [](const Json::Value &section, const char *key)
    {
        if (!section[key].isNumeric())
            throw std::runtime_error(std::string("Missing arena setting: ") + key);
        return section[key].asFloat();
    };
    const auto vector = [](const Json::Value &value)
    {
        if (!value.isArray() || value.size() != 3 || !value[0].isNumeric() ||
            !value[1].isNumeric() || !value[2].isNumeric())
            throw std::runtime_error("Invalid arena vector");
        return glm::vec3(value[0].asFloat(), value[1].asFloat(), value[2].asFloat());
    };
    ArenaTuning result;
    result.center = vector(root["center"]);
    result.templeScale = number(root, "templeScale");
    result.mothership.scale = number(root["mothership"], "scale");
    result.mothership.start = vector(root["mothership"]["start"]);
    result.mothership.destination = vector(root["mothership"]["destination"]);
    result.halfExtents = vector(root["halfExtents"]);
    result.warningMargin = number(root, "warningMargin");
    result.verticalWarningMargin = root.get("verticalWarningMargin", 12).asFloat();
    result.returnRate = number(root, "returnRate");
    result.returnResponse = number(root, "returnResponse");
    result.velocityDamping = number(root, "velocityDamping");
    result.mothership.approachSeconds = number(root["mothership"], "approachSeconds");
    result.mothership.waitSeconds = number(root["mothership"], "waitSeconds");
    const auto &enemy = root["enemies"];
    result.enemies.separationDistance = number(enemy, "separationDistance");
    result.enemies.separationStrength = number(enemy, "separationStrength");
    result.enemies.separationAcceleration = number(enemy, "separationAcceleration");
    result.enemies.separationLookAhead = number(enemy, "separationLookAhead");
    result.enemies.approachSpread = number(enemy, "approachSpread");
    result.enemies.speed = number(enemy, "speed");
    result.enemies.breakSpeed = number(enemy, "breakSpeed");
    result.enemies.turnRate = number(enemy, "turnRate");
    result.enemies.velocityResponse = number(enemy, "velocityResponse");
    result.enemies.attackRange = number(enemy, "attackRange");
    result.enemies.fireAngle = number(enemy, "fireAngle");
    result.enemies.breakDistance = number(enemy, "breakDistance");
    result.enemies.breakDuration = number(enemy, "breakDuration");
    result.enemies.fireInterval = number(enemy, "fireInterval");
    result.enemies.damage = number(enemy, "damage");
    result.enemies.health = number(enemy, "health");
    result.enemies.playerHealth = number(enemy, "playerHealth");
    result.validate();
    return result;
}
void ArenaTuning::validate() const
{
    const auto range = [](const char *name, float value, float low, float high)
    {
        if (!std::isfinite(value) || value < low || value > high)
            throw std::runtime_error(std::string("Invalid arena setting: ") + name);
    };
    for (int axis = 0; axis < 3; ++axis)
    {
        range("center", center[axis], -1000, 1000);
        range("halfExtents", halfExtents[axis], 60, 2000);
    }
    range("temple scale", templeScale, 1, 100);
    range("mothership scale", mothership.scale, 1, 1000);
    for (int axis = 0; axis < 3; ++axis)
    {
        range("mothership start", mothership.start[axis], -4000, 4000);
        range("mothership destination", mothership.destination[axis], -4000, 4000);
    }
    range("mothership approach seconds", mothership.approachSeconds, .1f, 3600);
    range("mothership wait seconds", mothership.waitSeconds, 0, 3600);
    range("warningMargin", warningMargin, 10,
          std::min(halfExtents.x, std::min(halfExtents.y, halfExtents.z)) * .8f);
    range("verticalWarningMargin", verticalWarningMargin, 2, halfExtents.y * .8f);
    range("returnRate", returnRate, 90, 240);
    range("returnResponse", returnResponse, 1, 10);
    range("velocityDamping", velocityDamping, 1, 40);
    range("separation distance", enemies.separationDistance, 8, 80);
    range("separation strength", enemies.separationStrength, 0, 5);
    range("separation acceleration", enemies.separationAcceleration, 0, 50);
    range("separation look-ahead", enemies.separationLookAhead, 0, 3);
    range("approach spread", enemies.approachSpread, 0, 80);
    range("enemy speed", enemies.speed, 8, 60);
    range("break speed", enemies.breakSpeed, 8, 80);
    range("enemy turn rate", enemies.turnRate, 20, 120);
    range("enemy response", enemies.velocityResponse, 1, 10);
    range("attack range", enemies.attackRange, 30, 300);
    range("fire angle", enemies.fireAngle, 1, 20);
    range("break distance", enemies.breakDistance, 10, enemies.attackRange * .75f);
    range("break duration", enemies.breakDuration, .5f, 5);
    range("enemy interval", enemies.fireInterval, .3f, 3);
    range("enemy damage", enemies.damage, 1, 100);
    range("enemy health", enemies.health, 1, 1000);
    range("player health", enemies.playerHealth, 1, 2000);
}
