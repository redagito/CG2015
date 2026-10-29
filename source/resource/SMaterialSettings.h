#pragma once
#include "SEmissiveAccents.h"

enum class AlphaMode
{
    Opaque,
    Mask,
    Blend
};
struct SMaterialSettings
{
    AlphaMode alphaMode = AlphaMode::Opaque;
    float alphaCutoff = 0.5f;
    float opacity = 1.f;
    bool srgb = true;
    bool background = false;
    SEmissiveAccents emissiveAccents;
};
