#pragma once
#include "ResourceConfig.h"
#include <string>

enum class LoadPhase
{
    Preparing,
    AwaitingUpload,
    Uploading,
    Ready,
    Failed,
    Cancelled
};
struct AsyncLoadStatus
{
    LoadPhase phase = LoadPhase::Preparing;
    unsigned prepared = 0, total = 0, published = 0, uploadTotal = 0;
    std::string error;
    bool terminal() const;
};
