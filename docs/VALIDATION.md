# Validation — polish pass

## Passed

- Native staff, mixer and song-book captures visually inspected at 240×240; deterministic character ROM generation uses 16 source PNGs.
- Polished application rebuilt with Emscripten 4.0.20; WASM size 182,344 bytes.
- New WASM checks cover valid/invalid palette bounds, the 66-note Twinkle demo, demo undo, transport and refusal to replace the song during playback.
- Native regression confirms EDIT+key erases without switching pages, tap toggles mixer, demo undo/redo, playback protection and song-book navigation.
- Existing core, importer and host panel/save/reboot regression suite rerun successfully.

- Native C99 core compiled with warnings treated as errors (compact conditional formatting warning suppressed).
- Real fm1-base host application compiled; panel test ran 3770 ms simulated with 717 blits.
- Mixed sound chord entry, audition, undo/redo, bar duplication, cursor/grid navigation, save/load through simulated reboot.
- Injected flash-write failure kept dirty state and preserved the last saved song across reboot.
- Audio frame timing at ten tempos from 30 through 273 BPM, grid boundaries at quarter/eighth/sixteenth values, looping with nonzero loop start and exclusive song end.
- 384-event capacity refusal and atomic duplication refusal; wire roundtrip/CRC rejection without mutating the previous song.
- Voice exhaustion/stealing and eventual silence after repeated triggers; 8/12/16-voice stress loops.
- Four Python importer tests including unsupported-input subcases; eleven-event example decoded by C core.
- AddressSanitizer and UndefinedBehaviorSanitizer core run passed with `ASAN_OPTIONS=detect_leaks=0`. LeakSanitizer cannot operate under this execution environment's ptrace; the core performs no heap allocations.
- Emscripten 4.0.20 compiled the same application to WASM. Node executed that preview module: key input, SCF1 import/export, audio, refusal to import while playing, save/reboot persistence.

Representative optimized host stress measurements (4000 blocks × 256 frames, retriggering every voice each block): 8 voices ~0.11%, 12 ~0.17%, 16 ~0.23% of audio duration on this host CPU. This deliberately stresses stealing. **These percentages do not predict PI32 CPU headroom.** Native panel test also renders UI; no target worst-case display/audio claim is made. The app defaults to eight voices provisionally.

## Device build (PI32v2)

- `./build.sh` produced `build/stamp.fwsc` (611,496 B, SHA-256 `e1b0401dfd01cbc07b444515f3a1156333be89cb66f65b4dcb99d0f430b2950b`), dev identity `FM-1_5600`; app image `build/stamp.bin` 220,076 B (`deaaa25f…7cb9246`).
- Toolchain `jieli-linux-toolchains-20260730.1` (clang 4.0.1, from `pkgman.jieliapp.com/s/linux-toolchain`, archive SHA-256 `a1545cdd…c7647bd`). SDK files `uboot.boot`, `cfg_tool.bin`, `cfg/eq_cfg_hw.bin` from fw-AC79_AIoT_SDK d179b44 (`AC79NN_SDK_V1.2.1_2023-12-13`); all three match build.py's pinned SHA-256.
- Linked ELF against fm1-base limits: app image 220,076 of 581,564 B slot (0x8DFBC, 37.8%); static RAM .data+.bss 23,224 B + .ram_text 3,132 B, build check reports 23,236 of 98,304 B; pool 106,828 of 344,064 B (31%, history + canvas). No `.map` is emitted; figures are from `objdump -h` and build.py's checks.
- All build.py device checks passed: register access, no inlined 64-bit multiply beside loads (bsp docs/CPU.md erratum; `sc_note` duration math made 32-bit exact), no soft-double, `.ram_text` call-free.
- Device-only `dsp/libc_shim.h` (memmove, strcmp, minimal snprintf) matched glibc on 16 app format cases plus truncation under ASan/UBSan.
- Identity `FM-1_56` chosen after reading all 36 public `fm1.toml` id_prefix values on GitHub: `FM-1_73` is used by gravkode/terykons and haunted-fm1; no published prefix begins with 5. Firmware outside GitHub (e.g. thefirmwarehub.com) was not checked.

## Device package in fm1-emu (joakimeriksson/fm1-emu 9de1ef0)

