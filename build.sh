#!/bin/sh
set -eu
cd "$(dirname "$0")"
if [ -n "${STAMP_WAV_DIR:-}" ]; then
    python3 tools/make_bank.py --wav-dir "$STAMP_WAV_DIR"
else
    python3 tools/make_bank.py
fi
exec python3 "${FM1_BASE:-base}/tools/build.py" "$PWD" "$@"
