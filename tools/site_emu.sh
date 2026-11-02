#!/bin/sh
# SPDX-License-Identifier: MIT
# Add the real firmware, emulated, to a site made by base/tools/make_site.py: fm1-emu's web build (MIT,
# github.com/joakimeriksson/fm1-emu) in SITE/fm1-emu/, its catalog holding only firmware/stamp-VERSION.fwsc.
# emu/ beside it stays the WASM preview of the same C source; fm1-emu/ runs the PI32 package itself.
#   tools/site_emu.sh SITE_DIR VERSION [FM1_EMU_DIR]   (FM1_EMU_DIR: a clone at FM1_EMU_REV; cloned if absent)
set -eu
SITE="${1:?usage: tools/site_emu.sh SITE_DIR VERSION [FM1_EMU_DIR]}"; V="${2:?version}"
REV="${FM1_EMU_REV:-9de1ef0}"; EMU="${3:-build/fm1-emu-src}"
[ -f "$SITE/firmware/stamp-$V.fwsc" ] || { echo "site_emu.sh: no $SITE/firmware/stamp-$V.fwsc"; exit 1; }
[ -d "$EMU/.git" ] || git clone -q https://github.com/joakimeriksson/fm1-emu "$EMU"
git -C "$EMU" -c advice.detachedHead=false checkout -q "$REV"
rustup target add wasm32-unknown-unknown >/dev/null 2>&1 || true
bash "$EMU/web/build.sh" "$(cd "$SITE" && pwd)/fm1-emu"
cp "$EMU/LICENSE" "$SITE/fm1-emu/LICENSE"
python3 - "$SITE/fm1-emu/firmwares.json" "$V" <<'PY'
import json, sys
json.dump({"note": "Stamp Composer only: the PI32 firmware package, emulated.", "default": "stamp",
           "firmwares": [{"id": "stamp", "name": "Stamp Composer", "version": sys.argv[2], "author": "Michael Perry",
                          "license": "MIT", "repo": "https://github.com/MichaelAPerry/MPFM1",
                          "about": "A Mario Paint-inspired music composer: the firmware package itself, emulated.",
                          "package": f"../firmware/stamp-{sys.argv[2]}.fwsc", "installer": "../install/"}]},
          open(sys.argv[1], "w"), indent=1)
PY
echo "site_emu: $SITE/fm1-emu (fm1-emu $REV, stamp-$V.fwsc)"
