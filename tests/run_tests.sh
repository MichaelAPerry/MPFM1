#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p build
python3 tools/make_bank.py
python3 tools/import_smp.py examples/playground.smp build/playground.scf --placeholder-bank
python3 -m unittest discover -s tests -p 'test_*.py'
cc -std=c99 -O2 -Wall -Wextra -Werror -Wno-misleading-indentation -Idsp tests/core.c dsp/stamp.c -o build/core_test
build/core_test build/playground.scf
./build.sh --host
build/host/stamp_host tests/panel.txt build/host

build/host/stamp_host tests/polish.txt build/host
build/host/stamp_host tests/features.txt build/host
build/host/stamp_host tests/triplet.txt build/host
build/host/stamp_host tests/clock.txt build/host
build/host/stamp_host tests/groove.txt build/host
build/host/stamp_host tests/bars.txt build/host
build/host/stamp_host tests/autosave.txt build/host
build/host/stamp_host tests/live.txt build/host
build/host/stamp_host tests/controls.txt build/host
