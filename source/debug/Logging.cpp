#include "Logging.h"
#include "LoggingConfiguration.h"
#include <filesystem>
#include <mutex>
#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace
{

LoggingConfiguration &configuration()
{
    static LoggingConfiguration value;
    return value;
}
} // namespace

void logging::initialize(const std::string &logFile)
{
    auto &config = configuration();
    std::lock_guard lock(config.mutex);
    if (logFile.empty() || !config.file.empty())
        return;
    const auto parent = std::filesystem::path(logFile).parent_path();
    if (!parent.empty())
        std::filesystem::create_directories(parent);
    auto file = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFile, false);
    file->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [thread %t] [%s:%# %!] %v");
    // LoggingConfiguration occurs during startup, before any asset workers are started.
    config.logger->sinks().push_back(std::move(file));
    config.file = logFile;
}

std::shared_ptr<spdlog::sinks::ringbuffer_sink_mt> logging::overlaySink()
{
    return configuration().overlay;
}
