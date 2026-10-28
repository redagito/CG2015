"""Regenerate the bundled original shield hit; uses only Python standard libraries."""
from pathlib import Path
import hashlib
import json
import math
import random
import struct
import wave


def prepare():
    root = Path(__file__).resolve().parents[1]
    rate = 48000
    rng = random.Random(7213)
    samples = []
    phase = filtered = 0
    for i in range(int(rate * .28)):
        t = i / rate
        envelope = min(1, t / .003) * math.exp(-t * 15)
        frequency = 130 + 1100 * math.exp(-t * 20)
        phase += 2 * math.pi * frequency / rate
        filtered = .75 * filtered + .25 * rng.uniform(-1, 1)
        value = (math.sin(phase + 1.4 * math.sin(phase * 1.8)) * .45 + filtered * .7) * envelope
        samples.append(int(max(-1, min(1, value)) * .8 * 32767))
    path = root / "data/audio/sfx/shield_hit.wav"
    with wave.open(str(path), "wb") as output:
        output.setnchannels(1)
        output.setsampwidth(2)
        output.setframerate(rate)
        output.writeframes(struct.pack("<" + "h" * len(samples), *samples))
    manifest = root / "data/audio/sources.json"
    sources = json.loads(manifest.read_text())
    sources["sfx/shield_hit.wav"] = {
        "author": "CG2015 project", "license": "CC0-1.0",
        "source": "scripts/prepare_shield_sound.py",
        "processing": "48 kHz mono PCM, deterministic FM/noise synthesis, attack and exponential decay",
        "sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
    }
    manifest.write_text(json.dumps(sources, indent=2) + "\n")


if __name__ == "__main__":
    prepare()
