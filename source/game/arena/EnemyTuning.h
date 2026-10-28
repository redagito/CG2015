#pragma once
struct EnemyTuning
{
    float separationDistance = 28, separationStrength = 2, separationAcceleration = 18,
          separationLookAhead = 1.25f, approachSpread = 35;
    float speed = 24, breakSpeed = 28, turnRate = 65, velocityResponse = 4;
    float attackRange = 180, fireAngle = 7, breakDistance = 28, breakDuration = 1.8f;
    float fireInterval = 1, damage = 12, health = 100, playerHealth = 300;
};
