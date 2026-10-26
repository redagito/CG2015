#pragma once
#include "Logging.h"
#include <mutex>

struct LoggingConfiguration
{
    std::mutex mutex;
    std::shared_ptr<spdlog::sinks::ringbuffer_sink_mt> overlay;
    std::shared_ptr<spdlog::logger> logger;
    std::string file;
    LoggingConfiguration();
};
