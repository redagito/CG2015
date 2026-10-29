# Session progression and waves

Each defeated enemy awards one skill point, including the guardian. Points and purchased ranks survive retries and remain available after both defeat and victory. Closing the game discards them; there is no save file. One game system owns the session progression, shared by its gameplay and result states.

## Skill tree

The result screen has **24 skills across four tabbed branches**, each with five ranks (120 ranks total). A first rank in the displayed prerequisite unlocks a child. Ranks cost 1, 2, 3, 4 and 5 points; fully buying the tree costs 360 points. Failed, locked and maxed purchases do not consume points.

| Branch | Skills and effect per rank |
|---|---|
| Gunnery | Firepower +20% base damage; rapid fire +20% firing rate; multishot +2 projectiles and +20% total volley damage; critical chance +5 percentage points; critical power +0.1x crit multiplier; energy siphon heals 1% of actual direct hull damage, capped at 1 hull/second per rank |
| Ordnance | High velocity +20% projectile speed; long range +15% lifetime; piercing +1 distinct target per shot; explosive rounds unlock splash (40-80% noncritical pellet damage); blast radius +3 units; chain reaction detonates direct kills for 30 damage per rank |
| Defense | Reinforced hull +10% capacity; armor 4% damage reduction; shield generator +30 shield; shield recharge +5/second after four safe seconds; repair nanites +0.4 hull/second after seven safe seconds; salvage repair +2 hull per direct kill, capped at 2 hull/second per rank |
| Flight | Agility +15% turn rates; responsiveness +25% thrust/deceleration/velocity response; engine power +20% speeds; afterburner +20% boost speed; boost vectoring +0.06 boost turn factor; combat brakes +0.12 brake turn factor |

Choose branches with the tabs or keys 1-4. Click a node to buy one rank. Tab cycles the six visible cards and retry button; Enter activates the selection. Click RETRY RUN or press R for a fresh run. Entry captures held keys/buttons to prevent accidental purchases. Skill names, current ranks, costs, prerequisites and effects remain visible; longer descriptions wrap inside their cards.

Multipliers apply to freshly loaded player tuning once per run. Maximum firepower gives 100 base damage, and maximum rapid fire gives 6.67 volleys/second. Multishot distributes a total volley budget across its projectiles: rank five emits eleven pellets sharing 200 noncritical damage (18.18 each), rather than eleven 100-damage shots. Crit chance reaches 25%, and critical power raises the multiplier from 1.5x to 2x. Each pellet rolls its own crit. Damage and crits reduce hits-to-kill against the campaign's armored and elite targets; a fully upgraded single projectile can still kill a 100-hull raider.

Piercing reaches six distinct receivers. Splash excludes the primary victim, falls to half strength at the edge and reaches 33 units. Its damage is based on noncritical pellet damage, preventing crits from multiplying the area budget. Direct kills can cause a fixed-strength blast (up to 150 damage over 20 units). Splash and chain kills award their normal points but cannot heal the player or trigger another blast generation. Deferred bursts remain bounded per frame.

Armor reduces incoming damage before shields absorb it. Full defensive ranks give 450 hull, 150 shield and 20% damage reduction. Shield recharge reaches 25/second after four seconds without damage; slow hull repair reaches 2/second after seven seconds. Each new hit resets both delays. Crossing a delay within a frame grants only that frame's time beyond the delay.

Lifesteal credits actual hull damage after armor, shields and all pending hits; shield absorption and overkill grant no healing. At maximum rank, it heals 5% of direct hull damage, limited to a 5-hull/second refill rate. Direct-kill salvage repairs 10 hull per kill with a 10-hull/second refill rate. Each source can bank at most one second of recovery, including its initial allowance, so a burst has limited immediate healing and sustained recovery remains bounded. Neither source exceeds maximum health or resurrects a dead player. The HUD displays actual shield capacity once purchased.

An early ten-point build can buy firepower rank 3, rapid fire rank 1, multishot rank 1, high velocity rank 1 and piercing rank 1. Fragile raiders reward crowd coverage; tougher fighters reward concentrated damage and critical hits. Maximum defenses buy recovery time, but concentrated incoming fire remains lethal even with constant direct-hit and kill opportunities. Automated checks verify these mechanics; final encounter difficulty still requires hands-on playtesting.

Retries rebuild health, shields, objects, projectiles, collisions, targets, waves, timers and cinematics from prepared resources while reusing the scene. Points and purchased ranks remain. No runtime font/image/mesh parsing was added.

## Campaign and arena

The flight volume is 1,600 x 1,000 x 1,600 world units (four times wider and deeper). Arena art, the skybox, camera far plane and radar range match the expansion. Early wave groups spawn near the player; larger swarms spread throughout the arena. Spatial spawn checks avoid occupied locations and retry across the flight volume when a local shell fills. `spawn_separation_scale` controls initial clearance (0.65 in the swarm preset), independently from the larger predictive avoidance radius used during movement.

`data/game/defenders_of_cthedra/encounters.json` configures the campaign. Each spawn group supplies `wave` (1-based), type, count, delay and interval. Multiple groups can share a wave number to mix enemy types. Timing restarts when that wave starts. `playerDistance` optionally places a group near the player instead of interpreting offsets relative to arena center. `max_active` still caps concurrent enemies; delayed and queued enemies count toward the current wave and must be defeated before advancing. Numbered waves must be contiguous and nonempty.

