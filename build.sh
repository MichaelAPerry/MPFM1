#!/bin/sh
set -eu
cd "$(dirname "$0")"
if [ -n "${STAMP_WAV_DIR:-}" ]; then
    python3 tools/make_bank.py --wav-dir "$STAMP_WAV_DIR"
else
    python3 tools/make_bank.py
fi
python3 tools/make_stamps.py
python3 "${FM1_BASE:-base}/tools/build.py" "$PWD" "$@"


case " $* " in
 *" --web "*) python3 tools/package_preview.py ;;
esac
