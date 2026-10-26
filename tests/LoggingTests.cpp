#include "debug/Logging.h"
#include "debug/CDebugInfo.h"
#include <atomic>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>

namespace {
void require(bool value,const char* message) {if(!value) throw std::runtime_error(message);}
}
int main()
{
    try {
        const std::string path="build/logging-tests/spdlog.log";
        std::filesystem::create_directories("build/logging-tests");
        std::filesystem::remove(path);
        logging::initialize(path);
        auto ring=logging::overlaySink();
        const auto sinkCount=spdlog::default_logger()->sinks().size();
        logging::initialize(path);
        require(spdlog::default_logger()->sinks().size()==sinkCount,"Repeated setup duplicated log sinks");

        const std::string longText(8000,'x');
        SPDLOG_INFO("literal {} payload {}", "{braces} %s",longText);
        auto messages=ring->last_formatted(1);
        require(messages.size()==1 && messages[0].find(longText)!=std::string::npos &&
            messages[0].find("{braces} %s")!=std::string::npos,"Typed log messages were truncated or interpreted twice");

        SPDLOG_DEBUG("debug-level-check");
        messages=ring->last_formatted(1);
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_DEBUG
        require(messages[0].find("debug-level-check")!=std::string::npos,"Debug logging was filtered in Debug");
#else
        require(messages[0].find("debug-level-check")==std::string::npos,"Debug logging was enabled in Release");
#endif
        std::atomic<unsigned> done=0;
        std::vector<std::jthread> workers;
        for(unsigned worker=0;worker<4;++worker) workers.emplace_back([worker,&done] {
            for(unsigned i=0;i<80;++i) SPDLOG_INFO("worker {} message {}",worker,i);
            ++done;
        });
        while(done.load()!=4) {
            require(ring->last_formatted(10).size()<=10,"Overlay snapshot exceeded its limit");
            std::this_thread::yield();
        }
        workers.clear();
        require(ring->last_formatted().size()==64,"The log ring was not bounded");
        CDebugInfo overlay;overlay.setLog(ring->last_formatted(overlay.getLogBufferSize()));
        require(overlay.getLog().size()==10,"The debug overlay did not receive its snapshot");
        for(const auto& line:overlay.getLog()) require(line.find('\n')==std::string::npos && line.find('\r')==std::string::npos,
            "Console line endings became visible overlay glyphs");
        SPDLOG_WARN("flush-test {}",42);
        spdlog::default_logger()->flush();
        std::ifstream file(path);std::string contents((std::istreambuf_iterator<char>(file)),{});
        require(contents.find("worker 3 message 79")!=std::string::npos && contents.find("flush-test 42")!=std::string::npos,
            "Worker messages were not written to the file sink");
        require(contents.find("LoggingTests.cpp:")!=std::string::npos,"File log lost source locations");
        std::cout<<"spdlog: typed formatting, long messages, worker writes, bounded overlay snapshots, file/source output and levels passed.\n";
        return 0;
    }catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
