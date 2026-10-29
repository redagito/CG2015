# CG2015

A university graphics/game project from 2015, updated for C++23 and OpenGL 4.6 core.
The original deferred renderer and game assets are preserved. The experimental forward renderer has been removed.

## Build on this PC

Uses the installed CMake (validated with 4.4.3), Conan 2 (2.33.0), and Visual Studio
2026 C++ tools (MSVC 19.51, x64). Dependencies are managed by Conan; no submodule checkout is needed.

From the project directory in PowerShell:

```powershell
./scripts/build.ps1 -GpuTests
./build/Debug/CG2015.exe
```

`-GpuTests` enables hidden-window tests; it requires a GPU/driver providing OpenGL
4.6 core debug contexts. The normal regression suite does not need a GPU. Tests do
not capture screenshots or capture the mouse. Build/cache/temporary files stay
inside the project. The build script also sets the project-local default Conan profile to C++23. The checked-in Windows profile matches this PC; on a different
compiler installation, adjust `profiles/windows-msvc`.

```powershell
./scripts/build.ps1 -Configuration Release -GpuTests -Package
./build/package/Release/CG2015.exe
```

The package includes `data/` and can be launched from another working directory.
UI fonts, glyph atlases and their licenses are bundled in `data/ui/`; no system
fonts or runtime downloads are required. The title uses a responsive text overlay;
see `doc/title-ui.md`.

PNG/OBJ decoding and resource-file preparation run on worker threads. The sphere
scene displays real loading progress, then waits for Enter after assets and GPU
uploads are ready. See `doc/async-loading.md` for manifests, APIs and upload limits.
Dependencies are linked statically (the Visual C++ runtime is dynamic). Install the
matching Microsoft Visual C++ redistributable when moving a Release package to a
PC without it. Debug builds require the development runtime.

## Build details

CMake 3.28+ and Conan 2 are required. Dependency versions and recipe revisions are
pinned in `conanfile.py` and `conan.lock`. Conan generates CMake dependency targets,
a toolchain, and IDE presets. In a PowerShell session:

```powershell
$env:CONAN_HOME = "$PWD/.conan2"
$env:TEMP = "$PWD/build/tmp"
$env:TMP = $env:TEMP
New-Item -ItemType Directory -Force $env:TEMP | Out-Null
conan install . -pr:h=profiles/windows-msvc -pr:b=profiles/windows-msvc --build=missing
cmake --preset conan-default -DCG2015_GPU_TESTS=ON
cmake --build --preset conan-debug
ctest --preset conan-debug --output-on-failure
```

For Linux, use a native Conan profile with C++23, install the GLFW platform development
packages, then `conan install . --build=missing -s compiler.cppstd=23`, followed by
the generated configure/build presets. Linux has not been validated in this update.
macOS cannot provide OpenGL 4.6 and is rejected at configure time.

## Run and diagnose

```powershell
./build/Debug/CG2015.exe --help
./build/Debug/CG2015.exe --hidden --frames 3
./build/Debug/CG2015.exe --config data/startup.ini
```

The default startup configuration launches the original game. Change `[mode] type`
to `demo` and `[scene] file` to a `data/world/test_*.json` scene to explore the renderer.
`[renderer] type=deferred` is the supported renderer. Escape exits; 1 toggles mouse capture in demo mode, F2 toggles the debug overlay, and F8 toggles frustum culling. Gameplay uses mouse steering, W/S throttle, A/D roll, Shift boost, Space brake and left mouse fire. Tab cycles targets, T selects near the gun line, and X clears selection. See [flight controls and tuning](doc/flight-controls.md) and [combat and arena behavior](doc/combat-and-arena.md).

A 4.6 **core debug context is mandatory in every build**, including Release.
glad 2.0.8 replaces the old 4.3 loader, generated for OpenGL 4.6 core without extensions; shaders target GLSL 460 core. Functions are resolved through GLFW, with native GL debug reporting in both Debug and Release. Debug output is
synchronous: GL errors, undefined behavior, and high-severity messages print to
stderr and abort immediately, keeping the offending GL call on the debugger stack.
Medium/low diagnostics are printed; notifications are suppressed. There is no
silent downgrade to an older GL version. The graphics suite includes an isolated
process that intentionally makes an invalid GL call to verify termination.

Logs are created automatically in `log/`. Warnings and errors remain enabled in Release. Some OBJ files refer to absent Blender
`.mtl` files; these warnings are expected because materials come from the separate
JSON material assets. Driver performance diagnostics may also appear.

Logging uses spdlog with typed formatting and thread-safe console, file and debug
overlay sinks. See `doc/logging.md` for configuration, tests and removed legacy code.

## Scope of this update

- Target-scoped CMake, C++23, Conan dependencies, generated presets, lockfile,
  repeatable Debug/Release build/test/package commands.
- Strict GL context/version validation, driver identification, synchronous debug
  callback, direct-state-access index buffer uploads.
- GPU cleanup before context destruction, window cleanup, safe minimize events,
  correct program deletion/link diagnostics and vertex draw counts.
- Updated OBJ API with multiple shapes, triangulation, complete index remapping,
  missing UV/normal handling and transactional failures.
- Explicit degree-to-radian camera projections, 90-degree cubemap shadow projection,
  initialized GLM defaults, monotonic timing, correct startup title/renderer selection.
- CPU regression tests and optional real-driver smoke tests of all six demo scenes
  through the deferred renderer, plus game startup and numerical GPU regression tests.

The rendering review and resolved findings are documented in `doc/rendering-review.md`; material settings, reload APIs, profiling and design limits are in `doc/rendering-completion.md`. The pass layout and output-preservation checks are described in `doc/rendering-architecture.md`. Broader modernization work is described in `doc/modernization.md`.
# Screenshots

![](https://github.com/redagito/CG2015/wiki/screen_game_1.png)

## Testscenes
![](https://github.com/redagito/CG2015/wiki/testscene_1.png)

![](https://github.com/redagito/CG2015/wiki/testscene_2.png)

![](https://github.com/redagito/CG2015/wiki/testscene_3.png)

Godray with lense flare
![](https://github.com/redagito/CG2015/wiki/testscene_4.png)

## Features
* OpenGL 4.6 core debug context
* Deferred renderer
* Shadow mapping
* Omni-directional shadow mapping
* HDR rendering with fixed (non-adaptive) tonemapping
* Bloom
* Depth of field (Gauss blur based)
* FXAA
* Fog
* God ray
* Vignette with distortion/blur effect
* Lense flare
* View frustum culling (bounding sphere based)
* State based game system

Audio setup, bundled assets and loading behavior are described in [doc/audio.md](doc/audio.md).

Combat configuration and behavior: [combat and arena](doc/combat-and-arena.md), [combat feedback](doc/combat-effects.md). Enemy types/waves live in `encounters.json`; player hit/shield/camera feedback lives in `effects.json`.

The game now has ten clear-to-advance waves, session skill progression, a result-screen skill tree and retries. The arena is 1,600 � 1,600 units; the UFO arrives on wave 10 with a 60-second approach and five seconds of playable grace. [Progression, controls and wave configuration](doc/progression-and-waves.md).

Swarm preset: ten enemies initially, doubling through 5,120 on wave 10. Active gameplay exits if its one-second FPS average falls below 30. See [swarm performance and benchmark](doc/swarm-performance.md).

The skill tree now has 24 skills in four tabs, including multishot, piercing, splash/chain explosions, shields, armor and repairs. Floor/ceiling warnings use a separate 12-unit margin. [Upgrade details and controls](doc/progression-and-waves.md).
