# Feature set 0.2

What the composer needs to be a full instrument, beyond 0.1's staff, mixer and song book. **Built** items are in
this branch; **Next** items are designed but not started. Validation lives in VALIDATION.md.

## Built in 0.2

| Feature | What it does | Where it is stored |
| --- | --- | --- |
| 31 sounds | One stamp per Super Mario Paint instrument (Mario .. Gnome, SMP order). The ribbon scrolls a window around the selected sound; the MIDI page shows all 31 channels in an 8 x 4 grid of stamps. Stamp ROM: 31 stamps plus the treble clef (id 31). | Song (SCF v2: per-sound arrays of 31) |
| Real sound pack | `assets/sounds/smp_bank.bin`: Mario Paint samples (ADPCM, 22,050 Hz), levels matched; a swappable asset pack with its own notice (Nintendo's sounds, not MIT). `STAMP_BANK=placeholder` or `STAMP_WAV_DIR` replace it. | Firmware asset |
| Loudness | Per-voice gain puts a C4 at velocity 100 near -12 dBFS; a soft limiter (linear to -6 dBFS, then an x/(1+x) knee, never full scale) keeps chords and 16-voice passes from clipping. MASTER is unchanged. | — |
| Twinkle demo | Twinkle Twinkle (public-domain melody from one of Mario Paint's three demo songs; our 66-note arrangement: STAR melody, HEART bass, 12 bars, 100 BPM, loop). Song book DEMO entry after slot 8 (REC twice loads it, undoable; SAVE refused), browser button, host `demo` command. | — |
| Sound shaping | Per sound: TUNE −12..+12 semitones, TONE (one-pole low-pass, 0 dark .. 127 open), DECAY (release after the note's length, 0 short .. 127 long), PAN (−64 left .. +63 right). New SOUND page. | Song (SCF v2) |
| MIDI out per sound | Per sound: channel OFF or 1–16. Note-on at trigger with the note's velocity; note-off after its length; all notes off on stop. Auditions while editing go out too. | Song (SCF v2) |
| MIDI mode | OFF, ON (internal + MIDI) or MIDI ONLY (internal muted, e.g. driving a synth). | Song |
| MIDI clock out | Optional 24 PPQN clock with Start/Stop, following the song tempo. | Song |
| External clock sync | MIDI page K3 clock IN: Start/Continue/Stop and 24 PPQN clocks (USB, Bluetooth, TRS) drive playback; tempo is measured (mean of 24 intervals) and the playhead is phase-locked to 4 ticks per clock; header shows EXT. See CONTROLS.md. | Song (`midi_clock` 2) |
| MIDI in note entry | Notes arriving over USB, Bluetooth and TRS act as key presses: on the staff audition and place with the selected sound (cursor, or playhead while playing), elsewhere audition only. MIDI page K4 filters notes by channel (OMNI or 1–16); clock and transport messages are never filtered. | Song (`midi_in`, SCF v2 byte 127) |
| Edit existing notes | Hold EDIT and turn K2/K3: velocity/length of the selected sound's notes at the cursor. | Song |
| Triplet grids | SEQ cycles 1/4, 1/8, 1/16, 1/8T (32 ticks), 1/16T (16 ticks); K3 length steps follow the grid (16 ticks on triplets). | — |
| Time signature | MIX K1 BEATS 2–12 per bar (n/4); bar lines, BAR n, bar copy and song end follow. | Song (`beats`, SCF v2) |
| Sound mute | FX on every page except MIX and BARS mutes/unmutes the selected sound without touching its volume: no audio, no auditions, no MIDI out. Runtime only (engine mask), not saved; shown as a checkered stamp and an M on the ribbon. | — |
| Layer mute/solo | MIX page: ALGO picks the layer, FX mutes it, SEL solos it (several layers may be soloed). Works during playback; silences internal audio and MIDI out. Runtime only (engine mask), not saved; the staff ribbon shows `LAYER n M` / `SOLO` / `OFF`. | — |
| Swing and humanize | New GROOVE page: SWING 0–75 % of the off-beats of one grid (K3: 8TH = off-beat 8ths, 16TH = off-beat 16ths), HUMAN 0–100 (random timing ±8 ticks, velocity ±20 at 100; K4: TIME+VEL, TIME or VEL; deterministic per playback start). 0/0 renders as before. | Song (SCF v2 `swing`, `human`, byte 127) |
| Play from cursor | Hold EDIT and press PLAY: starts at the cursor's bar; clock OUT sends Song Position Pointer + Continue. | — |
| Help overlay | Hold HOME 600 ms: full-screen list of the current page's controls (docs/screens/help.png); release hides it. Playback unaffected. | — |
| Playback animation | The selected character hops along the staff at the playhead, landing before each step with a note start (docs/screens/playback-hop.png). | — |
| Screen mirror | `mirror = true`; `app_redraw` is a no-op because every frame repaints the whole screen. | — |
| Flats | Black keys can be spelled as flats: a per-note flag (bit 7 of the SCF v2 layer byte, no effect on sound) drawn on the next letter's line with a flat sign, header name Bb3/Eb4. Spelling mode SHARP / AUTO (Eb Ab Bb, C# F#) / FLAT for new notes, keys, MIDI in and live placement; EDIT+OCT−/OCT+ on the staff steps the mode toward flat/sharp and respells the notes at the cursor (one undo step). BARS transpose keeps flats on black keys. The SMP importer keeps b/B accidentals (docs/screens/flats.png). | Song |
| Bar ranges | New BARS page: pick bars FROM..TO (inclusive) and a TARGET bar, then COPY (merge: notes already there kept, same tick/pitch/sound/layer skipped), MOVE, DELETE bars (later notes, song end and loop start shift left), INSERT empty bars (shift right) or TRANSPOSE up/down 1–12 semitones (OCT+/OCT−). Copy/move/transpose follow the SEL sound filter; delete/insert take every sound. Each op is checked first (capacity, 65535 ticks, C2–C6), refused whole with a message, and is one undo step (docs/screens/bars.png). | Song |
| Autosave | The working song (with its slot and unsaved flag) is written to its own flash object (OBJ_AUTO) once you have stopped, left the keys and buttons and changed nothing for 3 s, at most every 20 s; never while playing or during a backup/restore. At power-up it comes back (message RESTORED), unsaved edits included; a damaged record falls back to slot 1. SAVE also refreshes it at once. Not in the USB backup (that is the eight slots). | Flash (OBJ_AUTO, 0xA7000) |
| Live placement | While playing, a key or MIDI-in note auditions and is placed at the playhead snapped to the nearest grid step (wraps to the loop start past the end, else the last step; duplicates skipped); EDIT+key erases there. One undo step per playback run; the engine scan index is kept consistent (`sc_insert`/`sc_remove`). | Song |
| Free play | OCT− + OCT+ together toggle a mode where keys only play the selected sound (gated by key hold, MIDI note-off on release), over a stopped or playing song; no placing or erasing. Header FREE. | — |
| Backup and restore | fm1-base backup over USB (browser page or `fm1_backup.py`): all eight song slots, all or nothing. | — |

Defaults keep 0.1 songs sounding the same: tune 0, tone 127, decay 0, pan 0, MIDI OFF. SCF v1 files still load.

## Pages

Per-page controls are tabulated in CONTROLS.md; keys place notes on the staff only (other pages audition), and REC bar copy, SEQ and ARP act on the staff only. EDIT tap (a clean tap: any other input while EDIT is held cancels it) cycles STAFF → MIX → SOUND → MIDI → GROOVE → BARS → STAFF; GLO toggles the song book; HOME returns to the staff.

| Page | K1 | K2 | K3 | K4 |
| --- | --- | --- | --- | --- |
| MIX | beats per bar | sound volume | layer volume | song end (FX mute, SEL solo the layer) |
| SOUND | tune | tone | decay | pan |
| MIDI | sound's channel | MIDI mode | clock OFF/OUT/IN | MIDI-in channel OMNI/1–16 |
| GROOVE | swing | human | swing grid 8TH/16TH | human mode TIME+VEL/TIME/VEL |
| BARS | from bar | to bar | target bar | transpose amount 1–12 (REC copy, FX move, SEQ delete, ARP insert, OCT− down, OCT+ up) |

## Next

1. **Sections and chains**: name up to 8 bar ranges and chain them into a song (needs SCF v3 with explicit
   migration; FORMAT.md forbids reusing reserved bytes).
2. **Loops and extra zones**: sustain loops tuned per sound and more sample zones per instrument (tools/make_smp_bank.py).
3. **Hardware voice limit**: measure 8/12/16 voices on a physical FM-1 and set the supported count.
4. **Live record** with count-in, metronome and quantize.
5. **Loop selected bars** instead of the whole song.
6. **Colour themes** for the screen.
7. **Chord follow / arp**: held chords or arpeggios follow the played key.
8. **More song slots and song chaining.**
9. **Recorded automation** of knobs (tone, pan, volume) over time.
10. **Ratchets and rolls** on single notes.
11. **Custom stamp sounds from WAV** loaded on the device.
12. **WAV bounce over USB audio** of the finished song.
13. **SMF export** of the song as a Standard MIDI File.
