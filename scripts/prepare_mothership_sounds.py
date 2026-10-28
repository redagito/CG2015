"""Generate original CC0 cinematic sounds using only the Python standard library."""
from pathlib import Path
import hashlib
import json
import math
import random
import struct
import wave


def prepare():
    root = Path(__file__).resolve().parents[1]
    manifest = root / "data/audio/sources.json"
    sources = json.loads(manifest.read_text())
    rate = 48000
    for name, duration in [("mothership_charge", 1.2), ("mothership_beam", 2.3)]:
        rng = random.Random(8421)
        samples = []
        phase = filtered = 0
        for i in range(int(rate * duration)):
            t = i / rate
            progress = t / duration
            filtered = .85 * filtered + .15 * rng.uniform(-1, 1)
            if name.endswith("charge"):
                phase += 2 * math.pi * (65 + 600 * progress * progress) / rate
                value = (math.sin(phase + 1.8 * math.sin(phase * .5)) * .4 + filtered * .3) * progress
            else:
                phase += 2 * math.pi * (72 + math.sin(t * 17) * 5) / rate
                value = math.sin(phase + 2.5 * math.sin(phase * 3)) * .35 + filtered * .8
            envelope = min(1, t / .015, (duration - t) / .06)
            samples.append(int(max(-1, min(1, value * envelope)) * 28000))
        path = root / "data/audio/sfx" / (name + ".wav")
        with wave.open(str(path), "wb") as output:
            output.setnchannels(1)
            output.setsampwidth(2)
            output.setframerate(rate)
            output.writeframes(struct.pack("<" + "h" * len(samples), *samples))
        sources["sfx/" + path.name] = {
            "author": "CG2015 project", "license": "CC0-1.0",
            "source": "scripts/prepare_mothership_sounds.py",
            "processing": "Original deterministic oscillator/noise synthesis, 48 kHz mono PCM",
            "sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
        }
    manifest.write_text(json.dumps(sources, indent=2) + "\n")


if __name__ == "__main__":
    prepare()
