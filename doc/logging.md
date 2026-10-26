# spdlog logging

Logging now uses [spdlog 1.17.0](https://github.com/gabime/spdlog/releases/tag/v1.17.0)
directly. Conan pins the recipe revision in `conan.lock`. The library is linked
statically and uses C++23's standard formatting support, without an additional
fmt dependency. The package includes its MIT license in `data/licenses/spdlog.txt`.

Call sites use spdlog's `SPDLOG_INFO`, `SPDLOG_WARN`, `SPDLOG_ERROR`, etc. macros
with checked brace formatting. The old printf-style argument formatting and its
fixed 5,000-byte buffer are removed. Source filename, line and function are retained.
Debug messages compile out in Release; info, warnings and errors remain enabled.

`debug/Logging.cpp` only configures standard spdlog sinks during startup:

- A thread-safe colored stderr sink with level and message.
- A thread-safe basic file sink under `log/`, with timestamps, thread IDs and source
  locations. Warning/error messages flush immediately; the sink flushes on close.
- A thread-safe ring-buffer sink holding the latest 64 messages. F2 reads a snapshot
  of the latest ten messages on the render thread.

Worker threads write through spdlog's synchronized sinks. There are no logger
listeners, callbacks into UI objects, custom worker-message queues or logging
pump calls. `CDebugInfo` holds only the main-thread snapshot and debug values.
Sink configuration occurs before workers start; it is not a runtime reconfiguration
API. The logger uses synchronous sinks, rather than a background logging queue.

OpenGL debug messages also go through spdlog and are flushed before fail-fast
termination. The driver callback catches logging exceptions and retains a minimal
stderr fallback so no exception crosses the driver's C boundary. GL errors,
undefined behavior and high severity still abort synchronously.

## Removed unused code

- `CLogger`, `ILogListener` and the `LOG_*` compatibility macros.
- The uninstrumented legacy `CProfiler` and `PROFILE_START` macros. The active
  CPU/GPU render profiler remains.
- The obsolete hand-written `CObjModelLoader`; the active tinyobjloader path remains.
- `SRenderRequest`, `CScreenSpacePass`, the incomplete `CPostProcessPass` and unused
  `IRenderPass` interface.
- The unused `CScreenQuadPass`, its initialization-only shader and orphaned test
  shaders. Actual full-screen passes still use the shared quad mesh.
- The unused log-buffer resize API, including its invalid empty-buffer trimming.

The `logging` test checks typed formatting, literal braces/percent signs, messages
longer than the old buffer, concurrent worker writes and snapshots, bounded history,
file/source output, duplicate initialization and Debug/Release filtering. The full
suite now contains 18 tests, including async loading and real-driver rendering.

Final validation: all 18 tests passed in Debug and Release. The refreshed package
completed 1,000 hidden frames from another project directory, reached the title,
and produced a structured file log with thread/source fields. Exit code: 0.
