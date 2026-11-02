# Third-party boundaries

The root MIT license applies to the original application, tooling and the generated placeholder sounds (`tools/make_bank.py --placeholder`) in this repository. Existing copyright is retained.

fm1-base and felucca-bsp are pinned git submodules with their original notices and licenses. Their own LICENSING.md files are authoritative for distribution. Core platform files are MPL-2.0; selected files/dependencies are Apache-2.0; fonts have SIL OFL notices. The app does not enable Bluetooth or Wi-Fi. A device package still includes SDK uboot.boot, cfg_tool.bin and eq_cfg_hw.bin; retain their applicable terms and Apache notices. Do not redistribute a future firmware package without its dependency notices and matching source.

Super Mario Paint's repository declares MIT for its software. This project inspected its format and sound engine and independently implements a limited converter. The first fifteen character PNGs and treble clef are now included, with revision and rights boundaries recorded in `assets/NOTICE.md` and the upstream software license in `assets/SMP-LICENSE.txt`. The Nintendo artwork is not licensed by this project’s MIT code license. The sound pack `assets/sounds/smp_bank.bin` (one 22,050 Hz IMA ADPCM sample per instrument, from Super Mario Paint's `soundset3.sf2`, Mochi115/Cyan's soundset, derived from Nintendo's Mario Paint sounds) is included on the same footing as the stamps: not covered by the MIT code license, no separate license from Nintendo; provenance and how to swap it are in `assets/sounds/NOTICE.md`. `docs/smp-soundfont-inventory.json` is metadata, not sound data. This is an unaffiliated fan project.

The original Nintendo instruction booklet was consulted for behavioral reference only. No manual pages are bundled. The experimental emulation and host simulation routes do not establish physical hardware behavior.


The browser shell and worklet are modified fm1-base MPL-2.0 files and retain their original notices. `theme.css`, `app.js`, `app_worklet.js` and the companion manual are application additions. Prebuilt preview dependency notices are in `preview/licenses/`.
