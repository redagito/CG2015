# Combat and arena

The second and third flight milestones share the same weapon simulation for player and enemies. Data is bundled in `data/game/defenders_of_cthedra/combat.json` and `arena.json`, parsed by the existing async loader before gameplay, then read from cache. No files are opened when firing, selecting a target or updating an emitter.

## Firing and targeting

Left mouse fires alternating shots from the two model-space gun sockets. The offsets were placed using the front gun geometry of `ship_2.obj`, with clearance for the projectile's rear cap. Translation, full quaternion attitude (including roll) and model scale transform each socket. A projectile starts with its actual render/collision pose, and its velocity is:

`ship velocity + ship forward × projectile speed`

The bundled projectile's long axis is Y; its render attitude aligns that axis with its world flight path. Shots live for a configured duration and damage only the first receiver along a swept path. Actor movement runs before projectile sweeps, and receiver motion is included, so a target crossing the path between frames is not missed because of spawn order. AABBs include the original mesh center, scale and rotation. Collision remains a conservative bounding-box approximation, not per-triangle physics.

World object IDs are monotonic and separate from renderer slots. Target snapshots copy names, positions and velocities, so selecting an enemy does not retain a pointer to it. Destroyed/deleted/out-of-range targets clear the selection. Expired shots release their colliders and scene proxies; render slots are reused.

| Control / marker | Meaning |
|---|---|
| Tab | Cycle nearby hostiles in stable spawn order, including ones behind you |
| T | Select the hostile closest to the ship's forward direction, within the acquisition cone |
| X | Clear selection; it stays clear until the next selection action |
| Red brackets | Selected target; clamped toward the edge when off screen |
| Green hollow square | Aim/lead point for the next muzzle |
| White cross | Current muzzle's forward gun line |
| Amber dot | Ship velocity direction |

The first suitable forward target is acquired automatically. The lead solver uses relative target/shooter velocity and the same projectile speed, muzzle and lifetime as firing. It solves the quadratic interception equation, including its linear-degenerate case, and rejects unreachable or expired solutions. The compensated aim point differs from the future impact position when the shooter has velocity. Lead predicts constant target velocity; enemies can change direction after a shot. There is no auto-aim or homing.

`combat.json` configures relative projectile speed, lifetime, damage per shot, interval, muzzle offsets, radar range and acquisition angle. Enemy movement defaults are in `arena.json`; per-type health, weapons and behavior are in `encounters.json`. Space remains brake and does not fire.

## Engine sound

The player engine uses throttle, brake and boost state rather than position differences. Gain and pitch rise from idle to full throttle, with an additional boost stage; braking spools them down. Exponential smoothing prevents abrupt changes. Teleportation or boundary corrections cannot create a false engine-load spike. The same miniaudio voice remains attached to the ship and is stopped with its owner/state. Positional enemy loops and the approaching UFO emitter remain independent.

The engine section of `combat.json` contains idle/full/boost gain and pitch multipliers plus response rate. They modulate the base cue and engine bus from `data/audio/cues.json`. Pitch is applied on the game thread; the callback continues to consume prepared samples only.

## Arena and opponents

The default flight volume is 400 × 180 × 400 world units, centered at `(0, 100, 0)`. Ground arena art is enlarged to match it. A projected holographic outline shows the full volume, including its upper boundary. Near a face, the outline changes color and a warning appears. Assistance increasingly turns the ship inward and damps outward velocity, while preserving manual roll. This is a soft boundary: there are no portals, position snaps or invisible hard clamping. The old wrapping controller has been removed.

`arena.json` configures the volume, warning margin, return steering/damping, enemy movement and combat tuning. The shipped ground art matches the default volume; changing its size substantially also calls for updating the scene art.

