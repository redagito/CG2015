#pragma once
#include "ColorTarget.h"
#include "DeferredFrameState.h"
#include "FullscreenResources.h"
#include "GBufferTargets.h"
#include "LightingTargets.h"
#include "PostProcessTargets.h"
#include "ShadowTargets.h"
#include "SunState.h"

struct DeferredRenderTargets
{
    GBufferTargets gbuffer;
    ShadowTargets shadows;
    LightingTargets lighting;
    ColorTarget sceneColor;
    PostProcessTargets post;
};
