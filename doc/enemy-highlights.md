# Enemy hull and exhaust highlights

Gameplay returns to ambient intensity 0.2 and its original two directional lights. The additional opposing fill light is removed. Enemy identification comes from narrow red emissive inlays, with the existing bloom pass providing their halo.

## How the markings are drawn

The geometry vertex shader passes the original mesh position and normal to the material shader. `data/shader/include/emissive_accents.glsl` constructs two masks in those coordinates:

- A pair of longitudinal strips at positive/negative X, limited to an authored Z interval and upward/downward-facing hull surfaces.
- Engine panels behind an authored Z limit, limited in width and height and facing toward negative Z (the stern; ships face positive Z).

The combined mask colors those surface regions red and writes their intensity into the existing glow channel. Composition adds that emission independently of scene lighting. Derivative filtering softens the edges, and the red paint extends through that filtered edge to avoid a pale fringe from gray hull texels. The original base/normal textures still supply the unmarked hull detail. There are no additional per-ship lights, objects, textures, or draw calls.

Because the mask uses mesh coordinates, ship translation, rotation, scaling and instancing do not move the markings relative to the hull. Both opaque and transparent material paths use the same mask helper. Shadow coverage is unchanged.

## Material settings

The optional `emissive_accents` object in a material JSON contains:

| Setting | Meaning |
|---|---|
| `color` | Linear RGB emission/inlay color, each channel 0–1 |
| `strength` | HDR glow strength, 0–8; zero disables the markings |
| `hull_strip_offset` | Distance of each strip from the mesh's center line |
| `hull_strip_width` | Half width of each strip |
| `hull_z_range` | Start/end coordinates along the hull |
| `engine_z_max` | Forward limit of the engine region |
| `engine_half_width` | Engine region's half width around X=0 |
| `engine_center_y` | Vertical center of the engine region |
| `engine_half_height` | Engine region's half height |

Both enemy materials use strength 2, giving red radiance up to 8 before lighting. Saturated red needs this headroom to cross the existing luminance-based bloom threshold of 1.4. Placement values are in the OBJ mesh's unscaled coordinates. The raider uses `data/material/enemy.json`; the guardian uses `data/material/enemy_guardian.json`. Shared galvanized metal and the player's material remain unmodified. Materials without the optional block retain their existing appearance. Parsing happens during the existing async resource preparation; invalid dimensions and non-finite values are rejected.

## Validation

The `enemy_highlights` GPU regression loads materials asynchronously, then renders both actual ship meshes from above and behind, directly and with translated/rotated instancing. With all scene lighting set to black, it checks that the accents produce red HDR emission, that most of the image remains dark, and that disabling the markings removes emission. All readings come from test-owned render textures; no display capture is used.

The post-processing regression also uses the authored raider emission to verify that the existing bloom produces a red halo beyond the source.
