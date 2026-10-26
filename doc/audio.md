# Audio

The game uses miniaudio behind `IAudioSystem`. `GameAudio` maps game events to cues; gameplay controllers only report events or move emitters. The player is the listener, with camera orientation. Music, effects, engines and UI have separate volume buses configured in `data/audio/cues.json`.

The loading worker parses the sound bank and decodes effects before publishing resources. Music keeps compressed FLAC in memory and primes a two-second PCM ring before the loading screen becomes ready. A producer thread replenishes that ring. Playback only reads prepared samples: it does no file access or decoding. Missing audio devices select a silent backend; hidden runs are silent by default.

Title and combat music crossfade. Shots, damage and destruction select variations. The player engine follows movement; the approaching UFO has a positional engine loop whose inverse-distance attenuation makes it louder as it approaches. Emitters stop with their objects and scenes. Voice limits, priorities and shared asset ownership bound playback and keep resource removal safe.

All runtime assets are bundled in `data/audio`; no downloads, Python or system fonts are needed. Keep the packaged executable beside its data directory. `sources.json` records origins, processing and checksums. The preparation script is a development tool, not part of startup.

Audio tests use the offline mixer, covering async preparation, primed music, attenuation, panning, voice limits, mute, damage events and emitter destruction. The loading-screen test also enters the real title and gameplay states with offline sound. `cg2015_audio_tests --device-check` opens and closes the default playback device without playing sound.

Code layout: each class and each non-private-nested struct has its own header; implementations live in matching cpp files. Private nested structs may stay with their owning class.

The victory and defeat scenes use the bundled UI font for their result overlays. Their legacy text meshes were removed; lens flare, god rays and depth of field are disabled on these screens. The overlay is cleared on exit.
