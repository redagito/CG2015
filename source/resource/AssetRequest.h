#pragma once
#include "ResourceConfig.h"
#include <string>

enum class AssetKind
{
    Mesh,
    Image,
    Material,
    Model,
    Shader,
    Text,
    Json,
    Scene,
    Audio,
    AudioBank
};
struct AssetRequest
{
    AssetKind kind;
    std::string file;
    EColorFormat format = EColorFormat::RGB24;
    bool stream = false;
    unsigned channels = 2;
};
