#!/bin/sh
set -eu
cd "$(dirname "$0")"
# sound pack: default assets/sounds/smp_bank.bin; STAMP_BANK=placeholder: original generated tones; STAMP_WAV_DIR: user WAVs
if [ -n "${STAMP_WAV_DIR:-}" ]; then
    python3 tools/make_bank.py --wav-dir "$STAMP_WAV_DIR"
elif [ "${STAMP_BANK:-}" = placeholder ]; then
    python3 tools/make_bank.py --placeholder
else
    python3 tools/make_bank.py
fi
python3 tools/make_stamps.py
python3 "${FM1_BASE:-base}/tools/build.py" "$PWD" "$@"


case " $* " in
 *" --web "*) python3 tools/package_preview.py ;;
esac
