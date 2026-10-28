#include "CombatEffectsTuning.h"
#include "resource/IResourceManager.h"
#include <cmath>
#include <json/json.h>
#include <stdexcept>
#include <string>
CombatEffectsTuning CombatEffectsTuning::load(IResourceManager &resources)
{
    Json::Value root;
    if (!resources.loadJsonDocument("data/game/defenders_of_cthedra/effects.json", root))
        throw std::runtime_error("Prepared combat effects configuration missing");
    CombatEffectsTuning result;
    if (!root["maxParticles"].isUInt() || root["maxParticles"].asUInt() < 64 ||
        root["maxParticles"].asUInt() > 4094)
        throw std::runtime_error("Particle budget must be 64..4094");
    result.maxParticles = root["maxParticles"].asUInt();
    const auto number = [&](const char *name, float low, float high)
    {
        if (!root[name].isNumeric())
            throw std::runtime_error(std::string("Missing effect setting: ") + name);
        const float value = root[name].asFloat();
        if (!std::isfinite(value) || value < low || value > high)
            throw std::runtime_error(std::string("Invalid effect setting: ") + name);
        return value;
    };
    result.trailInterval = number("trailInterval", .01f, .1f);
    result.trailLifetime = number("trailLifetime", .03f, .5f);
    result.shieldDuration = number("shieldDuration", .1f, 1);
    result.shieldRadius = number("shieldRadius", 1, 8);
    result.cameraKick = number("cameraKick", 0, 1);
    result.resultDelay = number("resultDelay", .2f, 2);
    return result;
}
