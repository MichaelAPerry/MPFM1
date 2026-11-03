# Architecture and status

Use fm1-base at 02f422c7d9de121ca099bb2fdebe5c035f59e8ad with its pinned felucca-bsp cd76d43ab4b94d6da9a8f1f24f3f4446135196b7. It supplies the hardware platform, object storage, graphics, firmware packaging and host/web runtime while allowing original MIT application files. No radio is enabled. This avoids the official firmware's binary DSP SDK dependency while retaining the native device route; packaging still requires SDK files and the JieLi compiler.

`app.c` owns controls, display, edit history and persistence. `dsp/stamp.c` owns bounded note events, validation/serialization, audio-frame sequencing and shared sample voices. Both host and device use these files. `tools/` contains deterministic asset generation, SF2 metadata inspection, and strict SMP conversion.

The audio callback allocates nothing and scans only voices plus events becoming due. An integer phase accumulator advances 96-PPQN ticks from the audio sample clock; tick-zero events trigger before audio is rendered. Remainders carry across blocks and loops. Constant PCM uses linear interpolation and a pitch-ratio table. Free voices win, otherwise the oldest voice is replaced; a 64-frame tail fade and new-voice fade-in reduce stealing steps. MASTER is smoothed. No effects chain or mandatory drum engine exists.

UI edits lock platform interrupts. Large history copies, duplication and serialization are bounded, but lock duration and display contention MUST be profiled on the device. Storage executes from the main loop with playback stopped. The undo ring keeps 16 changes plus a redo endpoint in the larger buffer pool. Native structures are not the wire format.

## Measured application payload on this host ABI

| Item | Bytes |
| --- | ---: |
| Event | 8 |
| Song structure | 3284 |
| Maximum wire record | 3292 |
| 17 history snapshots (pool) | 55828 |
| Engine with sixteen voice slots | 1920 |
| One voice | 72 |
| Sound pack: 31 instruments, IMA ADPCM 22050 Hz (constant/flash target) | 204468 |
| Pack table (31 × 24-byte entries) | 744 |
| Character stamp ROM (constant) | 9216 |
| Canvas requested from platform (240×112×2, pool) | 53760 |

These are component measurements, not a linked firmware memory report. Runtime platform state, fonts, stacks, USB/audio buffers and alignment are additional. The linked device image, actual free RAM/pool and storage wear are not measured yet. The upstream linker assigns 96 KiB static RAM, 336 KiB pool, and a packed application slot of 0x8DFBC bytes; its larger temporary XIP allowance is not extra installable flash. Eight song slot pairs (0x97000..0xA6FFF) consume 65536 flash bytes independent of the app slot; the autosave pair (OBJ_AUTO, 0xA7000..0xA8FFF, 8 byte header + up to 3292 byte SCF record) adds 8192 more. OBJ_PANEL stays at 0xDC000.

## Remaining work

- Obtain toolchain/SDK, link/package firmware, inspect its map and verify the package identity.
- Execute that exact firmware binary in fm1-emu; test the physical unit before declaring a supported voice count.
- The sound pack (assets/sounds/smp_bank.bin) holds Mario Paint's own samples (one zone per instrument, full length): get explicit permission before any wider distribution, or swap in permission-cleared sounds (assets/sounds/NOTICE.md). More zones per instrument (tuned ranges) are possible within the 581,564-byte slot.
- Device import/export transport, backup, reusable sections and arrangement chains; currently standalone editing and eight slots work in host tests.
- PC corpus testing and optional legacy MPC support; the implemented SMP subset rejects unsupported features.
- Improve dense chord selection, off-band note navigation, comprehensive screen reader change announcements and per-existing-note duration/velocity editing. Current duration/velocity knobs affect new entries.
- USB MIDI performance I/O, Classic restrictions and browser companion editing are secondary.

Completed features and tests are in README and VALIDATION. This branch is an engineering prototype, not the entire product brief marked complete.

