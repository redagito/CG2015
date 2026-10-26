# Asynchronous resource preparation

The application prepares resource files on CPU worker threads. PNG decoding,
OBJ parsing, material/model/scene JSON, shader files and includes are handled in
private staging resource managers. Workers never access the live scene, live
resource maps, graphics callbacks or an OpenGL context.

## Startup and the sphere loading screen

1. After creating the window, the engine queues `data/bootstrap-assets.json`.
   It contains renderer/UI shader sources, the font and its metadata, the light
   volume/screen quad, and the sphere/rings loading scene with its dependencies.
   While this small bootstrap is prepared, the window pumps events and shows black.
2. The render thread attaches the graphics manager and initializes the renderer
   from these cached CPU resources. OpenGL objects and shader compilation remain
   on the context-owning thread.
3. Only the loading game state is initialized initially. On entry it queues
   `data/game/defenders_of_cthedra/assets.json`. This manifest lists title,
   gameplay, win/lose scenes and the assets used by programmatic gameplay objects.
   Scene requests discover their mesh/material/model dependencies; materials and
   models recursively prepare their images and other dependent resources.
4. The sphere/rings animate while the worker prepares content. The overlay reports
   preparation and upload counts. Completed CPU batches are published by the main
   thread with a nominal 2 ms budget per frame. A resource's dependencies are
   published before its callback runs. Readiness includes these GPU callbacks.
5. When the batch is ready, the sphere screen shows **PRESS ENTER TO CONTINUE**.
   It remains visible and animated until a fresh Enter press. Presses during loading
   and a held Enter key cannot dismiss the ready prompt or skip the title. States
   assemble their scenes on first entry using prepared resources. Entering gameplay
   therefore performs no file reads, image decoding or OBJ parsing.

Demo scene resources are also prepared on a worker before their scene is assembled.
Startup configuration itself is read before resource loading begins.

## API and ownership

`IResourceManager` exposes:

- `loadAsync(requests)` for mesh, image, material, model, shader, text, JSON or scene
  requests; image requests specify RGB, RGBA or greyscale decoding.
- `loadManifestAsync(path)` to read an asset list on the worker.
- `getLoadStatus(id)` for preparation/publication counts, phase and error details.
- `pumpAsyncLoads(budgetMilliseconds)` to publish prepared resources on the owner
  thread. Normal `loadMesh`, `loadMaterial`, etc. then return the cached handles.
- `cancelLoad(id)` for cooperative cancellation.
- `loadJsonDocument(path, output)` for cached parsed documents.

Each batch uses an isolated CPU resource namespace. Publication remaps IDs and
deduplicates against the live file caches, including image decoding formats and
shader preprocessing modes. Existing GPU resource identities are retained on cache
hits. At most two batches prepare concurrently; a batch resolves its dependencies
sequentially within its worker. Handles and status queries belong to the owner
thread; attempting to use a live manager from another thread fails explicitly.

The application enables cache-only mode after bootstrap. Unexpected resource
misses and synchronous reload calls fail instead of silently doing blocking work
during rendering. Legacy synchronous load/reload APIs remain available for
standalone tools and existing tests that do not enable this mode. Add newly used
assets to the appropriate manifest or queue them explicitly before using them.

Worker logging uses spdlog's thread-safe sinks. The debug overlay reads a bounded
ring-buffer snapshot on the main thread; no worker calls UI code. See `logging.md`.

## Failure, cancellation and practical limits

A failed CPU batch publishes nothing. Publication errors keep the loading screen
active with an error message; they do not enter gameplay. A failed individual GPU
publication is removed from the CPU cache and its listener objects are cleaned up,
so a retry cannot falsely succeed from a poisoned cache entry. Earlier successful
publications remain cached. Cancellation prevents further publication. Shutdown
requests cancellation and joins workers before destroying their staging storage.

Cancellation is checked between requests and dependency preparations; it does not
interrupt an OBJ parser or PNG decoder halfway through a file. Shutdown can wait
for that in-flight operation. GPU uploads and shader compilation are indivisible
main-thread work and can exceed the nominal frame budget for one large resource.
There are no background OpenGL calls or promises of completely stall-free uploads.
CPU preparation completes per batch before its publication begins; the current
implementation is not a continuously streamed, byte-bounded asset pipeline.

Gameplay lens flare is disabled on the gameplay camera, independently of the
title/loading settings and demo effects.

## Validation

The CPU async test covers deferred owner-thread callbacks, dependency remapping,
material settings, publication after source files are removed, upload budgets,
deduplication, cache-only misses, cross-thread misuse, missing files, cancellation,
failed-upload retry and destruction with work outstanding.

The real-driver loading test checks the ready prompt, held-key rejection and explicit continuation, and renders the animated loading scene while preparing
assets, reaches the title, activates Enter, and runs gameplay with cache-only mode
enabled. The earlier Debug run rendered 207 loading frames. The complete test suite
contains 17 tests, including the existing numerical rendering and UI regressions.

Final validation: all 17 tests passed in Debug and Release. The refreshed Release
package completed 1,000 hidden frames from another project directory, progressed
from the sphere loading scene to the title using only packaged assets, and exited
with code 0.
