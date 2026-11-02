# SCF1 versions 1 and 2

All multibyte fields are little endian. No C structs are dumped to disk. Decoder checks CRC, lengths, ranges, sorted timing, version and reserved bytes before replacing the current song.

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
| 16 | 15 (v1) / 31 (v2) | Per-instrument volume 0–127 |
| 31 (v1) / 47 (v2) | 8 | Per-layer volume 0–127 |
| 39 (v1) | 9 | v1 only: reserved zero; nonzero rejected |
| 48 (v1) / 216 (v2) | count × 8 | Events |
| final 4 | 4 | IEEE CRC32 of preceding bytes |

v2 head (216 bytes; the v1 head is the first 48 bytes above): volume u8 ×31 at 16, layer volume u8 ×8 at 47, then the sound block: tune i8 ×31 at 55 (−12..12 semitones), tone u8 ×31 at 86 (0–127, 127 open), decay u8 ×31 at 117 (0–127, 0 = stop at note end with the short fade), pan i8 ×31 at 148 (−64..63), MIDI channel u8 ×31 at 179 (0 off, 1–16), then MIDI mode u8 at 210 (0 off, 1 internal + MIDI, 2 MIDI only), MIDI clock u8 at 211 (0 off, 1 out, 2 in: follow external clock), swing u8 at 212 (0–75, % of the half-beat the off-beats of the chosen grid are late: 8ths at tick%96==48 by up to 48 ticks, or 16ths at tick%48==24 by up to 24), human u8 at 213 (0–100, randomness), flags u8 at 214 (bit 0 swing grid: 0 off-beat 8ths, 1 off-beat 16ths; bits 1–2 human mode: 0 timing + velocity, 1 timing only, 2 velocity only, 3 rejected; bits 3–7 MIDI-in channel: 0 omni, 1–16, above 16 rejected), byte 215 reserved zero (nonzero rejected). 0/0 plays exactly as written. Mute and solo are not stored. Out-of-range values are rejected. A v1 file (sounds 0–14 only) decodes with volume 127 for sounds 15–30, swing 0, human 0, tune 0, tone 127, decay 0, pan 0, MIDI off, and sounds the same. Total size = 52 + 8 × events (v1) or 220 + 8 × events (v2), at most 3292 bytes (the autosave record adds 8), below the platform's 3840-byte object limit.

Each event: tick:u16, duration:u16, MIDI pitch:u8, instrument:u8, velocity:u8, layer:u8. Duration must be positive. Pitch is 36–84 (C2–C6); instrument 0–30; velocity 0–127; layer byte: bits 0–6 the layer 0–7, bit 7 (0x80) FLAT spelling (v2 only; a v1 layer byte above 7 is rejected). FLAT means the note is shown as the flat of the next letter (Bb, not A#) and has no effect on sound or MIDI. It is only valid on black-key pitches (C#, D#, F#, G#, A#): a file with it on a white key is rejected (not normalised), and the encoder refuses such a song. A v1 file decodes with the flag clear. Spelling is not part of a note's identity: the duplicate rules (live placement, copy/move merge, erase) match on tick, pitch, sound and layer and ignore it. Events may share a tick. Time is 96 ticks per quarter; the editor's grids are 96, 48, 24, 32 (1/8T) and 16 (1/16T); the format stores plain ticks. Beats per bar: the format accepts 1–16, the UI offers 2–12. Duration gates a sample; it cannot lengthen a one-shot sample (a looped one, if the pack has any, sustains for the duration). A natural sample ending or stolen voice gets a short fade. Voices are independent even for repeated pitches.

Instrument IDs 0–30 are the SMP `SMPInstrument` enum order (the soundset3.sf2 programs of COIN and PIRANHA are swapped; ids follow the enum, the pack is built by name): mario/piano, mushroom/kick, yoshi/zip, star/bell, flower/trumpet, gameboy/pulse, dog, cat, pig, swan/horn, face/baby, plane/guitar, boat/block, car/organ, heart/bass, piranha, coin, shy guy, boo, luigi, peach, feather, bullet bill, goomba, bob-omb, spiny, fruit, 1-up, moon, egg, gnome. Each sound is one sample of the sound pack (docs/ARCHITECTURE.md), played at its root key (the stored cents and cents-per-key apply), so a pitch is the sample's rate ratio; there is no special reserved percussion track.

Maximum time span is about 170 4/4 bars, subject to the 384-event limit. There are no section references in v1. A future version must explicitly define reusable sections/arrangements and migration, not reinterpret reserved bytes. Eight fixed A/B storage-object pairs at 0x97000–0xA6FFF store songs; panel calibration remains at 0xDC000–0xDDFFF. Object IDs and offsets must remain stable across updates. Host storage reproduces API behavior; power-loss reliability on real flash is still to be tested.
