# Swarm performance and FPS enforcement

The swarm starts with ten enemies and doubles through 5,120 on stage 10 (10,230 enemies across a run). The active cap is 5,120. Stage 3 has forty enemies; its balance is a playtesting question rather than an enforced skill prerequisite.

## Current maximum-upgrade measurements

Measured on this PC on 2026-10-29 with an NVIDIA GeForce RTX 5060 Ti, Release MSVC build with cross-file optimization (IPO), OpenGL 4.6 core synchronous debug context, and framebuffer feedback validation enabled. All 24 skills are at rank five. Earlier six-skill measurements are superseded by this heavier workload.

All 5,120 enemies remain simultaneously alive and finite. The benchmark renders the arena, temple, UFO, HUD and effects, fires the fully upgraded eleven-projectile player volleys with piercing and splash, simulates every enemy's movement/firing/projectiles, and mixes bounded offline audio. Actors are invulnerable only in the benchmark to preserve the full population. Kill-chain correctness is tested separately; this fixture does not measure mass-death bursts. Geometry frustum culling is disabled for the stress view.

Variable completed-frame durations drive simulation, matching runtime timing. Qualification runs measure at least 1,800 frames and 65 simulated seconds after three presentation warm-up frames. Vsync is disabled and glFinish includes GPU completion. No screenshots are captured.

| Actual framebuffer | Frames | Simulated duration | Average FPS | Minimum one-second average FPS | Slowest frame |
|---|---:|---:|---:|---:|---:|
| 1920x1080 | 3299 | 65.01 s | 50.74 | 40.35 | 35.75 ms |

This run uses the enlarged temple, obelisks and mothership, the 1,000-unit-high flight volume, restored dark lighting, procedural red enemy hull/exhaust emission and the same 12,000-unit camera far plane as gameplay. The log is `build/balance-grace-swarm.log`. This run includes the mixed final-wave roster (3,583 raiders, 1,280 armored fighters, 256 elites and one guardian), rebalanced maximum upgrades, and the full 60-second approach plus five-second playable grace. Every measured one-second window stayed above 30 FPS. The slowest individual measured frame took 35.75 ms; individual hitches are allowed by the one-second-average requirement.

For historical comparison, before the landmark/lighting changes, the 2026-10-28 run at an actual 2560x1421 framebuffer averaged 46.66 FPS with a minimum one-second average of 43.97 FPS. That higher-resolution result has not been requalified with the new scene. A sustained minimum of 60 FPS remains a stretch target. Results vary with hardware, resolution, background work and encounter behavior; these are measurements of defined scenarios, not a universal guarantee.

## Runtime guard

CFrameRateGuard measures completed active-gameplay frames in non-overlapping one-second windows. Below 30 FPS, the engine logs the measured rate and exits through the normal exception path with a failure exit code. An individual slow frame is tolerated when the window average remains at least 30 FPS. Loading, menus, results, state-entry initialization, cinematics and minimized windows reset measurement. The first complete active-gameplay frame starts monitoring, without an extra multi-second grace period.

## Repeat the benchmark

Build Release with GPU tests enabled, then run from the project directory:

```
.\build\Release\cg2015_swarm_benchmark.exe 1800 1920 1080
```

Arguments are minimum frame count, requested width and requested height. Runs requesting at least 900 frames also cover at least 65 simulated seconds. The benchmark fails on loading/spawn failure, a population below 5,120, non-finite positions, no complete timing window, or a one-second average below 30 FPS. Normal qualification stops early on an FPS failure. It is intentionally separate from CTest because machine load changes timing.

An optional final `profile` argument enables per-controller CPU timings at a fixed 30 Hz simulation step. That diagnostic mode reports timings rather than qualifying performance; its instrumentation adds overhead and its successful exit code does not mean the FPS requirement passed.

## Supporting changes

- Opaque geometry and shadows instance objects by mesh, material and mirrored winding. Numerical GPU comparisons verify material and normal transforms against reference draws. Feedback validation remains enabled per batch, with synchronous core GL error reporting.
- Every enemy still moves and fires each frame. Long movement steps subdivide at 50 ms; separation decisions run at staggered 20 Hz and use sixteen nearby threats from a bounded local sample. This avoids both dense all-pairs work and catch-up feedback near the FPS floor.
- Spawning uses an incremental spatial index, configurable initial clearance and retries throughout the arena. Object-to-ID lookup is constant time.
- Projectile queries lazily build a per-group spatial index over swept receiver bounds. Friendly groups are rejected before visiting their cells; stable collider IDs prevent piercing shots from hitting the same target twice after allocator reuse. Zero-damage ordinary overlaps skip unnecessary checks. Moving targets, rotated extents, large bounds and exact ties retain their sweep semantics.
- Impact/chain damage is deferred and processed in bounded batches, without recursive death callbacks. Cosmetic particles and audio voices remain bounded independently of damaging projectiles and enemy counts.

Multishot, piercing, splash and kill-chain upgrades now provide crowd-clearing options for the exponential campaign. The new volley budget, actual-hull-damage lifesteal, separate healing refill limits, and direct-kill-only bursts prevent multiplicative sustain. Their final-wave balance still requires hands-on playtesting.
