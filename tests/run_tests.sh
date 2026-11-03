#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p build
python3 tools/import_smp.py examples/playground.smp build/playground.scf
python3 tools/import_smp.py tests/flats.smp build/flats.scf
python3 -m unittest discover -s tests -p 'test_*.py'
# core tests against both packs: the placeholder tones (pinned render hash, looped sounds) and the committed SMP pack
for pack in --placeholder ""; do
 python3 tools/make_bank.py $pack
 cc -std=c99 -O2 -Wall -Wextra -Werror -Wno-misleading-indentation -Idsp tests/core.c dsp/stamp.c -o build/core_test -lm
 build/core_test build/playground.scf build/flats.scf
done
./build.sh --host
build/host/stamp_host tests/panel.txt build/host
build/host/stamp_host tests/sounds31.txt build/host

build/host/stamp_host tests/polish.txt build/host
build/host/stamp_host tests/features.txt build/host
build/host/stamp_host tests/triplet.txt build/host
build/host/stamp_host tests/clock.txt build/host
build/host/stamp_host tests/groove.txt build/host
build/host/stamp_host tests/bars.txt build/host
build/host/stamp_host tests/autosave.txt build/host
build/host/stamp_host tests/live.txt build/host
build/host/stamp_host tests/controls.txt build/host
build/host/stamp_host tests/flats.txt build/host
build/host/stamp_host tests/erase.txt build/host
