#include "LoggingConfiguration.h"
#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/stdout_color_sinks.h>

LoggingConfiguration::LoggingConfiguration()
{
    auto console = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
    console->set_pattern("%^[%l]%$ %v");
    overlay = std::make_shared<spdlog::sinks::ringbuffer_sink_mt>(64);
    // Overlay entries are single lines, not console records with an EOL glyph.
    overlay->set_formatter(std::make_unique<spdlog::pattern_formatter>(
        "[%l] %v", spdlog::pattern_time_type::local, ""));
    logger = std::make_shared<spdlog::logger>("CG2015", spdlog::sinks_init_list{console, overlay});
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_DEBUG
    logger->set_level(spdlog::level::debug);
#else
    logger->set_level(spdlog::level::info);
#endif
    logger->flush_on(spdlog::level::warn);
    spdlog::set_default_logger(logger);
}
