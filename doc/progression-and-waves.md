# Session progression and waves

Each defeated enemy awards one skill point, including the guardian. Points and purchased ranks survive retries and remain available after both defeat and victory. Closing the game discards them; there is no save file. One game system owns the session progression, shared by its gameplay and result states.

## Skill tree

The result screen shows three branches with two nodes each. A first rank in the upper node unlocks the lower node. Every node has five ranks. The next rank costs 1, 2, 3, 4 and 5 points respectively; locked, unaffordable and maxed nodes cannot consume points.

| Branch | Upper node | Lower node |
|---|---|---|
| Weapons | Firepower: +20% damage per rank | Rapid fire: +15% shots per second per rank |
| Handling | Agility: +10% pitch/yaw/roll speed per rank | Responsiveness: +15% acceleration, deceleration and velocity response per rank |
| Engines | Engine power: +10% flight speeds per rank | Afterburner: +12% boost speed per rank |

Multipliers apply to freshly loaded player tuning once per run. Enemy weapons retain their base tuning. Rapid fire divides the shot interval by its multiplier. Engine power also scales the baseline boost speed, then afterburner applies its own boost multiplier. Skills do not change the authored weapon sockets or projectile collision rules.

Click a node to buy one rank. Tab cycles through nodes and the retry button; Enter activates the selected item. Click RETRY RUN or press R for a new run. Held buttons/keys on entering the menu do not purchase or retry automatically. Buying is allowed only on the result screen.

Retries rebuild player health, objects, projectiles, collisions, targets, waves, timers and cinematic state from prepared resources, reusing the existing scene rather than allocating an extra scene each time. Session points/ranks remain untouched. No files are decoded during retries.

## Campaign and arena

The flight volume is 1,600 x 180 x 1,600 world units (four times wider and deeper). Arena art, the skybox, camera far plane and radar range match the expansion. Wave groups spawn near the player's current position, with boundary and separation checks, so travel across the larger volume is optional.

`data/game/defenders_of_cthedra/encounters.json` configures the campaign. Each spawn group supplies `wave` (1-based), type, count, delay and interval. Multiple groups can share a wave number to mix enemy types. Timing restarts when that wave starts. `playerDistance` optionally places a group near the player instead of interpreting offsets relative to arena center. `max_active` still caps concurrent enemies; delayed and queued enemies count toward the current wave and must be defeated before advancing. Numbered waves must be contiguous and nonempty.

| Wave | Enemies |
|---|---|
| 1-9 | 3, 4, 5, 6, 7, 8, 9, 10, then 12 raiders |
| 10 | 1 guardian and 4 raiders |

The default campaign has 69 enemies. Clearing a wave starts the next; there is no timed overlap. The HUD shows wave number, current enemies remaining (including pending spawns), total kills and unspent points.

The UFO is hidden and inactive through waves 1-9. Wave 10 reveals it and starts the configured approach (`arena.json`, 30 seconds by default). Clear the final enemies before arrival to win. Otherwise combat freezes at arrival, and the configured five-second wait runs before the six-second temple-strike cinematic and skill-tree defeat screen. The wait provides no extra combat time. The HUD displays arrival and strike countdowns only during the final wave, with a TIME EXPIRED warning during the wait.

## Verification

Tests cover exact point rewards, rank costs/prerequisites/caps, actual tuning multipliers, session reset, mouse/keyboard input and hit regions after resizing. GPU tests numerically check node text in dedicated offscreen targets at small, normal and portrait sizes. The cached-loading integration defeats an enemy, buys an upgrade, retries, verifies clean objects and retained stats, clears nine waves to unlock the UFO, exercises the frozen deadline/cinematic, and completes a separate successful ten-wave campaign. No desktop capture is used.
