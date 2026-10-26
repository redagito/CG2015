# Modernization review

The original project has a useful rendering foundation: deferred shading, point and
directional shadows, HDR, bloom, depth of field, FXAA, fog, and lens flare. Preserving
those features while making builds and diagnostics reliable is a useful first step.
This update keeps the existing architecture and gameplay recognizable.

## Implemented and verified

The build now uses C++23, target-scoped CMake and Conan 2 packages with a revision
lockfile. Dependency sources from 2015 are no longer part of the build. glad 2.0.8
is generated for GL 4.6 core without extensions and uses GLFW's function resolver.
The generated header reports zero extensions. The Conan extension list uses a repeated NONE sentinel because the recipe treats a single NONE as false and otherwise generates all extensions. The native synchronous GL debug callback aborts for errors, undefined behavior,
and high-severity messages, including in Release. Loader debug wrappers are disabled
so they do not consume error flags ahead of the native diagnostics.

Concrete fixes include destruction of GPU resources while their context still
exists, GLFW window destruction, safe zero-size resize events, direct state access
for index uploads, shader program deletion/link-log queries, warnings/errors retained in Release, correct vertex counts
for meshes and overlay text, updated dynamic-buffer sizes, and honoring the startup
renderer/title. GLM default construction is explicitly initialized and camera degree
values are converted to radians. Cubemap faces use 90 degrees. Elapsed-time queries
use a monotonic clock.

OBJ loading now adapts current tinyobjloader's independent indices to the renderer's
single vertex index, combines shapes, triangulates polygons, fills absent UVs and
generates missing normals. Failed loads do not modify an existing mesh. Generated
normals are area weighted across shared position/UV tuples; OBJ smoothing-group
semantics for files without explicit normals are a remaining refinement.

The automated suite covers OBJ edge cases, degree FOV semantics, the clock,
real-driver context/debug/DSA behavior, deliberate fail-fast termination, startup
of the game, and all six demo scenes through the deferred renderer. Smoke tests run
three hidden frames each. These establish startup/render/shutdown behavior, not
visual equivalence or full gameplay coverage. No desktop screenshots are taken.


The bloom and lens-flare composites now render into textures separate from all
sampled inputs. Their original in-place composites caused undefined framebuffer
feedback, consistent with flickering or block-shaped glare artifacts. The flare
shader also handles the exact optical center without normalizing a zero vector.
An offscreen GPU test uses synthetic HDR input to check that flare adds light,
identical frames remain stable, black input stays dark, and pixels remain finite.
It reads only the test application's own framebuffer as numeric data.

## Suggested next work, in order

1. **Simulation and input.** `CEngine::run` has repeated key cooldown logic and uses
   the previous frame's elapsed time for variable-step simulation. Introduce action
   bindings and edge-triggered toggles, then a fixed simulation step with a bounded
   accumulator. Test movement, firing, state transitions, and long pauses before
   changing gameplay behavior.
2. **Game configuration.** `CEngine::initGameSystem` currently ignores its game-file
   argument and hardcodes the states. Make the game JSON authoritative, report
   missing/invalid fields with their paths, and add a state-transition test that
   runs beyond the initial loading screen.
3. **Rendering structure.** Pass state, alias checks, color space, materials,
   resource reload and timings are now implemented; see `rendering-completion.md`.
   The first pass extraction milestone is complete; see `rendering-architecture.md`. Scene preparation and resource ownership can be simplified in later milestones.
4. **Ownership and assets.** Replace remaining raw ownership at API boundaries,
   audit redundant interfaces/legacy parsers, preserve OBJ smoothing groups, and
   remove missing Blender MTL references from assets using JSON materials.
5. **Performance after measurement.** Use the new CPU/GPU timings to select useful
   batching, layout, uniform-buffer or allocation changes. No speed improvement is
   inferred merely from the OpenGL upgrade.

## Practical limitations

The validated platform is Windows x64, Visual Studio 2026, CMake 4.4.3, Conan 2.33.0,
and an NVIDIA RTX 5060 Ti driver exposing OpenGL 4.6. macOS cannot satisfy the GL
requirement. Linux still needs a native toolchain/platform-dependency validation.
Compiler warnings remain in legacy code (unused parameters and narrowing
conversions); they are enabled rather than suppressed globally.

References: [Conan CMake integration](https://docs.conan.io/2/integrations/cmake.html),
[glad](https://github.com/Dav1dde/glad),
[GLFW context hints](https://www.glfw.org/docs/latest/window_guide.html),
[KHR_debug behavior](https://wikis.khronos.org/opengl/Debug_Output).
