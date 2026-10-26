#pragma once
class IResourceManager;

// World units, seconds and degrees. Loaded once from the prepared resource cache.
struct FlightTuning
{
    float minSpeed = 12, maxSpeed = 32, initialSpeed = 22, boostSpeed = 42, brakeSpeed = 8;
    float acceleration = 20, deceleration = 32, velocityResponse = 4;
    float throttleRate = .35f, pitchRate = 80, yawRate = 90, rollRate = 140;
    float angularResponse = 10, autoBank = 28, bankResponse = 6;
    float brakeTurnMultiplier = 1.35f, boostTurnMultiplier = .7f;
    float pointerRadius = .22f, deadZone = .06f, steeringExponent = 1.4f;
    bool invertY = false;
    float cameraDistance = 10, cameraHeight = 3, lookAhead = 35, anticipation = 2;
    float cameraFrequency = 3, cameraRotationResponse = 8, resetDistance = 40;
    float fieldOfView = 65, boostFieldOfView = 8, fieldOfViewResponse = 4;
    static FlightTuning load(IResourceManager &resources);
    void validate() const;
};
