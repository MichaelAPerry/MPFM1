# Feature set 0.2

What the composer needs to be a full instrument, beyond 0.1's staff, mixer and song book. **Built** items are in
this branch; **Next** items are designed but not started. Validation lives in VALIDATION.md.

## Built in 0.2

| Feature | What it does | Where it is stored |
| --- | --- | --- |
| Sound shaping | Per sound: TUNE −12..+12 semitones, TONE (one-pole low-pass, 0 dark .. 127 open), DECAY (release after the note's length, 0 short .. 127 long), PAN (−64 left .. +63 right). New SOUND page. | Song (SCF v2) |
| MIDI out per sound | Per sound: channel OFF or 1–16. Note-on at trigger with the note's velocity; note-off after its length; all notes off on stop. Auditions while editing go out too. | Song (SCF v2) |
| MIDI mode | OFF, ON (internal + MIDI) or MIDI ONLY (internal muted, e.g. driving a synth). | Song |
| MIDI clock out | Optional 24 PPQN clock with Start/Stop, following the song tempo. | Song |
| MIDI in note entry | Notes arriving on USB MIDI act as key presses: audition and place at the cursor with the selected sound. | — |
| Edit existing notes | Hold EDIT and turn K2/K3: velocity/length of the selected sound's notes at the cursor. | Song |
| Backup and restore | fm1-base backup over USB (browser page or `fm1_backup.py`): all eight song slots, all or nothing. | — |

Defaults keep 0.1 songs sounding the same: tune 0, tone 127, decay 0, pan 0, MIDI OFF. SCF v1 files still load.

## Pages

EDIT tap cycles STAFF → MIX → SOUND → MIDI → STAFF; GLO toggles the song book; HOME returns to the staff.

| Page | K1 | K2 | K3 | K4 |
| --- | --- | --- | --- | --- |
| MIX | tempo | sound volume | layer volume | song end |
| SOUND | tune | tone | decay | pan |
| MIDI | sound's channel | MIDI mode | clock out | — |

## Next

1. **External clock sync**: follow MIDI clock/Start/Stop from USB; tempo display shows EXT.
2. **Sections and chains**: name up to 8 bar ranges and chain them into a song (needs SCF v3 with explicit
   migration; FORMAT.md forbids reusing reserved bytes).
3. **Playback animation**: the character hops across the staff during playback, as in Mario Paint.
4. **Sample banks**: replace the placeholder PCM with a permission-cleared expressive bank (tools/make_bank.py
   `--wav-dir`); loops and longer tails.
5. **Hardware voice limit**: measure 8/12/16 voices on a physical FM-1 and set the supported count.
6. **Screen mirror** (`mirror = true`): for demos and teaching; free in fm1-base.
