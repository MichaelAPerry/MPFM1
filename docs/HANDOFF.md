# Handoff: Stamp Composer 0.2

**State:** everything is on branch `device-build` (PR [#2](https://github.com/MichaelAPerry/MPFM1/pull/2) into `main`). Tests, device build, fm1-emu and web all pass. **Never run on a physical FM-1.**

**Left to do**
1. Merge PR #2 into `main` (the workflows only run from `main` for release events).
2. Push tag `v0.2-beta` on `main`: `release.yml` builds, attests and publishes a pre-release.
3. `pages.yml` deploys the site on the published release (or run it by hand: Actions → Pages → `v0.2-beta`). Pages source = GitHub Actions (already on).
4. Optional: regenerate `docs/hero.png` (still shows 15 sounds).

**Why the last session stopped:** its git proxy accepted pushes to the working branch only. Pushes to `main` and tag pushes were cut off. A session or person with normal push rights can finish steps 1–3.

**Build locally:** JieLi toolchain (`curl -fL https://pkgman.jieliapp.com/s/linux-toolchain | tar -xJ`) plus 3 files from AC79 SDK `d179b44` (`cpu/wl82/tools/uboot.boot`, `cfg_tool.bin`, `cfg/eq_cfg_hw.bin`; gitee raw links work). Set `JIELI_TOOLCHAIN`, `AC79_SDK`, then `./build.sh`. Tests: `./tests/run_tests.sh`. Emscripten 4.0.20 for `./build.sh --web`.

**Hardware rule:** no flashing without the owner's go-ahead. Leave the installer's m-bootloader box unticked. Read `docs/INSTALL.md` (backup, back-to-stock, rescue, boot-area notes).

**Where things are:** `docs/FEATURES.md` (built and next), `docs/CONTROLS.md`, `docs/MANUAL.md`, `docs/VALIDATION.md`, `docs/FORMAT.md` (SCF v2).
