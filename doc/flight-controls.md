# Flight controls — first milestone

Mouse movement displaces a virtual stick around screen center. The cyan bracket shows its position; move it back toward the center tick to stop steering, or hold R to recenter. A small dead zone makes neutral aiming steady. The mouse is captured only during gameplay, released on focus loss or state exit, and starts neutral on reacquisition. Hidden test windows do not capture it.

| Input | Action |
|---|---|
| Mouse | Pitch/yaw, stronger toward the edge of the steering circle |
| W / S | Increase/decrease retained throttle |
| A / D | Roll left/right |
| Left Shift | Boost; steering is gentler at boost speed |
| Space | Brake; tighter turns, overrides boost |
| Left mouse | Fire |
| R | Recenter steering |
| Escape | Exit |

The white cross marks the ship's forward gun line at 100 world units. The amber dot marks its velocity direction at that distance, making drift visible. Neither is a target lead indicator. Cyan brackets show mouse steering independently of camera lag. The HUD also shows speed, throttle and brake/boost status.

`CFlightController` owns quaternion attitude, angular response, throttle and assisted velocity. Automatic bank is cosmetic: it does not inadvertently tilt the steering axes. Manual roll changes the physical attitude. Small integration steps keep turns consistent across frame rates; an exceptional frame contributes at most 0.25 seconds. There is no player altitude clamp.

`CChaseCamera` follows after world movement, with critically damped position and look-target springs. It follows deliberate rolls, anticipates steering and gently widens FOV under boost. Teleports/wraps reset the springs instead of dragging the view across the arena. Projection keeps the 0.5 near plane that fixes distant shadow acne and uses the same aspect/FOV as the reticles. Gameplay depth-of-field is disabled for aiming clarity.

## Tuning

Edit `data/game/defenders_of_cthedra/flight.json` and restart. The file is bundled and prepared asynchronously with the gameplay manifest; startup uses cached JSON. Values are validated and invalid/missing settings report an error.

| Settings | Meaning |
|---|---|
| minSpeed, maxSpeed, initialSpeed, boostSpeed, brakeSpeed | World units per second; cruising range and overrides |
| acceleration, deceleration | Speed change per second |
| velocityResponse | How quickly velocity follows the ship's forward direction |
| throttleRate | Throttle fraction changed per second while holding W/S |
| pitchRate, yawRate, rollRate | Maximum degrees per second |
| angularResponse | How quickly turn rates respond |
| autoBank, bankResponse | Visual bank angle in degrees and response |
| brakeTurnMultiplier, boostTurnMultiplier | Steering changes under brake/boost |
| pointerRadius | Full-turn mouse displacement as a fraction of the shorter window dimension |
| deadZone, steeringExponent, invertY | Fractional center dead zone, response curve and inverted pitch option |
| cameraDistance, cameraHeight, lookAhead, anticipation | Chase framing in world units |
| cameraFrequency | Critically damped spring frequency in Hz |
| cameraRotationResponse | Camera up-axis smoothing rate |
| resetDistance | Ship displacement that triggers a camera reset |
| fieldOfView, boostFieldOfView, fieldOfViewResponse | Vertical FOV in degrees, additional boost FOV and response rate |

Rates other than angular speeds/frequency are exponential response coefficients in inverse seconds. Larger response values reduce lag. Keep the speed ordering brake <= minimum <= initial <= maximum <= boost.

## Scope and verification

This milestone replaces the old movement and chase-camera controllers and adds a quaternion bridge to game-object rendering. It keeps the existing enemies, projectile speeds, collision behavior and X/Z arena wrapping. Boundary assistance, target selection, lead prediction and combat/AI balancing remain later milestones. The old wrap can still be encountered by the player; the camera now resets cleanly across it.

The flight regression tests cover neutral flight, steering signs, dead zone, inverted pitch, loops/rolls, throttle/boost/brake, momentum, different frame rates, roll/vertical camera stability, wrap reset, boost FOV and reticle positioning at different aspect ratios. The loading integration enters gameplay with cached tuning, steering input and firing. Existing GPU tests cover the render pipeline and far-wall shadow stability.

The initial boost speed is kept below the existing projectile speed so the ship cannot overtake its own shots. Inherited projectile velocity belongs to the combat milestone.
