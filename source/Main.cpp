#include <charconv>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include "engine/Engine.h"

int main(int argc, const char** argv)
{
    std::string config = "data/startup.ini";
    bool visible = true;
    unsigned int frames = 0;
    for (int i = 1; i < argc; ++i)
    {
        const std::string_view arg(argv[i]);
        if (arg == "--hidden") visible = false;
        else if (arg == "--config" && i + 1 < argc) config = argv[++i];
        else if (arg == "--frames" && i + 1 < argc)
        {
            const std::string_view number(argv[++i]);
            const auto [end, error] = std::from_chars(number.data(), number.data() + number.size(), frames);
            if (error != std::errc() || end != number.data() + number.size() || frames == 0)
            {
                std::cerr << "--frames requires a positive integer.\n";
                return 2;
            }
        }
        else
        {
            std::cout << "Usage: CG2015 [--config file.ini] [--hidden] [--frames count]\n";
            return arg == "--help" ? 0 : 2;
        }
    }
    // Installed bundles can be launched from any working directory.
    if (!std::filesystem::exists("data/startup.ini"))
    {
        const auto executableDir = std::filesystem::absolute(argv[0]).parent_path();
        if (std::filesystem::exists(executableDir / "data/startup.ini"))
            std::filesystem::current_path(executableDir);
    }
    try
    {
        std::unique_ptr<IEngine> engine(createEngine());
        if (!engine->init(config.c_str(), visible)) return 1;
        engine->run(frames);
    }
    catch (const std::exception& error)
    {
        std::cerr << "Fatal error: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
