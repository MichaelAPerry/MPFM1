# Controls

Hardware/API source: pinned `base/include/plat.h` and `base/device/lcd.c`: 240×240 RGB565 screen, 27 chromatic keys F3–G5 at octave zero, seven detent encoders, fourteen named buttons, MASTER pot. This mapping has been exercised in the host simulator, not on physical hardware. Keys outside C2–C6 are rejected after octave shifting.

| Control | Action |
| --- | --- |
| Note keys | Insert/audition at cursor; multiple presses form a chord; cursor stays until moved |
| SELECT encoder | Move time cursor one selected grid step |
| PRESET | Choose one of fifteen sounds |
| ALGO | Choose layer 1–8 |
| K1 | Tempo 30–300 BPM; song book: save slot; SOUND page: tune −12..+12; MIDI page: the sound's channel (OFF, 1–16) |
| K2 | New-note velocity; MIX page: selected instrument volume (0 mutes); SOUND: tone 0..127; MIDI: mode OFF / ON / MIDI ONLY. Hold EDIT on the staff: velocity of the selected sound's notes at the cursor |
| K3 | New-note duration in 24-tick steps; MIX page: layer volume (0 mutes); SOUND: decay 0..127; MIDI: clock out OFF/ON. Hold EDIT on the staff: length (24-tick steps) of the notes at the cursor |
| K4 | Song end in quarter-note steps; cannot cut off existing event starts; SOUND page: pan L64..R63 |
| PLAY | Play from beginning / stop |
| OCT− / OCT+ | Shift keyboard by octave |
| SEQ | Cycle quarter/eighth/sixteenth grid |
| ARP | Toggle whole-song looping |
| FX | Mute/unmute selected sound (unmute restores level 127) |
| ENV / LFO | Undo / redo |
| REC | Duplicate cursor's whole bar into next bar, preserving existing notes; capacity checked |
| EDIT | Tap to cycle STAFF → MIX → SOUND → MIDI → STAFF; hold with a key to erase matching pitch/sound/layer at cursor; hold and turn K2/K3 to edit the notes at the cursor (one undo step per hold) |
| SEL | Show all sounds / only selected sound |
| GLO | Toggle song-book view |
| SAVE | Save current song to selected slot; rejected during playback |
| REC in song-book view | First press arms load; second loads that slot. Load can be undone |
| HOME | Return to staff, cursor zero; cancel pending load |
| MASTER | Smoothed audio level |
| MIDI in (USB) | Note-on C2–C6 acts as that key: audition and place at the cursor with the selected sound, layer and the message's velocity (erase and playback rules apply); note-off and other messages are ignored |

SOUND and MIDI settings are song data: undoable, saved with the song, and disabled during playback like the mixer. MIDI out goes to USB: note-on at each note with its velocity (tune applies), note-off after its length, all-notes-off on stop. Backup over USB holds the eight saved slots only; unsaved edits are not written for you. Editing notes and song settings is disabled during playback; navigation and sound selection remain available. Stop before saving because the platform pauses audio while writing flash. No one-press clear-song control exists. Boot loads slot 1; select other slots in song-book view. A load confirmation remains armed until slot/view changes or HOME; it is not timed.

Staff lines use treble-staff pitch spacing. The view changes octave bands for low/high notes; extreme pitches may be outside the visible band. Sharps mark chromatic pitches. Dense chords with the same position can overlap; filtering helps, but individual selection still needs refinement. The display now includes character stamps, ledger lines, octave-band labels and off-band indicators.


Browser shortcuts: the side palette selects the native instrument; Load Playground demo creates an undoable 32-note sketch; Play song controls native transport. Both demo replacement and import require stopped playback. A nonempty song requires confirmation before replacement. Screen Reader can request a native page/status summary; full incremental narration is still incomplete.
