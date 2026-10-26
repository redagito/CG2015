#include "FlightTuning.h"
#include "resource/IResourceManager.h"
#include <cmath>
#include <json/json.h>
#include <stdexcept>

FlightTuning FlightTuning::load(IResourceManager &resources)
{
    Json::Value root;
    if (!resources.loadJsonDocument("data/game/defenders_of_cthedra/flight.json", root) ||
        !root.isObject())
        throw std::runtime_error("Prepared flight configuration is missing");
    FlightTuning tuning;
    const auto number = [&](const char *key, float &value)
    {
        if (!root.isMember(key) || !root[key].isNumeric())
            throw std::runtime_error(std::string("Missing numeric flight setting: ") + key);
        value = root[key].asFloat();
    };
    number("minSpeed", tuning.minSpeed);
    number("maxSpeed", tuning.maxSpeed);
    number("initialSpeed", tuning.initialSpeed);
    number("boostSpeed", tuning.boostSpeed);
    number("brakeSpeed", tuning.brakeSpeed);
    number("acceleration", tuning.acceleration);
    number("deceleration", tuning.deceleration);
    number("velocityResponse", tuning.velocityResponse);
    number("throttleRate", tuning.throttleRate);
    number("pitchRate", tuning.pitchRate);
    number("yawRate", tuning.yawRate);
    number("rollRate", tuning.rollRate);
    number("angularResponse", tuning.angularResponse);
    number("autoBank", tuning.autoBank);
    number("bankResponse", tuning.bankResponse);
    number("brakeTurnMultiplier", tuning.brakeTurnMultiplier);
    number("boostTurnMultiplier", tuning.boostTurnMultiplier);
    number("pointerRadius", tuning.pointerRadius);
    number("deadZone", tuning.deadZone);
    number("steeringExponent", tuning.steeringExponent);
    number("cameraDistance", tuning.cameraDistance);
    number("cameraHeight", tuning.cameraHeight);
    number("lookAhead", tuning.lookAhead);
    number("anticipation", tuning.anticipation);
    number("cameraFrequency", tuning.cameraFrequency);
    number("cameraRotationResponse", tuning.cameraRotationResponse);
    number("resetDistance", tuning.resetDistance);
    number("fieldOfView", tuning.fieldOfView);
    number("boostFieldOfView", tuning.boostFieldOfView);
    number("fieldOfViewResponse", tuning.fieldOfViewResponse);
    if (!root["invertY"].isBool())
        throw std::runtime_error("invertY must be a boolean");
    tuning.invertY = root["invertY"].asBool();
    tuning.validate();
    return tuning;
}

void FlightTuning::validate() const
{
    const auto range = [](float value, float low, float high)
    {
        if (!std::isfinite(value) || value < low || value > high)
            throw std::runtime_error("Flight setting is outside its supported range");
    };
    for (float value : {minSpeed, maxSpeed, initialSpeed, boostSpeed, brakeSpeed})
        range(value, 1, 200);
    if (maxSpeed <= minSpeed || initialSpeed < minSpeed || initialSpeed > maxSpeed ||
        boostSpeed < maxSpeed || brakeSpeed > minSpeed)
        throw std::runtime_error("Invalid flight speed ordering");
    for (float value : {acceleration, deceleration})
        range(value, 1, 200);
    for (float value : {velocityResponse, angularResponse, bankResponse, cameraRotationResponse,
                        fieldOfViewResponse})
        range(value, .1f, 40);
    range(throttleRate, .01f, 3);
    range(pitchRate, 1, 240);
    range(yawRate, 1, 240);
    range(rollRate, 1, 360);
    range(autoBank, 0, 60);
    range(brakeTurnMultiplier, 1, 2);
    range(boostTurnMultiplier, .2f, 1);
    range(pointerRadius, .05f, .4f);
    range(deadZone, 0, .4f);
    range(steeringExponent, .5f, 3);
    range(cameraDistance, 3, 30);
    range(cameraHeight, 0, 10);
    range(lookAhead, 5, 100);
    range(anticipation, 0, 10);
    range(cameraFrequency, .5f, 10);
    range(resetDistance, cameraDistance * 2, 500);
    range(fieldOfView, 40, 100);
    range(boostFieldOfView, 0, 20);
}
