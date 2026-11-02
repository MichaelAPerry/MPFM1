# Validation — polish pass

## Passed

- Native staff, mixer and song-book captures visually inspected at 240×240; deterministic character ROM generation uses 16 source PNGs.
- Polished application rebuilt with Emscripten 4.0.20; WASM size 182,344 bytes.
- New WASM checks cover valid/invalid palette bounds, the 32-note demo, demo undo, transport and refusal to replace the song during playback.
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

## Not validated

- No PI32 firmware binary was built: the build stopped because `cpu/wl82/tools/uboot.boot` was absent; the JieLi toolchain and AC79 SDK are not configured.
- No device binary was executed in fm1-emu and no physical FM-1 was tested.
- No final device linker map, total flash/RAM report, audible quality evaluation or sample-fidelity approval.
- Full browser UI interaction/AudioWorklet scheduling remains unverified. The cloud browser could inspect the reference simulator but could not reach this workspace’s local preview server (`ERR_HTTP_RESPONSE_CODE_FAILURE`). Node WASM tests are distinct from a full browser session.
- The imported example is an authored fixture of the inspected SMP syntax. A real-world SMP/MPC corpus and proprietary sample-bank comparisons remain outstanding.

## Next physical checks

1. Check package identity, boot/update/rescue and panel calibration preservation.
2. Verify every encoder direction/detent and key mapping, display bands and input latency.
3. Run dense 8/12/16-voice songs with all sounds, repeated notes, longest tails and continuous display redraw; record platform CPU/xruns, lock duration, flash cache behavior and peak RAM.
4. Measure pitch across C2–C6, aliasing at high transposition, clipping, MASTER motion and stealing clicks through headphones/line output.
5. Test interrupted saves and all eight slots, recovery from malformed records, and storage lifetime assumptions.
6. Only then set a supported hardware voice limit and distribute an installable package with matching source/notices.

