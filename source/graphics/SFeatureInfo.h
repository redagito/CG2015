#pragma once

enum class FogType
{
    None,
    Linear,
    Exp,
    Exp2
};

enum class RenderMode
{
    Final,
    Color,
    Depth,
    Normals,
    Lights,
    GodRay
};

struct SFeatureInfo
{
    FogType fogType = FogType::None;
    bool fogActive = false; // TODO Use this flag instead FogType::NONE
    float fogBegin = 0.f;
    float fogEnd = 50.f;
    float fogDensity = 0.02f;

    RenderMode renderMode = RenderMode::Final;

    bool shadowsActive = true;
    float shadowDistance = 200.f;
    unsigned int shadowMapResolution = 4096;
    unsigned int pointShadowResolution = 1024;
    bool frustumCullingActive = true; /**< View frustum culling enabled flag. */

    bool dofActive = true;
    float dofNearFocus = 0.5f;
    float dofNearBlur = 0.1f;
    float dofFarFocus = 10.f;
    float dofFarBlur = 150.f;

    bool fxaaActive = true;
    bool lenseFlareActive = true;
    bool godRayActive = true;
    bool toonActive = false;

    bool normalMappingActive = true;
    // Hackyyy
    mutable unsigned int culledObjectCount = 0;
};