| Wave | Raiders | Armored | Elites | Guardian | Total |
|---|---:|---:|---:|---:|---:|
| 1 | 10 | 0 | 0 | 0 | 10 |
| 2 | 18 | 2 | 0 | 0 | 20 |
| 3 | 34 | 6 | 0 | 0 | 40 |
| 4 | 66 | 12 | 2 | 0 | 80 |
| 5 | 132 | 24 | 4 | 0 | 160 |
| 6 | 264 | 48 | 8 | 0 | 320 |
| 7 | 512 | 112 | 16 | 0 | 640 |
| 8 | 960 | 256 | 64 | 0 | 1,280 |
| 9 | 1,856 | 576 | 128 | 0 | 2,560 |
| 10 | 3,583 | 1,280 | 256 | 1 | 5,120 |

Raider hull stays at 100 with no armor. Armored fighters have 350 hull and 20% armor, slower flight and 18-damage shots every 1.25 seconds. Elites have 800 hull and 10% armor, faster turns/flight and 18-damage shots every 0.8 seconds. The guardian has 1,400 hull and 20% armor. All use the shared spawner, AI, weapon and health systems; health does not grow automatically each wave. Enemy armor is an optional per-type `armor` fraction in `encounters.json` (0–0.5). The tougher roles reuse the bundled ship meshes and red emissive materials at larger scales.

The default campaign has 10,230 enemies, doubling at each stage from ten. The active cap is 5,120 rather than 24. Stages 1-4 spawn at 0.02-second intervals per group; later stages use 0.001 seconds. Groups deploy concurrently, so the mixed final swarm deploys in about 3.6 seconds. Clearing a wave starts the next; there is no timed overlap. The HUD shows wave number, current enemies remaining (including pending spawns), total kills and unspent points.

The UFO is hidden and inactive through waves 1-9. Wave 10 reveals it and starts the configured approach (`arena.json`, 60 seconds by default). The UFO then hovers above the temple for five seconds while movement, weapons, enemy AI, spawning and FPS monitoring remain active. Clearing the final enemies during either approach or grace wins and cancels the strike. If enemies remain at the end of grace, combat freezes and the six-second temple-strike cinematic plays before the skill-tree defeat screen. The HUD shows arrival/strike countdowns and a FINAL CHANCE prompt during grace. Total playable final-wave time is 65 seconds.

## Verification

Tests cover exact point rewards, rank costs/prerequisites/caps, actual tuning multipliers, session reset, mouse/keyboard input and hit regions after resizing. GPU tests numerically check node text in dedicated offscreen targets at small, normal and portrait sizes. The cached-loading integration defeats an enemy, buys an upgrade, retries, verifies clean objects and retained stats, clears nine waves to unlock the UFO, exercises the 60-second approach, playable hover, grace-period victory and frozen cinematic, and completes a separate successful ten-wave campaign. No desktop capture is used.

## Swarm limits and performance

The exponential preset remains an intentional stress scenario. Multishot, piercing, splash damage and chain explosions now provide tools for the swarm; clearing later stages is not guaranteed by the automated tests.

During active gameplay, a one-second average below 30 FPS logs the measured rate and exits with a failure code. Loading, menus, result presentation, cinematics and minimized windows reset the window. State-entry initialization is excluded; there is no multi-second gameplay grace period. The monitor uses completed frame wall time including drawing, presentation and event polling.

Opaque ships and projectiles are instanced by mesh/material/winding in geometry and shadow passes. AI flight updates every frame, subdividing long steps at 50 ms; local separation decisions are staggered at 20 Hz and select sixteen nearby threats from a bounded sample. Spawn placement uses an incremental spatial index. Audio voices and cosmetic particles remain bounded; saturated effects use bounded replacement work rather than scanning the entire pool for every spark. These limits preserve the enemy population and actual firing simulation.

See [swarm performance](swarm-performance.md) for the repeatable rendered benchmark and measured results. The benchmark is separate from functional CTest checks because frame timing depends on the machine, resolution and background load.

## Boundary warning

The horizontal warning margin remains 45 units. Altitude has its own `verticalWarningMargin` in `arena.json`, defaulting to 12 units instead of using 45 for every axis. Normal flight at Y=50 now receives no warning; approaching the floor around Y=20 warns to climb. The HUD distinguishes LOW ALTITUDE, CEILING and ARENA EDGE. Physical return assistance and spawn-safe positions use the same per-axis margins.


Landmark scale and ship visibility
--------------------------------
The floor remains at y=10 for flight assistance; the ceiling is now y=1,010. Launch altitude remains y=70 and the ship starts 230 units south of the temple. `templeScale` and the mothership `scale`, `start`, and `destination` fields in `arena.json` control landmark size and the approach route. The temple is approximately 335 units wide and 123 high; the four obelisks are approximately 455 high; the mothership is approximately 785 wide, approaching at y=750. Grounded landmark meshes rest at y=0. Cinematic framing, beam thickness, destruction effects and UFO audio attenuation account for the larger scale. The approach lasts 60 seconds, followed by five seconds of playable grace while the UFO hovers.

Gameplay ambient intensity is restored to 0.2 with the original two directional lights. Enemy hull strips and rear engine panels emit red light through optional material masks, preserving visibility in the darker arena. See [enemy highlights](enemy-highlights.md) for placement and material settings. The arena lighting remains in `data/world/game_1.json`.