Raiders and the guardian pursue the player's motion in 3D, line up attack runs, fire only when a predicted shot is near their gun line, and break away before close passes. They extend and come around for another run rather than following fixed routes. The same boundary assistance brings them back into the volume. Their turn rate, cruise/break speeds, firing cone, reaction behavior, attack distance, break duration, separation distance, prediction horizon and approach spread are deliberately simple arcade tuning. There is no obstacle pathfinding; the arena's normal flight volume is above the ground structures.

The player now has health, shown on the HUD, and enters defeat when destroyed. Defeating the configured encounter wins; completion follows the roster count, including queued waves. The default remains six raiders and one guardian. The guardian's decorative ring follows its stable object ID and disappears when the guardian is removed. The approaching UFO provides the timed defeat condition, ending in a temple-strike cinematic.

## Verification

The suite includes mathematical interception checks against actual projectile trajectories, rolled/scaled muzzle placement, alternating guns, initial render pose, inherited motion, cadence at 30/60/144 Hz, stationary and moving swept receivers, lifetime expiry, scene-slot reclamation, target cycling/deletion/clearing, and engine throttle/boost/brake routing. Offline audio verifies that pitch changes the rendered frequency. GPU checks cover target/lead visibility at small, wide and portrait sizes. Arena tests exercise all six return directions under boost, pursuit/attack/break-away states, owner lifetime and victory counting. The loading integration deploys pursuit enemies, fires from cached resources, and enters defeat on player destruction.

## Enemy roster and spacing

`encounters.json` defines an arbitrary list of `enemy_types` and `waves`. Each type supplies an ID, display name, mesh, material, health, scale and engine cue. Optional `behavior`, `weapon` and `decoration` entries override shared defaults. A wave names its type and supplies count, delay and interval; optional offsets are fractions of arena half-extents. New types do not require C++ branches. All referenced assets join async loading before gameplay.

`max_active` limits concurrent hostiles; remaining spawns wait for room. Spawn positions spread around the arena, avoid occupied locations and keep clear of the player. Pursuit uses separate approach lanes and predicted local separation, including deterministic escape directions for coincident ships. A spatial grid supplies nearby ships without every enemy scanning the entire roster. Enemy count and the audio voice budget are separate limits.

Roster tests cover a third type, 137 configured enemies, invalid data and 1,000 spatial neighbors. The gameplay integration spawns and defeats 37 enemies with a four-ship concurrency limit, using cached assets throughout.

## Mothership deadline

`arena.json` contains `mothership.approachSeconds` (30 by default) and `mothership.waitSeconds` (5). Movement is derived from the approach duration, so changing timing preserves the same start and destination. The controller's clock drives both the HUD and defeat; crossing arrival within a frame consumes the remaining portion of that frame from the wait. The HUD shows arrival and total time left, then switches to MOTHERSHIP ARRIVED for the final wait. Countdown values round up and turn red in the last ten seconds. The deadline freezes when a victory or player-destruction sequence starts.

## Mothership strike cinematic

After the configured approach and wait (30 + 5 seconds by default), combat freezes and the six-second temple strike begins: 1.2 seconds charging, 2.3 seconds firing, then 2.5 seconds of destruction and fade. A dedicated orbiting camera frames the UFO and temple, including portrait windows. The beam connects mesh-derived underside/roof positions; its core and halo render in HDR before bloom, with strike lighting, sparks, debris and smoke. The temple disappears in the destruction burst, then the normal defeat screen appears.

World simulation stops at the deadline, including the remainder of the triggering step. Spawning, AI, weapons, collision damage and movement stop; existing projectile visuals are hidden. Flight input is released and combat music/voices/events are stopped before cinematic sounds start. The cinematic updates only its camera, particles and lighting. Ordinary player-death defeat and victory still use their existing result paths.

Cinematic sounds are original bundled CC0 assets, generated by `scripts/prepare_mothership_sounds.py` and decoded through the async audio bank before gameplay. Tests cover the timeline, same-step freeze, beam endpoints, camera framing, fade and cleanup. The loading integration renders the sequence with cached resources and verifies ships remain stationary despite held fire input.
