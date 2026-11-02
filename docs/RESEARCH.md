# Inspection record — 2026-11-01

## Pinned upstreams

| Project | Inspected revision | Material inspected |
| --- | --- | --- |
| [Super Mario Paint](https://github.com/DC37/Super-Mario-Paint) | 969e4f55743b3383b450c2a4f9b3a3c4a5e25828 | Java/JavaFX source, README, MIT license, SMP and MPC readers, writer, sound player/tracker, instrument enum, actual SF2 RIFF metadata |
| [fm1_open](https://github.com/m-vave/fm1_open) | 4c7496daac04a95ff10d1afdcc8f9ead2dbabc41 | README architecture, current features, build requirements and stated APP/SDK licensing boundary |
| [fm1-base](https://github.com/charlesvestal/fm1-base) | 02f422c7d9de121ca099bb2fdebe5c035f59e8ad | Native template, build scripts, platform API, input/display definitions, host runtime, linker, licensing |
| [felucca-bsp](https://github.com/hugelton/felucca-bsp) | cd76d43ab4b94d6da9a8f1f24f3f4446135196b7 | Tree, licensing and pinned integration in fm1-base |
| [fm1-emu](https://github.com/joakimeriksson/fm1-emu) | a59210db8922486edbbe5ac0be94b915b3c5285c | Tree and README: executes device packages, unlike native host/WASM simulation |

The official V14 README confirms eight tracks/eight sample slots, step recording, MIDI, and a browser workbench. It describes GPLv3 application source with an FM1 SDK linking exception and separately licensed binary SDK. Public application source does not make its entire DSP stack open source. No official SDK code is copied here.

## Original versus desktop versus this application

Nintendo's 1992 instruction booklet, music-composition section (pages 20–21, [transcription](https://manualzz.com/doc/26101036/nintendo-mario-paint-family-user-manual)), directly describes fifteen sounds, B below middle C through G above the staff, three simultaneous notes, 3/4 or 4/4, tempo, end marker, loop, scrolling and animated playback. The supplied brief's 96-position/24-bar maximum and exact quarter-only limitation were not independently established from that booklet excerpt; do not label them verified from it.

SMP is a separate PC remake. Its README identifies 1.5.1 and Java/JavaFX, lists extensions, undo/redo, accidentals, instrument replacement, volume and arrangements. Current enum has **31** instruments, not fifteen. The full desktop UI, fonts, custom cursors, Java runtime and arbitrary SoundFont loading are not device dependencies. Drawing, animation editing and Gnat Attack are outside this app.

Verified source paths in SMP:

- `src/backend/saving/FileService.java`: human-readable comma-separated header, bar:line index (bar one-based, line zero-based), named notes and VOL.
- `src/backend/saving/smp/SMPDecoder.java`: TEMPO/EXT/TIME/SOUNDSET properties; accidental and mute modifier parsing.
- `src/backend/songs/TimeSignature.java`: denominator is display metadata; line timing does not scale with it.
- `src/gui/Staff.java`: line delay is 60/tempo seconds.
- `src/backend/sound/NoteTracker.java`: with extensions off, the next occupied line for an instrument stops all its prior pitches; this is not a fixed one-grid-step gate.
- `src/backend/sound/SoundPlayer.java`: MIDI synth note-on velocities from line VOL, bank cache/load, mute-pitch and mute-instrument handling.
- `src/gui/SMPInstrument.java`: enum order, 31 IDs, and coin/piranha index swap for soundfont compatibility.
- `src/backend/saving/mpc/MPCDecoder.java`, `MPCInstrumentIndex.java`: legacy format and letter IDs differ. Legacy MPC and arrangements are rejected by the initial converter, not guessed.

C2–C6 is this app's chosen target. The converter enforces it regardless of the desktop's broader values. Extensions, mute commands, noninteger tempo and extra instruments are rejected; changing timbres requires explicit `--placeholder-bank` acknowledgement. No untested full-compatibility claim is made.

## Actual asset inventory

Measured `src/resources/soundfonts/soundset3.sf2`: **7,204,552 file bytes**, **7,194,408 PCM-pool bytes**, **128 sample headers**, **31 presets**, 16-bit PCM. Header rates include 25036, 28104, 31546, 37512, 42110, 44100 and 47266 Hz. The first fifteen presets collectively reference **1,655,824 unique PCM bytes** (about 1.58 MiB); they are not fifteen interchangeable half-second 22.05-kHz clips.

| Program | SMP role | Sample references | Unique PCM bytes |
| --- | --- | ---: | ---: |
| 0 | Mario / piano | 6 | 107430 |
| 1 | Mushroom / kick | 1 | 33214 |
| 2 | Yoshi / zip | 1 | 41914 |
| 3 | Star / bell | 6 | 227946 |
| 4 | Flower / trumpet | 6 | 99250 |
| 5 | Gameboy / pulse | 6 | 69568 |
| 6 | Dog | 1 | 11690 |
| 7 | Cat | 1 | 26242 |
| 8 | Pig | 1 | 7414 |
| 9 | Swan / horn | 1 | 22084 |
| 10 | Face / baby | 1 | 32444 |
| 11 | Plane / guitar | 6 | 374004 |
| 12 | Boat / block | 2 | 22462 |
| 13 | Car / organ | 6 | 311066 |
| 14 | Heart / bass | 6 | 269096 |

`smp-soundfont-inventory.json` records every sample's rate, frames, duration, root, cents, loop offsets and sample type/link; preset-to-instrument-to-sample references and raw generators preserve zone/envelope/tuning evidence. Zone settings can override sample headers. Names like “E4” in sample labels must not be assumed to use the same MIDI octave convention as this application. No soundfont audio is copied into this repository; MIT software licensing alone is not proof of the provenance of every audio/image asset.

Current implementation selects flash/constant-backed short PCM with linear interpolation. It costs 122880 bytes plus 196 for pitch increments and tiny per-voice state. Caching the entire bank in RAM would add 120 KiB for little benefit at this stage. Longer uncompressed originals do not fit this app slot as-is. Root-zone reduction, explicit resampling and sustain loops need listening/tuning tests before deciding the final bank. ADPCM could reduce storage but requires measured per-voice decoding/cache costs. Procedural generation is used only to create clearly labeled original placeholders offline, not to silently substitute generic FM presets for the original palette.


## Presentation pass — 2026-11-02

Inspected the live [Bass Something 2 simulator](https://imijes.github.io/bass-something-2/emu/) and public source revision `44d415fbbc67e37e86ba8ad7b7de91e12e441c52` for presentation quality. Our simulator retains the pinned fm1-base shell, with a custom cream/charcoal/red theme, matching native UI and nearby controls/manual. Character imagery is now included from the pinned SMP revision; see `../assets/NOTICE.md`. Original game audio remains absent.
