# Combat feedback

Shots have a brief muzzle flash, short colored trails and sparks at the swept collision contact. A successful player shot also flashes the reticle. Destroyed ships leave an expanding flash, shockwave, sparks, debris and smoke that continue after their game object is removed. Player and enemy projectiles use distinct colors, configurable through their weapon entries.

Player damage triggers a cyan shield rim around the ship, a fading red HUD flash and directional edge indicator, a dedicated synthesized shield-hit sound, and a brief capped camera kick. The shield is visual feedback; damage still reduces hull health. The camera settles back to its normal framing. Lethal damage plays the explosion sound and destruction burst. Results wait briefly so the last explosion is visible.

`data/game/defenders_of_cthedra/effects.json` configures the particle budget, trail spacing/lifetime, shield duration/radius, camera kick and result delay. Set `cameraKick` to zero to remove the impact camera motion. `data/audio/cues.json` configures `player_hit` gain and voice limits. The bundled original CC0 sound is reproducible with `scripts/prepare_shield_sound.py` and is decoded through the existing async asset loader.

The particle pass draws depth-tested billboards into the HDR scene before bloom. Opaque geometry occludes particles; particles do not write depth. Smoke uses sorted alpha blending; flashes and sparks use additive blending. Shapes are procedural shader assets, with no runtime image downloads or generation. A bounded particle pool prioritizes impacts/explosions over trails, and scene changes clear all feedback.

CPU tests cover player damage-to-sound routing, shield/direction, camera recovery, persistent destruction and particle budget/expiry. GPU tests render into their own offscreen target and numerically verify glow, wall occlusion, state restoration and shield rim visibility. No desktop capture is used.
