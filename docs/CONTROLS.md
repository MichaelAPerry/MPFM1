# Controls

Hardware/API source: pinned `base/include/plat.h` and `base/device/lcd.c`: 240×240 RGB565 screen, 27 chromatic keys F3–G5 at octave zero, seven detent encoders, fourteen named buttons, MASTER pot. This mapping has been exercised in the host simulator, not on physical hardware. Keys outside C2–C6 are rejected after octave shifting.

| Control | Action |
| --- | --- |
| Note keys | Insert/audition at cursor (stopped only; while playing the message is STOP TO EDIT and nothing sounds); multiple presses form a chord; cursor stays until moved |
| SELECT encoder | Move time cursor one selected grid step (1/4, 1/8, 1/16, 1/8T, 1/16T) |
| PRESET | Choose one of fifteen sounds |
| ALGO | Choose layer 1–8 |
| K1 | Tempo 30–300 BPM (stopped only); MIX page: BEATS per bar 2–12 (time signature n/4, default 4); song book: save slot; SOUND page: tune −12..+12; MIDI page: the sound's channel (OFF, 1–16) |
| K2 | New-note velocity; MIX page: selected instrument volume (0 mutes); SOUND: tone 0..127; MIDI: mode OFF / ON / MIDI ONLY. Hold EDIT on the staff: velocity of the selected sound's notes at the cursor |
| K3 | New-note duration in 24-tick steps (16 on the triplet grids); MIX page: layer volume (0 mutes); SOUND: decay 0..127; MIDI: clock OFF / OUT / IN (follow external clock; below). Hold EDIT on the staff: length (24-tick steps) of the notes at the cursor |
| K4 | Song end in quarter-note steps; cannot cut off existing event starts; SOUND page: pan L64..R63 |
| PLAY | Play from beginning / stop. Hold EDIT and press PLAY: play from the start of the cursor's bar (MIDI clock out: Song Position Pointer + Continue instead of Start) |
| OCT− / OCT+ | Shift keyboard by octave |
| SEQ | (stopped only) Cycle grid: 1/4 (96 ticks), 1/8 (48), 1/16 (24), 1/8T (32), 1/16T (16 ticks, triplets) |
| ARP | Toggle whole-song looping (an undoable song edit, stopped only) |
| FX | Mute/unmute selected sound (unmute restores level 127; stopped only). MIX page: mute/unmute the selected layer (live, also while playing, not saved) |
| ENV / LFO | Undo / redo (stopped only; during playback the message is STOP FIRST) |
| REC | Duplicate cursor's whole bar into next bar, preserving existing notes; capacity checked |
| EDIT | Tap to cycle STAFF → MIX → SOUND → MIDI → GROOVE → BARS → STAFF; hold with a key to erase matching pitch/sound/layer at cursor; hold and turn K2/K3 to edit the notes at the cursor (one undo step per hold) |
| SEL | Show all sounds / only selected sound. MIX page: solo/unsolo the selected layer (live, not saved; with any solo set, only soloed layers sound) |
| GLO | Toggle song-book view |
| SAVE | Save current song to selected slot; rejected during playback |
| REC in song-book view | First press arms load; second loads that slot. Load can be undone |
| HOME | Tap: return to staff, cursor zero; cancel pending load (acts on release). Hold 600 ms: help overlay listing this page's controls until released; no action on release |
| GROOVE page | K1 swing 0–75, K2 human 0–100 (song data: undoable, saved, disabled during playback) |
| BARS page | K1 FROM bar, K2 TO bar (inclusive, TO ≥ FROM, up to the song end's last bar), K3 TARGET bar, K4 transpose −12..+12. REC copies the range to TARGET (existing notes kept, duplicates skipped; end grows if needed), FX moves it, SEQ deletes the bars (later notes, song end and loop start shift left; a loop start inside the cut goes to its first bar), ARP inserts as many empty bars at FROM (shifting right; a loop start at or after FROM shifts), OCT+ transposes by K4. With SEL solo view on, copy/move/transpose touch only the selected sound; delete/insert always take all sounds. An op that would exceed 384 notes, 65535 ticks, leave C2–C6 or change nothing is refused whole with a message; each is one undo step, disabled during playback |
| MASTER | Smoothed audio level |
| MIDI in (USB, Bluetooth, TRS) | Note-on C2–C6 acts as that key: audition and place at the cursor with the selected sound, layer and the message's velocity (erase and playback rules apply); note-off and other messages are ignored |

Clock IN (MIDI page K3): 0xFA Start plays from tick 0, 0xFB Continue from the playhead where playback last stopped (from 0 after the song end), 0xFC Stop stops; Start and Continue begin at the next 0xF8 clock. PLAY still starts and stops locally, at the last measured tempo and unlocked from the clocks. The tempo is the mean of the last 24 clock intervals; the header shows that BPM and EXT. The playhead is held within a few ticks (a quarter note is 96) of 4 ticks per clock, so it cannot drift. If clocks stop for over 500 ms playback continues at the last tempo and relocks when they return. IN sends no clock out. Clocks are read in the main loop (the platform queues MIDI without timestamps), so their stamps carry up to a frame of jitter, which the averaging and the phase lock absorb.

Mute/solo are performance state: silent layers send no MIDI either (auditions on a silenced layer too), they reset at power-up and are not saved. SWING delays off-beat 8ths (tick%96==48) by swing % of 48 ticks and off-beat 16ths (tick%48==24) by swing % of 24 ticks; HUMAN moves every note by a random ±(human × 8 / 100) ticks and its velocity by ±(human × 20 / 100), from a PRNG restarted at each playback start (same result every play); notes never move before the playhead or past the song end. Auditions are not swung.

BEATS is song data (undoable, saved, disabled during playback): bar lines, BAR n, REC bar copy and song-end bars follow it. During playback the selected sound's character hops along the staff at the playhead, landing on each step before a note starts. Screen mirror (fm1.toml `mirror = true`): the screen and panel can be mirrored to a computer over USB; the app repaints every frame, so a dropped part heals itself.

SOUND and MIDI settings are song data: undoable, saved with the song, and disabled during playback like the mixer. MIDI out goes to USB: note-on at each note with its velocity (tune applies), note-off after its length, all-notes-off on stop. Backup over USB holds the eight saved slots only; unsaved edits are not written for you. Editing notes and song settings is disabled during playback; navigation and sound selection remain available. Stop before saving because the platform pauses audio while writing flash. No one-press clear-song control exists. Autosave: after 3 s with no key or button press and no song change, stopped, the working song is written to flash (at most every 20 s, never while playing); SAVE writes it at once. Boot restores that working song with its slot and UNSAVED flag (message RESTORED), so power-off loses nothing; only if no valid autosave exists does boot load slot 1. A USB restore rewrites the working copy from the reloaded slot. Select other slots in song-book view. A load confirmation remains armed until slot/view changes or HOME; it is not timed.

Staff lines use treble-staff pitch spacing. The view changes octave bands for low/high notes; extreme pitches may be outside the visible band. Sharps mark chromatic pitches. Dense chords with the same position can overlap; filtering helps, but individual selection still needs refinement. The display now includes character stamps, ledger lines, octave-band labels and off-band indicators.


Browser shortcuts: the side palette selects the native instrument; Load Playground demo creates an undoable 32-note sketch; Play song controls native transport. Both demo replacement and import require stopped playback. A nonempty song requires confirmation before replacement. Screen Reader can request a native page/status summary; full incremental narration is still incomplete.
