#!/bin/sh
# SPDX-License-Identifier: MIT
# Run the device package in fm1-emu's command line: boot, place a chord and two notes, play, stop, save.
#   FM1_EMU=path/to/fm1-emu tests/emu_smoke.sh [build/stamp.fwsc]   -> build/emu-smoke/{work/lcd_*.bmp,run.wav,run.log}
set -eu
PKG="$(cd "$(dirname "${1:-build/stamp.fwsc}")" && pwd)/$(basename "${1:-build/stamp.fwsc}")"
OUT=build/emu-smoke; mkdir -p "$OUT/work"; cd "$OUT"
"${FM1_EMU:?set FM1_EMU to an fm1-emu binary (cargo build --release -p fm1-emu)}" --fwsc "$PKG" --ms 7500 \
  --keys '1500:n7,1700:n11,1900:n14,2200:enc0:+2,2500:n9,2800:enc0:+2,3000:n12,3600:PLAY,6000:PLAY,6500:SAVE' \
  --snap 2500,3300,4500,6300,7400 --wav run.wav > run.log
grep -E '^time|lcd frames' run.log
