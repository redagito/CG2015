#pragma once
// Preserve the lethal hit's origin so area damage cannot recursively award combat bonuses.
enum class DamageKind
{
    Direct,
    Splash,
    Chain
};
