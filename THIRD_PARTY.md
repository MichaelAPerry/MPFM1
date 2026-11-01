# Third-party boundaries

The root MIT license applies to the original application, tooling, geometric stamps and generated placeholder sounds in this repository. Existing copyright is retained.

fm1-base and felucca-bsp are pinned git submodules with their original notices and licenses. Their own LICENSING.md files are authoritative for distribution. Core platform files are MPL-2.0; selected files/dependencies are Apache-2.0; fonts have SIL OFL notices. The app does not enable Bluetooth or Wi-Fi. A device package still includes SDK uboot.boot, cfg_tool.bin and eq_cfg_hw.bin; retain their applicable terms and Apache notices. Do not redistribute a future firmware package without its dependency notices and matching source.

Super Mario Paint's repository declares MIT for its software. This project inspected its format and sound engine and independently implements a limited converter. Its Nintendo-themed imagery and sound bank have not been established as independently redistributable, and none are included. `docs/smp-soundfont-inventory.json` is metadata, not sound data. Import names are compatibility identifiers, not branding or bundled character art.

The original Nintendo instruction booklet was consulted for behavioral reference only. No manual pages are bundled. The experimental emulation and host simulation routes do not establish physical hardware behavior.
