#pragma once
#include "resource/AsyncLoading.h"
#include <array>
#include <atomic>
#include <memory>
#include <thread>
#include <unordered_map>
#include <vector>
class CResourceManager;

struct AsyncPublication
{
    EResourceType type;
    ResourceId local;
    std::string key;
    bool document = false;
};
