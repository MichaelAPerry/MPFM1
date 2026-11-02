# Character stamp provenance

The PNGs in `stamps/` are the first fifteen instrument icons and treble clef from [Super Mario Paint](https://github.com/DC37/Super-Mario-Paint), revision `969e4f55743b3383b450c2a4f9b3a3c4a5e25828`, paths `src/resources/instruments/normal/*.png` and `src/resources/ui/staff/TREBLE_CLEF.png`.

They depict the recognizable Mario Paint character/instrument palette requested for this fan project. Nintendo characters and original artwork remain the property of Nintendo and their respective rights holders. These assets are **not covered by MPFM1's MIT code license**. Upstream's software license is retained in `SMP-LICENSE.txt`; it does not establish a separate Nintendo artwork license. This project is not affiliated with or endorsed by Nintendo or M-VAVE.

The source PNGs are preserved. `tools/make_stamps.py` produces a 16×18 RGB565 representation with nearest-neighbor sampling and an explicit transparency key, used by the native app and its WASM build. No generative reconstruction or smoothing is used. The sounds are separate: see `sounds/NOTICE.md`.
