# Rendering system review — 26 October 2026

## Scope and assessment

Reviewed the deferred geometry/G-buffer, light accumulation, directional and cubemap
shadow passes, illumination composition, post-processing chain, camera/frustum/bounds
math, buffer/texture/framebuffer/shader ownership, graphics resource callbacks,
window/resize integration, and debug overlay. The experimental forward renderer,
its shaders, renderer-switch keys, and its F3 wireframe path were removed at the
user's request. The six demo scenes now exercise the deferred renderer only.

The largest correctness problems were implicit GL state, incomplete resize and
ownership handling, incorrect culling/normal math, and settings that never reached
the shaders. A clean GL debug log did not prove correct rendering: several of these
faults require numerical output checks. The renderer remains useful for learning,
but its lighting/material model is intentionally simple and should not be treated as a
physically based renderer.

## Confirmed bugs fixed during this review

| Priority | Problem and consequence | Fix / evidence |
| --- | --- | --- |
| P1 | `CFrameBuffer::resize` recreated renderbuffers and called attachment code which bound framebuffer zero. The geometry pass resized after binding, so its first draw after resize could target the window instead of the G-buffer. | Named framebuffer operations and in-place renderbuffer storage preserve bindings. GPU tests check binding and completeness after resize. |
| P1 | `CRenderBuffer` never set its initialized flag, leaking objects on resize and shutdown. Its dimensioned constructor was declared but undefined. | Implemented constructor, stable object IDs, initialized state and unconditional ID-based cleanup. GPU tests verify destruction and ID stability. |
| P1 | RGB/greyscale uploads assumed four-byte row alignment. Odd widths could read beyond the tightly packed input and corrupt rows. PNG readback could overrun a tightly packed output. | Set and restore pack/unpack alignment, use bounded named readback. GPU test compares every byte of a 3×2 RGB upload. |
| P1 | Geometry normals used only rotation; non-uniform scale was ignored. The derivative normal mapper received a normalized model-space direction instead of world position, and degenerate UVs could generate infinities/NaNs. | World positions and inverse-transpose normal transforms; guarded tangent-frame normalization. Pixel tests check scaled normals with normal mapping on/off and degenerate UVs. |
| P2 | Cubemap shadow culling used one identity-view camera for all six faces. Valid casters could be removed from other faces. | Build the culling camera from each face's actual view/projection. |
| P2 | Per-light `castsShadow` and camera `shadowsActive` were ignored; disabling shadows still rendered and sampled them. | Skip shadow rendering and gate shader sampling with the effective shadow flag. A GPU fixture compares lit and occluded pixels with the flag disabled/enabled. |
| P2 | Vertical directional lights passed parallel direction/up vectors to `lookAt`; non-unit directions changed diffuse intensity. | Alternate up vector for vertical lights; normalize direction; skip zero directions. |
| P2 | Cubemap ownership metadata described a 2D depth texture even though the object was an R32F cubemap. | Track texture target and actual format; use target-aware binding/resizing. |
| P2 | Frustum sphere testing returned true after intersecting one plane without checking the remaining planes. Other frustum builders used component-wise multiplication for a cross product, reversed matrix multiplication, and mislabeled planes. | Correct math and test all planes. CPU tests compare both matrix-based frustum builders against direct clip-space inclusion across a grid. |
| P2 | Mirrored scales could shrink bounding radii; nonzero local centers were ignored; repeatedly updating a sphere compounded scale. | Keep local bounds, transform the center, and use maximum absolute scale. CPU test covers reflection, translation and repeated updates. |
| P2 | First-person `lookAt` divided its vertical direction by sqrt(2), so it did not aim at its target. Vertical targets could create singular views. Default/zero rotation quaternions also produced invalid inverse transforms. | Correct pitch extraction, stable vertical basis and valid identity rotations. CPU tests compare the view direction to the requested target. |
| P2 | Camera-specific frustum culling and normal-mapping settings were unused. | Apply the camera's culling flag and send the normal-mapping switch to the geometry shader. |
| P2 | Gameplay cameras stayed at their initial aspect ratio when the viewport changed. | Update controllable camera projections from the viewport in the graphics system. GPU integration test exercises wide and tall viewports. |
| P2 | Fog ignored configured start/end/density, hardcoded camera planes, and reconstructed depth incorrectly. | Reconstruct view-space distance with the camera inverse projection; apply supplied fog settings. Pixel test checks a surface at 10 units with linear fog from 0 to 20. |
| P2 | Near depth-of-field blur did not approach zero at the focus boundary; coincident focus/blur limits could divide by zero. Blur passes also set a nonexistent sampler name and relied on its default value. | Continuous, bounded blur factors and the shader's actual sampler uniform. Combined-effects test checks finite output. |
| P2 | Reattaching color buffers duplicated draw destinations; sparse color attachments mapped to the wrong shader output. Status queries changed framebuffer state, and incomplete buffers were only logged. | Preserve output locations, avoid duplicates, named status queries, and reject incomplete framebuffers before use. GPU tests cover replacement, sparse attachment and separate read/draw bindings. |
| P2 | Mesh reloads retained old optional streams when new data omitted them; malformed stream sizes/indices were accepted. | Validate stream/index bounds before changing state and clear obsolete buffers. |
| P2 | GL-owning wrappers could be copied and double-delete their IDs. Missing resource lookups could dereference `end()` in Release; failed callbacks continued with uninitialized data. | Noncopyable owning wrappers, safe missing-resource returns, and runtime failures for failed resource retrieval. Missing material maps use existing fallback textures. |
| P2 | Rejected texture reinitialization changed metadata belonging to the old valid image. One-pixel-wide/tall textures skipped required mipmaps. Float GPU formats incorrectly assumed float-sized input despite an unsigned-byte API. | Commit metadata after validation/allocation; generate strip mipmaps; validate input bytes independently of GPU precision. GPU tests cover failed reinitialization and 1×8 mip levels. |
| P2 | Post-process initialization could succeed after tone-map setup failed, and checked the wrong screen-quad resource ID. Overlay drawing could dereference a missing active camera. | Correct failure propagation/ID check and null-camera guard. |
| P3 | Legacy `texture2D` calls remained in GLSL 460 core shaders. | Use the core `texture` overloads. |

