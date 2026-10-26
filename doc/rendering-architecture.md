# Deferred renderer: first refactoring milestone

The renderer now separates frame orchestration, pass implementation and named
framebuffer storage. This is an organizational refactor: shaders, texture formats,
attachment locations, resolutions, GL state policies and effect order are preserved.

`source/graphics/renderer/CDeferredRenderer.cpp` initializes the passes and executes
the frame. `pass/deferred/` contains the implementation and pass-owned shader IDs:

| Pass | Responsibility | Target contract |
| --- | --- | --- |
| GeometryPass | Opaque/masked geometry and blended-object detection | Writes GBufferTargets: depth, albedoGlow, normalSpecular |
| ShadowPass | Directional depth maps and point-light cube faces | Writes ShadowTargets |
| LightingPass | Ambient and per-light diffuse/specular accumulation; schedules shadows for each light | Reads G-buffer and shadows; writes LightingTargets |
| CompositionPass | Combines albedo/glow with diffuse and specular lighting | Writes sceneColor; shares G-buffer depth |
| BackgroundPass | Environment and projected sun | Updates sceneColor and SunState |
| TransparencyPass | Sorted, lit translucent surfaces | Reads retained shadow maps; blends into sceneColor |
| PostProcessPass | Existing FXAA, fog, blur/DOF, rays, vignette, bloom, flare, tone-map and cel chain | Reads sceneColor/depth/sun; uses PostProcessTargets |
| PresentationPass | Intermediate copies, final encoding and diagnostic views | Writes the selected display destination |

`DeferredRenderTargets.h` names the storage contracts. G-buffer and light-buffer
attachments are named by meaning. Post-processing uses `primary`, `secondary` and
`scratch` color targets because each allocation is reused by several effects;
`output` identifies the selected final texture. There is no resource pool or render
graph in this milestone.

The coordinator owns the targets and constructs passes with references to only the
groups they use. Read-only attachment contracts use const references where
applicable. Targets outlive all pass objects. Passes cannot be copied, preventing
references from accidentally remaining bound to another renderer's targets.

The shared fullscreen quad is an explicit resource dependency. `MaterialDraw`
contains the existing mesh/material binding path used by geometry, shadows,
background and transparency. Small protected forwarding methods preserve the
existing numerical test fixtures; rendering implementations no longer live in the
coordinator.

The frame still runs geometry, lighting (including per-light shadows), composition,
background, transparency, shadow-cache pruning, post-processing and presentation
in the same order. Root state preservation, feedback validation and profiling remain
in place. The shared transform state is retained to preserve the existing camera
matrix flow. Scene/resource APIs, shader bindings and the internal post-process
allocation strategy are deliberately left for separate milestones.

## Output preservation checks

A project-local reference harness compiled the saved pre-refactor renderer and the
extracted renderer against the same assets, shaders and graphics resources. It
rendered an opaque and translucent fixture with point/directional shadows and six
effect configurations, reading only its own offscreen framebuffer as numeric data.
All 76,050 floating-point samples were byte-identical on the local NVIDIA driver.
The reference had nonconstant output (6,360 distinct values), with four distinct
outputs among the six configurations. SHA-256 of both sample streams:

`492E53552906A1804EF6AFBAFE34B9032F39C82D3C403540533544E8025DFA52`

The one-off reference sources and results remain under ignored
`build/refactor-baseline/`; they are not part of the build or shipped package.
This verifies that fixture, rather than claiming visual equivalence for every
possible scene. The regular CPU/GPU regression and game/demo smoke tests provide
the broader correctness checks.

Final validation: all 14 tests passed in Debug and Release on this machine. The
Release package was refreshed and passed a three-frame hidden startup/render/shutdown
check from another project directory.
