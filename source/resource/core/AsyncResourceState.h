#pragma once
#include "AsyncBatch.h"
#include "CResourceManager.h"
#include "resource/AsyncLoading.h"
#include <array>
#include <atomic>
#include <memory>
#include <thread>
#include <unordered_map>

class CResourceManager;

struct CResourceManager::AsyncState
{
    AsyncLoadId next = 1;
    std::unordered_map<AsyncLoadId, std::shared_ptr<AsyncBatch>> batches;
};
