#pragma once
#include "resource/AsyncLoading.h"
#include <atomic>
#include <memory>
#include <thread>
#include <unordered_map>
#include <array>

class CResourceManager;
struct AsyncPublication {EResourceType type;ResourceId local;std::string key;bool document=false;};
struct AsyncBatch {
    std::atomic<LoadPhase> phase{LoadPhase::Preparing};
    std::atomic<unsigned> prepared{0},total{0};
    std::string error;
    std::unique_ptr<CResourceManager> staged;
    std::vector<AsyncPublication> publications;
    std::array<std::unordered_map<ResourceId,ResourceId>,6> remap;
    unsigned published=0;
    std::jthread worker;
};
struct CResourceManager::AsyncState {
    AsyncLoadId next=1;
    std::unordered_map<AsyncLoadId,std::shared_ptr<AsyncBatch>> batches;
};
