#pragma once
struct ProjectileModifiers
{
    unsigned targets = 1;
    float splashRadius = 0, splashFraction = 0;
    float splashDamage = 0; // Per-projectile noncritical damage, computed when a volley is fired.
};