- Command line (`tests/emu_smoke.sh`): `stamp.fwsc` boots to the compose screen; a chord and two notes placed with keys and SELECT; PLAY shows the playhead and the WAV has the chord and notes 1.00 s apart (two beats at 120 BPM), silence after stop; SAVE shows SAVED. No faults; 1273 LCD frames in 7.5 s.
- Save/reboot: flash dumped after SAVE and restored into a fresh boot (two-line local fm1-emu patch: `FM1_FLASH_OUT` writes the chip, `FM1_FLASH_IN` calls its `restore_flash` after boot; stock `--raw-flash` is overwritten by the package's flash at boot). The song reloaded with 5 notes and played — [capture](screens/fm1-emu-reboot.png).
- Browser: fm1-emu's web build with `tools/site_emu.sh` beside the release package `stamp-0.1.fwsc` (`FM-1_56000100`) booted under headless Chromium/Playwright with no console errors; computer-keyboard notes and Space (PLAY) placed three notes and started playback.
- fm1-emu emulates the CPU and peripherals; it is not hardware. Timing, flash wear, audio output stage and the PI32 erratum are not reproduced faithfully. Emulated runs used the emulator's default 100 MHz clock.

## Feature set 0.2 (docs/FEATURES.md)

- Engine: SCF v2 (v1 still loads), per-sound tune/tone/decay/pan, MIDI out per sound with note-off scheduling, all-notes-off at stop, 24 PPQN clock with Start/Stop. With default settings the render is bit-identical to 0.1 (hash test). Core tests cover v1/v2 decode, every new range check, shaping effects, MIDI timing at 120 BPM, clock count, mode MIDI ONLY and the note-off table overflow.
- App: SOUND and MIDI pages, MIDI in note entry, EDIT+K2/K3 note editing, backup/restore hooks (flush is a no-op: a backup holds what is saved). `tests/features.txt` covers page cycle, knob clamps, undo, MIDI out present/absent, MIDI in, note editing and restore. [SOUND](screens/sound.png) and [MIDI](screens/midi.png) pages inspected.
- Device build checks all ok: image 227,220 B of 581,564; RAM 24,500 of 98,304; pool 111,960 of 344,064. fm1-emu smoke on that package still plays with no faults. WASM rebuilt with Emscripten 4.0.20 (`preview/app.wasm`); `tests/web.mjs` passes.
- Not validated: MIDI out/in and clock against a real DAW or synth; backup/restore through fm1-base's browser page or `fm1_backup.py`; the new pages on hardware.

## Pre-beta features (triplets to autosave)

- Built and checked on the host, the device build (all checks ok; image 238,424 B, RAM 33,548 B, pool 125,004 B) and the WASM preview: triplet grids, BEATS 2–12, playback hop, screen mirror, external clock IN (core test: worst drift 0.27 ticks over 64 bars at 120 BPM with ±1 ms jitter), layer mute/solo, swing/humanize (GROOVE page), play from cursor with SPP, HOME-hold help, BARS range ops, autosave with restore at power-on. Host scripts: triplet, clock, groove, bars, autosave.
- fm1-emu on the device package: help overlay, BARS page, clock IN (EXT header from injected F8s) and autosave restore after a flash dump/restore reboot (2 unsaved notes back) were each seen on the emulated firmware.
- Site: release `stamp-0.2.fwsc` (`FM-1_56000200`) assembled with `make_site.py` + `tools/site_emu.sh`; front page, install, manual, emu, backup, mirror and fm1-emu pages load in headless Chromium without errors (with fm1-reader present), and the release boots in fm1-emu's page.
- Not validated: any of the above on a physical FM-1; MIDI with a real DAW/synth; backup/restore and mirror with fm1-base's pages against a device; flash wear of autosave.

## Not validated

- No physical FM-1 was tested: real boot, display timing, audio quality, input feel and flash storage are unverified. Build reproducibility (byte-identical rebuild) not checked.
- The full Pages site (`make_site.py`) was not assembled: it needs Emscripten for `build/emu`, which this environment lacks.
- No linker map, measured runtime peak RAM/stack, audible quality evaluation or sample-fidelity approval.
- Full browser UI interaction/AudioWorklet scheduling remains unverified. The cloud browser could inspect the reference simulator but could not reach this workspace’s local preview server (`ERR_HTTP_RESPONSE_CODE_FAILURE`). Node WASM tests are distinct from a full browser session.
- The imported example is an authored fixture of the inspected SMP syntax. A real-world SMP/MPC corpus and proprietary sample-bank comparisons remain outstanding.

## Next physical checks

1. Check package identity, boot/update/rescue and panel calibration preservation.
2. Verify every encoder direction/detent and key mapping, display bands and input latency.
3. Run dense 8/12/16-voice songs with all sounds, repeated notes, longest tails and continuous display redraw; record platform CPU/xruns, lock duration, flash cache behavior and peak RAM.
4. Measure pitch across C2–C6, aliasing at high transposition, clipping, MASTER motion and stealing clicks through headphones/line output.
5. Test interrupted saves and all eight slots, recovery from malformed records, and storage lifetime assumptions.
6. Only then set a supported hardware voice limit and distribute an installable package with matching source/notices.

