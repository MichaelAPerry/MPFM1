# Architecture and status

Use fm1-base at 02f422c7d9de121ca099bb2fdebe5c035f59e8ad with its pinned felucca-bsp cd76d43ab4b94d6da9a8f1f24f3f4446135196b7. It supplies the hardware platform, object storage, graphics, firmware packaging and host/web runtime while allowing original MIT application files. No radio is enabled. This avoids the official firmware's binary DSP SDK dependency while retaining the native device route; packaging still requires SDK files and the JieLi compiler.

`app.c` owns controls, display, edit history and persistence. `dsp/stamp.c` owns bounded note events, validation/serialization, audio-frame sequencing and shared sample voices. Both host and device use these files. `tools/` contains deterministic asset generation, SF2 metadata inspection, and strict SMP conversion.

The audio callback allocates nothing and scans only voices plus events becoming due. An integer phase accumulator advances 96-PPQN ticks from the audio sample clock; tick-zero events trigger before audio is rendered. Remainders carry across blocks and loops. Constant PCM uses linear interpolation and a pitch-ratio table. Free voices win, otherwise the oldest voice is replaced; a 64-frame tail fade and new-voice fade-in reduce stealing steps. MASTER is smoothed. No effects chain or mandatory drum engine exists.

UI edits lock platform interrupts. Large history copies, duplication and serialization are bounded, but lock duration and display contention MUST be profiled on the device. Storage executes from the main loop with playback stopped. The undo ring keeps 16 changes plus a redo endpoint in the larger buffer pool. Native structures are not the wire format.

## Measured application payload on this host ABI

| Item | Bytes |
| --- | ---: |
| Event | 8 |
| Song structure | 3106 |
| Maximum wire record | 3124 |
| 17 history snapshots (pool) | 52802 |
| Engine with sixteen voice slots | 540 |
| One voice | 32 |
| PCM bank (constant/flash target) | 122880 |
| Pitch-increment table | 196 |
| Character stamp ROM (constant) | 9216 |
| Canvas requested from platform (240×112×2, pool) | 53760 |

These are component measurements, not a linked firmware memory report. Runtime platform state, fonts, stacks, USB/audio buffers and alignment are additional. The linked device image, actual free RAM/pool and storage wear are not measured yet. The upstream linker assigns 96 KiB static RAM, 336 KiB pool, and a packed application slot of 0x8DFBC bytes; its larger temporary XIP allowance is not extra installable flash. Eight song slot pairs consume 65536 flash bytes independent of the app slot.

## Remaining work

- Obtain toolchain/SDK, link/package firmware, inspect its map and verify the package identity.
- Execute that exact firmware binary in fm1-emu; test the physical unit before declaring a supported voice count.
- Replace placeholders with a permission-cleared expressive bank; implement tuned zoning, loops, longer tails and explicit SoundFont conversion if needed. Current WAV importer is deliberately strict.
- Device import/export transport, backup, reusable sections and arrangement chains; currently standalone editing and eight slots work in host tests.
- PC corpus testing and optional legacy MPC support; the implemented SMP subset rejects unsupported features.
- Improve dense chord selection, off-band note navigation, comprehensive screen reader change announcements and per-existing-note duration/velocity editing. Current duration/velocity knobs affect new entries.
- USB MIDI performance I/O, Classic restrictions, extra instruments and browser companion editing are secondary.

Completed features and tests are in README and VALIDATION. This branch is an engineering prototype, not the entire product brief marked complete.

