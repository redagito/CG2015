#include "engine/CFrameRateGuard.h"
#include <iostream>
#include <stdexcept>
namespace
{
void require(bool value, const char *message)
{
    if (!value)
        throw std::runtime_error(message);
}
} // namespace
int main()
{
    try
    {
        CFrameRateGuard guard;
        for (int i = 0; i < 180; ++i)
            require(guard.record(1.0 / 60, true), "Healthy 60 FPS failed");
        require(guard.lastFps() > 59.9, "FPS window measured the wrong rate");
        guard.reset();
        for (int i = 0; i < 90; ++i)
            require(guard.record(1.0 / 30, true), "Exact 30 FPS failed");
        guard.reset();
        bool failed = false;
        for (int i = 0; i < 32; ++i)
            failed |= !guard.record(1.0 / 29, true);
        require(failed && guard.lastFps() < 30, "Below-30 one-second average was accepted");
        require(!guard.record(.001, true), "Failed guard silently recovered before exit");
        require(guard.record(5, false), "Loading/menu pause tripped the guard");
        for (int i = 0; i < 60; ++i)
            require(guard.record(1.0 / 60, true), "Inactive pause contaminated the next run");
        guard.reset();
        require(guard.record(.12, true), "Single slow frame caused exit before one second");
        for (int i = 0; i < 60; ++i)
            require(guard.record(1.0 / 60, true), "Healthy average with one hitch was rejected");
        guard.reset();
        for (int i = 0; i < 10; ++i)
            require(guard.record(.05, true), "Guard failed before one second");
        guard.record(0, false);
        for (int i = 0; i < 90; ++i)
            require(guard.record(1.0 / 60, true), "Leaving gameplay did not reset the window");
        std::cout << "FPS guard: one-second averages, 30 FPS boundary, transient hitch, failure "
                     "latch and inactive reset passed.\n";
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
