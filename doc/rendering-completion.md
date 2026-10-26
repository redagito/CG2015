# Rendering follow-up completion

All nine remaining findings from the rendering review are implemented. The first structural refactoring milestone is documented in [Rendering architecture](rendering-architecture.md). This is a
linear HDR deferred renderer with a separate translucent pass and Blinn-Phong
lighting. It is not a physically based material system.

## Materials and output

Material JSON accepts `alpha_mode` (`opaque`, `mask`, `blend`), `alpha_cutoff`
(default 0.5), `opacity` (default 1), `color_space` (`srgb` by default, or `linear`),
and `background` (default false). File-loaded materials with an alpha image default
to mask mode. Directly created materials default to opaque; use
`setMaterialSettings` to change them.

Albedo uses a separate sRGB texture view/cache; normal, specular, alpha and other
data maps remain linear. G-buffer colors and post-process intermediates use linear
HDR values. Only the final display pass encodes sRGB. Existing art can look different
because its encoded values are no longer used directly in lighting.

Mask mode discards matching pixels in geometry and both shadow passes. Blend mode
sorts object centers back-to-front, depth-tests against opaque geometry, and keeps
depth writes disabled. Each object receives ambient/emissive and all direct lights,
including opaque point/directional shadows. Blended surfaces do not cast opaque
shadows. Intersecting translucent surfaces retain the usual object-sort limitations.

The specular map scales a bounded dielectric specular response with exponent 32.
Colored highlights accumulate independently of diffuse illumination.

## Sky, sunlight and shadows

Background materials follow the camera and render once at far depth. Without a
background asset, a gradient supplies the environment. The strongest valid
directional light supplies the visible HDR sun disc and its projected screen
position. Rays sample a depth-based occlusion mask near that disc; arbitrary white
objects no longer become ray sources. Sources behind or outside the view produce
no rays.

Camera feature settings expose `shadowDistance` (200), `shadowMapResolution`
(4096) and `pointShadowResolution` (1024). Directional coverage fits the camera
frustum up to that distance, snaps to texels and derives bias from world texel size
and depth span. Vertical lights and distant cameras are covered by regression
tests. This is one fitted map per directional light, without cascades. Translucent
receivers retain per-light maps only while needed.

## Resources and pass boundaries

`CResourceManager` supports late listener replay, `updateString`, `reloadShader`,
`reloadResource(type, id)` and `removeResource(type, id)`. File-backed images,
meshes, materials, models and shader sources reload into existing GPU wrappers.
Shader reload expands includes and relinks programs that share changed stages.
Invalid compilation/linking raises an error. Listener attachment rolls back if
replay fails. One graphics manager uses one CPU resource namespace.

Deletion rejects resources referenced by CPU models/materials/shaders. Callers
must first release scene references with `removeObject`; arbitrary external raw
pointers are not tracked. Model instances resolve current IDs/bounds after reload.
Shader include paths are confined to their include directory and cycles fail.

The root renderer restores caller framebuffer/program/VAO, viewport, texture and
sampler bindings, and relevant raster/depth/blend state. Passes establish their
own state. Program activation checks the actual context binding. The default-on
`CG2015_VALIDATE_PASSES` option detects sampled draw-attachment aliases before
issuing draws, including writable depth attachments. It adds validation overhead.

## Timing and validation

F2 displays top-level CPU/GPU stage timings. `renderer.profiler()` exposes
`cpuMilliseconds()` and `gpuMilliseconds()` maps, including individual effects and
shadow stages. GPU queries are collected only when available, with bounded pending
storage; rendering never waits for timing results. Values are the most recently
completed scope per label, not averages or summed repeated invocations. Disable
pass validation when measuring its overhead separately. No speedup is claimed.

The 14-test suite includes CPU math/include tests, real-driver numerical tests of
sRGB round trips, masks/shadows, colored specular, sorted blending and shadowed
translucency, projected/occluded sunlight, reload/replay/delete, state restoration,
feedback detection and timings. It also checks fail-fast GL behavior and runs the
game plus six deferred demos for three frames each. Tests inspect only their own
offscreen framebuffer data. They do not establish visual equivalence or complete
gameplay coverage. Validation is on the local NVIDIA Windows driver; other vendors
and Linux remain untested.
