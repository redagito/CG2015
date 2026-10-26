#include "Logging.h"
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/pattern_formatter.h>
#include <filesystem>
#include <mutex>

namespace {
struct Configuration {
    std::mutex mutex;
    std::shared_ptr<spdlog::sinks::ringbuffer_sink_mt> overlay;
    std::shared_ptr<spdlog::logger> logger;
    std::string file;
    Configuration()
    {
        auto console=std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
        console->set_pattern("%^[%l]%$ %v");
        overlay=std::make_shared<spdlog::sinks::ringbuffer_sink_mt>(64);
        // Overlay entries are single lines, not console records with an EOL glyph.
        overlay->set_formatter(std::make_unique<spdlog::pattern_formatter>(
            "[%l] %v",spdlog::pattern_time_type::local,""));
        logger=std::make_shared<spdlog::logger>("CG2015",spdlog::sinks_init_list{console,overlay});
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_DEBUG
        logger->set_level(spdlog::level::debug);
#else
        logger->set_level(spdlog::level::info);
#endif
        logger->flush_on(spdlog::level::warn);
        spdlog::set_default_logger(logger);
    }
};
Configuration& configuration() {static Configuration value;return value;}
}

void logging::initialize(const std::string& logFile)
{
    auto& config=configuration();
    std::lock_guard lock(config.mutex);
    if(logFile.empty() || !config.file.empty()) return;
    const auto parent=std::filesystem::path(logFile).parent_path();
    if(!parent.empty()) std::filesystem::create_directories(parent);
    auto file=std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFile,false);
    file->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [thread %t] [%s:%# %!] %v");
    // Configuration occurs during startup, before any asset workers are started.
    config.logger->sinks().push_back(std::move(file));config.file=logFile;
}

std::shared_ptr<spdlog::sinks::ringbuffer_sink_mt> logging::overlaySink()
{
    return configuration().overlay;
}
