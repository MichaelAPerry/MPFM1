#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Render the front page's audio previews with the host simulator (build/host/stamp_host, ./build.sh --host):
one C4-E4-G4 arpeggio per sound (site/audio/NN.mp3) and the first 20 s of the Twinkle demo (twinkle.mp3).
These are renderings of the firmware's own engine and sound pack, not recordings of a unit.
  tools/make_site_audio.py [OUT_DIR]      (default site/audio; needs ffmpeg with libmp3lame)"""
import re, subprocess, sys, tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "site" / "audio"
HOST = ROOT / "build" / "host" / "stamp_host"
N = 31
KEYS = (7, 11, 14)            # C4, E4, G4 (key 0 = F3)
STEP, HOLD, TAIL = 330, 280, 650


def script():
    s = ["wait 300"]
    for i in range(N):
        s.append(f"wav s{i:02d}.wav")
        for k in KEYS:
            s += [f"key {k} down 100", f"wait {HOLD}", f"key {k} up", f"wait {STEP - HOLD}"]
        s += [f"wait {TAIL}", "wavstop"]
        if i < N - 1:
            s += ["turn PRESET 1", "wait 50"]
    s += ["demo", "wait 100", "wav twinkle.wav", "tap PLAY", "wait 20000", "tap PLAY", "wavstop"]
    return "\n".join(s) + "\n"


def gain(wav, seconds):
    """dB that bring the clip's peak to -5 dBFS (the engine renders near -12 dBFS; a preview should be audible)"""
    r = subprocess.run(["ffmpeg", "-v", "info", "-t", str(seconds), "-i", str(wav), "-af", "volumedetect", "-f", "null", "-"],
                       capture_output=True, text=True)
    return -5.0 - float(re.search(r"max_volume: (-?[\d.]+) dB", r.stderr).group(1))


def main():
    if not HOST.exists():
        sys.exit(f"{HOST}: run ./build.sh --host first")
    OUT.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory() as d:
        d = Path(d)
        (d / "audio.txt").write_text(script())
        subprocess.run([str(HOST), str(d / "audio.txt"), str(d)], check=True, stdout=subprocess.DEVNULL)
        enc = ["ffmpeg", "-y", "-v", "error", "-ac", "1", "-ar", "32000", "-c:a", "libmp3lame", "-b:a", "48k"]
        for i in range(N):
            subprocess.run(["ffmpeg", "-y", "-v", "error", "-i", str(d / f"s{i:02d}.wav"), "-af",
                            f"silenceremove=start_periods=1:start_threshold=-60dB,volume={gain(d / f's{i:02d}.wav', 1.8):.1f}dB,afade=t=out:st=1.5:d=0.3",
                            "-t", "1.8", *enc[4:], str(OUT / f"{i:02d}.mp3")], check=True)
        subprocess.run(["ffmpeg", "-y", "-v", "error", "-i", str(d / "twinkle.wav"), "-af",
                        f"volume={gain(d / 'twinkle.wav', 20):.1f}dB,afade=t=out:st=18.5:d=1.5", "-t", "20", *enc[4:], "-b:a", "40k", str(OUT / "twinkle.mp3")],
                       check=True)
    tot = sum(f.stat().st_size for f in OUT.glob("*.mp3"))
    print(f"{OUT}: {N} clips + twinkle.mp3, {tot} bytes")


main()
