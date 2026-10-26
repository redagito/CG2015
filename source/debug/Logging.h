#pragma once
#include <spdlog/spdlog.h>
#include <spdlog/sinks/ringbuffer_sink.h>
#include <memory>
#include <string>

// Project sink configuration only. Call sites use spdlog's typed API directly.
namespace logging {
void initialize(const std::string& logFile={});
std::shared_ptr<spdlog::sinks::ringbuffer_sink_mt> overlaySink();
}
