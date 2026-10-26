#pragma once
#include "CFrameBuffer.h"
#include "graphics/SceneConfig.h"
#include "math/CTransformer.h"
#include "resource/ResourceConfig.h"
#include <unordered_map>
#include <unordered_set>

struct SunState
{
    glm::vec2 screen = glm::vec2(0.5f);
    glm::vec3 color = glm::vec3(0.f);
    bool visible = false;
};