The bloom/lens-flare framebuffer feedback loops and zero-length flare normalization
were fixed immediately before this broader review. Their stable-HDR/black-input
regression remains part of the suite.

## Follow-up findings resolved

All nine follow-up findings have now been implemented. See
[Rendering completion](rendering-completion.md) for configuration, APIs and limits.

| Finding | Resolution |
| --- | --- |
| Model handles | Resolve current mesh/material IDs and bounds, including live model reloads. |
| Alpha materials | Explicit opaque/mask/blend modes; matching cutout shadows; sorted translucent lighting receiving point and directional shadows. |
| Specular response | Shared linear Blinn-Phong lighting with colored specular accumulation. |
| Color space | sRGB albedo views, linear HDR intermediates, one final sRGB conversion. |
| Sun rays | Project the strongest directional source and use depth occlusion around its disc. |
| Directional shadows/background | Camera-fitted, texel-snapped configurable maps and bias; separate camera-following background. |
| Resource lifecycle | Late listener replay, in-place reload, dependency-safe deletion and scene-handle release. |
| GL state and feedback | Explicit pass policies, caller-state restoration, actual program binding checks and sampler/attachment alias validation. |
| Measurement | Non-blocking GPU timestamp queries and CPU timings per stage/effect, with F2 summary. |

Additional follow-up tests exposed clear-before-resize ordering in geometry/shadow
passes and a shader preprocessor that never assigned its expanded output. Both
are fixed. Includes now reject cycles and parent/absolute paths.

## Validation and limits

CPU regression tests cover OBJ parsing, projection units, frustum inclusion,
reflected bounds, camera aiming and inverse transforms. Real-driver tests use hidden
OpenGL 4.6 core debug contexts and inspect numeric values in their own offscreen
framebuffers: upload bytes, attachment state, resize/storage, finite outputs, stable
glare, normal transforms, shadow flags, fog and combined post-processing. A separate
process intentionally generates a GL error to check synchronous termination.

The game and six deferred demo scenes each run three smoke-test frames. These tests
cover startup, drawing and cleanup, not a complete playthrough or visual equivalence.
No desktop or full-screen screenshots are captured. Validated hardware is the local
NVIDIA RTX 5060 Ti; AMD/Intel and Linux remain untested. Existing asset MTL warnings
and driver performance notices are distinct from test failures.

Final validation: all 14 tests passed in both Debug and Release on this machine. The Release package was refreshed; its startup, three hidden frames and shutdown were also checked from a different working directory inside the project.
