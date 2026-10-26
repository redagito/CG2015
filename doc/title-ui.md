# Title screen UI

The title and Enter prompt are screen-space text, rather than scene geometry.
`CTitleState` supplies a `UiDocument` to the graphics system when entered and clears
it on exit. The document contains colored rectangles and proportional text with
normalized viewport anchors, uniform scaling and a maximum text width.

The title reads **DEFENDERS of CTHEDRA** above the ship. **Enter** starts the game;
**Escape** retains the existing application exit behavior. Enter uses a press edge,
so holding it through the preceding loading scene does not skip the title. The old
three-second input lock is removed. The action is keyboard driven; the panel does
not currently implement pointer interaction.

`CUiRenderer` draws the UI after scene post-processing and before the debug overlay.
Text does not participate in lighting, shadows, depth of field or lens flare.
The title scene's depth of field is also disabled. The obsolete `titleobj.obj` and
`enter.obj` meshes and their scene entries are removed.

## Self-contained assets

The Release package includes these files under `data/ui/` alongside the executable:

- `font.png` and `font.json`: the glyph coverage atlas and proportional metrics
  used at runtime. The current atlas supports printable ASCII.
- `fonts/Rajdhani-SemiBold.ttf`: the source font from the Google Fonts Rajdhani
  distribution, copyright Indian Type Foundry.
- `fonts/OFL.txt`: the accompanying SIL Open Font License.

The game does not access installed fonts, use a system font fallback, require
Python/Pillow, or download assets at runtime. `scripts/generate_ui_font.py` is an
optional development tool to rebuild the checked-in atlas from the bundled TTF.
The normal CMake install step packages the entire `data/` directory.

## Validation

The `ui` GPU test renders the actual title document into isolated offscreen targets
at 320x180, 1280x720 and 480x900. It checks title/subtitle/prompt coverage, a clear
central ship area, restoration of caller GL state, Enter edge behavior and removal
of the overlay on exit. The full suite now contains 15 tests.

All 15 tests passed in Debug and Release on the local NVIDIA driver. The Release
package was refreshed and passed startup/render/shutdown from another project directory.
