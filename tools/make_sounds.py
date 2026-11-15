#!/usr/bin/env python3
"""Synthesize Omtris' line-clear sounds into src/sounds/.

Run from the project root: python3 tools/make_sounds.py
"""

import math
import random
import struct
import wave
from pathlib import Path

RATE = 44100
OUT = Path(__file__).resolve().parent.parent / "src" / "sounds"


def tone(buf, start, freq, duration, amp, decay=0.15, attack=0.004, harmonic=0.0):
    first = int(start * RATE)
    count = int(duration * RATE)
    for i in range(count):
        t = i / RATE
        if t < attack:
            env = t / attack
        else:
            env = math.exp(-(t - attack) / decay)
        value = math.sin(2 * math.pi * freq * t)
        if harmonic:
            value += harmonic * math.sin(4 * math.pi * freq * t)
        index = first + i
        if index < len(buf):
            buf[index] += amp * env * value


def crack(buf, start, duration, amp, decay=0.04):
    first = int(start * RATE)
    count = int(duration * RATE)
    for i in range(count):
        t = i / RATE
        index = first + i
        if index < len(buf):
            buf[index] += amp * math.exp(-t / decay) * (random.random() * 2 - 1)


def write(name, buf, peak=0.85):
    top = max(1e-9, max(abs(v) for v in buf))
    scale = peak / top
    path = OUT / name
    with wave.open(str(path), "wb") as out:
        out.setnchannels(1)
        out.setsampwidth(2)
        out.setframerate(RATE)
        out.writeframes(b"".join(struct.pack("<h", int(v * scale * 32767)) for v in buf))
    print(f"wrote {path} ({len(buf) / RATE:.2f}s)")


def clear_sound():
    buf = [0.0] * int(0.55 * RATE)
    crack(buf, 0.0, 0.10, 0.22, decay=0.030)
    tone(buf, 0.0, 110.0, 0.12, 0.28, decay=0.045)
    for i, freq in enumerate((1046.50, 1318.51, 1567.98, 2093.00)):
        tone(buf, 0.025 + i * 0.055, freq, 0.32, 0.20, decay=0.11, harmonic=0.18)
    return buf


def tetris_sound():
    buf = [0.0] * int(0.90 * RATE)
    crack(buf, 0.0, 0.16, 0.28, decay=0.045)
    tone(buf, 0.0, 72.0, 0.22, 0.38, decay=0.070)
    for i, freq in enumerate((523.25, 659.25, 783.99, 1046.50, 1318.51, 1567.98)):
        tone(buf, 0.035 + i * 0.060, freq, 0.40, 0.19, decay=0.13, harmonic=0.22)
    for freq in (1046.50, 1318.51, 1567.98):
        tone(buf, 0.43, freq, 0.45, 0.11, decay=0.24, harmonic=0.15)
    return buf


if __name__ == "__main__":
    OUT.mkdir(parents=True, exist_ok=True)
    write("clear.wav", clear_sound())
    write("tetris.wav", tetris_sound())