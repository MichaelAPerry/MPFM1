# SCF1 versions 1 and 2

All multibyte fields are little endian. No C structs are dumped to disk. Total size = 52 + 8 × event count (v1) or 132 + 8 × event count (v2), maximum 3204 bytes, below the platform's 3840-byte object limit. Decoder checks CRC, lengths, ranges, sorted timing, version and reserved bytes before replacing the current song.

| Offset | Bytes | Meaning |
| --- | --- | --- |
| 0 | 4 | ASCII SCF1 |
| 4 | 2 | Version 1 or 2 (the encoder writes 2) |
| 6 | 2 | Event count, 0–384 |
| 8 | 2 | Integer BPM, 30–300 |
| 10 | 2 | Exclusive song end tick, 1–65535 |
| 12 | 2 | Loop start tick, less than end |
| 14 | 1 | Loop enabled, 0/1 |
| 15 | 1 | Beats per bar, 1–16 (device default 4) |
| 16 | 15 | Per-instrument volume 0–127 |
| 31 | 8 | Per-layer volume 0–127 |
| 39 | 9 | Reserved zero; nonzero rejected |
| 48 | 79 | v2 only: sound block (below) |
| 127 | 1 | v2 only: reserved zero |
| 48 (v1) / 128 (v2) | count × 8 | Events |
| final 4 | 4 | IEEE CRC32 of preceding bytes |

v2 sound block, offsets from 48: tune i8 ×15 (−12..12 semitones), tone u8 ×15 (0–127, 127 open), decay u8 ×15 (0–127, 0 = stop at note end with the short fade), pan i8 ×15 (−64..63), MIDI channel u8 ×15 (0 off, 1–16), MIDI mode u8 (0 off, 1 internal + MIDI, 2 MIDI only), MIDI clock u8 (0 off, 1 out, 2 in: follow external clock), swing u8 (0–75, % late of off-beat 8ths/16ths), human u8 (0–100, timing/velocity randomness). 0/0 plays exactly as written. Mute and solo are not stored. Out-of-range values are rejected. A v1 file decodes with swing 0, human 0, tune 0, tone 127, decay 0, pan 0, MIDI off, and sounds the same.

Each event: tick:u16, duration:u16, MIDI pitch:u8, instrument:u8, velocity:u8, layer:u8. Duration must be positive. Pitch is 36–84 (C2–C6); instrument 0–14; velocity 0–127; layer 0–7. Events may share a tick. Time is 96 ticks per quarter; grids are 96, 48, 24. Duration gates a one-shot sample; it cannot lengthen the finite sample. A natural sample ending or stolen voice gets a short fade. Voices are independent even for repeated pitches.

Instrument IDs follow the first fifteen SMP enum positions: piano, kick, zip, bell, trumpet, pulse, dog, cat, pig, horn, baby, guitar, block, organ, bass. Percussion is pitched by the same sample-rate rule, with no special reserved track.

Maximum time span is about 170 4/4 bars, subject to the 384-event limit. There are no section references in v1. A future version must explicitly define reusable sections/arrangements and migration, not reinterpret reserved bytes. Eight fixed A/B storage-object pairs at 0x97000–0xA6FFF store songs; panel calibration remains at 0xDC000–0xDDFFF. Object IDs and offsets must remain stable across updates. Host storage reproduces API behavior; power-loss reliability on real flash is still to be tested.
