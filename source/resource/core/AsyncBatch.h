#pragma once
#include "resource/AsyncLoading.h"
#include <array>
#include <atomic>
#include <memory>
#include <thread>
#include <unordered_map>
#include <vector>
class CResourceManager;
#include "AsyncPublication.h"

struct AsyncBatch
{
    std::atomic<LoadPhase> phase{LoadPhase::Preparing};
    std::atomic<unsigned> prepared{0}, total{0};
    std::string error;
    std::unique_ptr<CResourceManager> staged;
    std::vector<AsyncPublication> publications;
    std::array<std::unordered_map<ResourceId, ResourceId>, 7> remap;
    unsigned published = 0;
    std::jthread worker;
};
