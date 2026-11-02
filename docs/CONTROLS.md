# Controls

Hardware/API source: pinned `base/include/plat.h` and `base/device/lcd.c`: 240×240 RGB565 screen, 27 chromatic keys F3–G5 at octave zero, seven detent encoders, fourteen named buttons, MASTER pot. This mapping has been exercised in the host simulator, not on physical hardware. Keys outside C2–C6 are rejected after octave shifting.

Every control acts on its own page only; a dash means nothing happens there. Refused edits during playback (knobs that change the song, BARS ops, undo/redo, SAVE, REC/SEQ/ARP on their pages) all say STOP FIRST.

**Same on every page**

| Control | Action |
| --- | --- |
| PRESET | Choose one of 31 sounds (wraps 31 ↔ 1, like the ribbon) |
| ALGO | Choose layer 1–8 (wraps) |
| PLAY | Play from beginning / stop. Hold EDIT and press PLAY: play from the start of the cursor's bar (MIDI clock out: Song Position Pointer + Continue instead of Start) |
| ENV / LFO | Undo / redo (stopped only) |
| SAVE | Save the song to the selected slot (stopped only; on the song book's DEMO entry: refused, PICK A SLOT) |
| GLO | Toggle the song book |
| HOME | Tap: return to the staff, cursor zero, cancel a pending load (acts on release). Hold 600 ms: help overlay for the page until released; no action on release |
| EDIT | Tap: cycle STAFF → MIX → SOUND → MIDI → GROOVE → BARS → STAFF (the song book is entered with GLO and left with GLO/HOME). The page changes only on a clean tap: any other button, key, knob or encoder while EDIT is held cancels it (EDIT+key erase, EDIT+K2/K3 note edit and EDIT+PLAY included) |
| OCT− + OCT+ | Together (second within 150 ms of the first; a first press's shift is undone): toggle FREE PLAY, on any page |
| MASTER | Smoothed audio level |
| MIDI in (USB, Bluetooth, TRS) | Note-on C2–C6 acts as that key (table below: STAFF auditions and places, with the selected sound, layer and the message's velocity; other pages audition only); in FREE PLAY note-off releases the note; other messages are ignored. Only the MIDI-in channel (MIDI page K4) is accepted for notes; clock and transport messages always are |

**Per page**

| Control | STAFF | MIX | SONG BOOK | SOUND | MIDI | GROOVE | BARS |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Note keys, MIDI-in notes | stopped: place at cursor (chords), audition; playing: audition + place at playhead (live); EDIT+key erases | audition only | audition only | audition only | audition only (MIDI out) | audition only | audition only |
| SELECT | cursor, one grid step | – | slot 1–8, then DEMO | – | – | – | FROM bar (TO follows if needed) |
| K1 | tempo 30–300 | beats per bar 2–12 | slot 1–8, then DEMO | tune −12..12 | sound's channel OFF/1–16 | swing 0–75 | FROM bar |
| K2 | new-note velocity; EDIT+K2: velocity of notes at cursor | sound level 0–127 | new-note velocity | tone | mode OFF/ON/MIDI ONLY | human 0–100 | TO bar |
| K3 | new-note length (24-tick steps, 16 on triplets); EDIT+K3: length at cursor | layer level 0–127 | new-note length | decay | clock OFF/OUT/IN | swing grid 8TH/16TH | TARGET bar |
| K4 | song end in quarter steps (tile: bars and beats, 4B+2 = 4 bars + 2 beats) | song end | – | pan L64..R63 | MIDI-in channel OMNI/1–16 | human mode TIME+VEL/TIME/VEL | transpose amount 1–12 |
| FX | mute/unmute the selected sound | mute/unmute the selected layer | mute sound | mute sound | mute sound | mute sound | MOVE range to TARGET |
| SEL | view filter: all sounds / selected sound | solo/unsolo the selected layer | view filter | view filter | view filter | view filter | view filter (copy/move/transpose: selected sound only) |
| REC | duplicate the cursor's bar into the next | – | arm load; again: load slot (DEMO: Twinkle Twinkle, then the staff) | – | – | – | COPY range to TARGET |
| SEQ | cycle grid 1/4, 1/8, 1/16, 1/8T, 1/16T (stopped only) | – | – | – | – | – | DELETE bars |
| ARP | toggle whole-song loop (stopped only) | – | – | – | – | – | INSERT bars |
| OCT− / OCT+ | shift keyboard by an octave; with EDIT held: spelling mode toward flat / sharp and respell the notes at the cursor (after the 150 ms chord window) | octave | octave | octave | octave | octave | OCT−: transpose DOWN, OCT+: UP, by K4 semitones (acts after the 150 ms chord window) |

K1–K4 changing the song are stopped-only; K2/K3 on the STAFF and the song book (new-note velocity and length) work during playback, MIX K2/K3 do not (STOP FIRST). Off the staff, EDIT held plus a knob does nothing. Keys, SELECT, PRESET, ALGO, FX and SEL work during playback.

**BARS ops**: FROM and TO are inclusive (TO ≥ FROM, up to the song end's last bar). REC copies the range to TARGET (existing notes kept, duplicates skipped, the end grows if needed); FX moves it; SEQ deletes the bars (later notes, song end and loop start shift left; a loop start inside the cut goes to its first bar); ARP inserts as many empty bars at FROM (shifting right); OCT−/OCT+ transpose down/up by K4. With SEL on, copy/move/transpose touch only the selected sound; delete/insert always take all sounds. An op that would exceed 384 notes, 65535 ticks, leave C2–C6 or change nothing is refused whole with a message; each is one undo step, stopped only. A single OCT press on BARS waits 150 ms for a second one (FREE PLAY) before it transposes.

**Sound mute (FX, not on MIX/BARS)** mutes the selected sound only in the engine, like the layer mask: its audio, auditions and MIDI out are silent, the level (volume) is untouched, it is not saved and clears at power-up. Muted sounds show a checkered stamp (ribbon, staff, MIDI page) and an M on the ribbon. FX again unmutes. On MIX, FX and SEL work on the selected layer instead (live, not saved; with any solo set only soloed layers sound); BARS keeps FX for MOVE.

**Groove and MIDI-in settings** are song data (undoable, saved, stopped-only): swing grid 8TH delays only off-beat 8ths (tick%96==48), 16TH only off-beat 16ths (tick%48==24); human mode TIME+VEL moves timing and velocity, TIME only timing, VEL only velocity (same random sequence each play). MIDI-in channel OMNI accepts every channel; 1–16 accepts that channel's notes only (MIDI note entry and FREE PLAY releases); realtime messages (clock, Start, Continue, Stop) are never filtered.

Clock IN (MIDI page K3): 0xFA Start plays from tick 0, 0xFB Continue from the playhead where playback last stopped (from 0 after the song end), 0xFC Stop stops; Start and Continue begin at the next 0xF8 clock. PLAY still starts and stops locally, at the last measured tempo and unlocked from the clocks. The tempo is the mean of the last 24 clock intervals; the header shows that BPM and EXT. The playhead is held within a few ticks (a quarter note is 96) of 4 ticks per clock, so it cannot drift. If clocks stop for over 500 ms playback continues at the last tempo and relocks when they return. IN sends no clock out. Clocks are read in the main loop (the platform queues MIDI without timestamps), so their stamps carry up to a frame of jitter, which the averaging and the phase lock absorb.

Live placement (playing, staff rules above): the note is auditioned (sound and MIDI out) and inserted with the selected sound, layer, velocity and new-note length at the playhead tick rounded to the nearest grid step (triplet grids too). At or past the song end it goes to the loop start when looping, else to the last step before the end. A note already there (same tick, pitch, sound, layer) is not added again; a full song says SONG FULL (the audition still sounds). All placements and erases of one playback run are one undo step (the first one checkpoints; ENV after stopping removes them all). A note snapped ahead of the playhead sounds again when the playhead reaches it. Knobs that change song settings, BARS operations, undo/redo and saving stay stopped-only.

FREE PLAY (OCT− + OCT+, runtime only, not saved): the header badge reads FREE (red) and the staff says FREE PLAY. Keys and MIDI-in notes (any page) only play the selected sound (polyphonic, with velocity, tune, tone, decay, pan and MIDI out on its channel), stopped or over the playing song, and never place or erase. The note is gated by the key: it is released (decay tail, MIDI note-off) when the key or MIDI note is released.

Layer and sound mute/solo are performance state: silent layers and sounds send no MIDI either (auditions too), they reset at power-up and are not saved. SWING delays the off-beats of the chosen grid (8TH: tick%96==48 by swing % of 48 ticks; 16TH: tick%48==24 by swing % of 24 ticks); HUMAN moves every note by a random ±(human × 8 / 100) ticks and its velocity by ±(human × 20 / 100), from a PRNG restarted at each playback start (same result every play); notes never move before the playhead or past the song end. Auditions are not swung.

BEATS is song data (undoable, saved, disabled during playback): bar lines, BAR n, REC bar copy and song-end bars follow it. During playback the selected sound's character hops along the staff at the playhead, landing on each step before a note starts. Screen mirror (fm1.toml `mirror = true`): the screen and panel can be mirrored to a computer over USB; the app repaints every frame, so a dropped part heals itself.

SOUND and MIDI settings are song data: undoable, saved with the song, and disabled during playback like the mixer. MIDI out goes to USB: note-on at each note with its velocity (tune applies), note-off after its length, all-notes-off on stop. Backup over USB holds the eight saved slots only; unsaved edits are not written for you. Editing notes and song settings is disabled during playback; navigation and sound selection remain available. Stop before saving because the platform pauses audio while writing flash. No one-press clear-song control exists. Autosave: after 3 s with no key or button press and no song change, stopped, the working song is written to flash (at most every 20 s, never while playing); SAVE writes it at once. Boot restores that working song with its slot and UNSAVED flag (message RESTORED), so power-off loses nothing; only if no valid autosave exists does boot load slot 1. A USB restore rewrites the working copy from the reloaded slot. Select other slots in song-book view. A load confirmation remains armed until slot/view changes or HOME; it is not timed.

**Spelling (flats)**: every black key can be spelled as a sharp or a flat. New notes use the spelling mode, SHARP / AUTO / FLAT (runtime only, not saved, AUTO at power-up; shown in the staff hint). AUTO spells D#, G#, A# as Eb, Ab, Bb and C#, F# as sharps. On the staff, EDIT + OCT− steps the mode toward flat (SHARP → AUTO → FLAT) and EDIT + OCT+ toward sharp; a single press waits 150 ms for the OCT− + OCT+ FREE PLAY chord, then acts, keeps the keyboard octave and cancels the EDIT page tap. Each press also respells the selected sound's black-key notes at the cursor (same match rule as EDIT+K2/K3: selected sound, current layer, cursor tick) to the new mode: one undo step, stopped only (the mode itself also changes while playing; press again at the end of the range to respell without changing mode). Keys, MIDI-in notes and live placement use the mode. BARS transpose: a note keeps its flat if it lands on a black key, a white-key landing clears it, a white key becoming black takes the mode. Spelling never changes sound; the duplicate rules ignore it.

Staff lines use treble-staff pitch spacing. The view changes octave bands for low/high notes; extreme pitches may be outside the visible band. Sharps mark chromatic pitches; a flat note sits on the line or space of the next letter up (Bb on the B position) with a flat sign, and the header name reads Bb3, Eb4. Dense chords with the same position can overlap; the SEL view filter shows one sound's notes. The display now includes character stamps, ledger lines, octave-band labels and off-band indicators.


Browser shortcuts: the side palette selects the native instrument; Load Twinkle demo creates the undoable 66-note Twinkle Twinkle arrangement (the song book's DEMO entry after slot 8: REC twice loads it, SAVE refuses it); Play song controls native transport. Both demo replacement and import require stopped playback. A nonempty song requires confirmation before replacement. Screen Reader can request a native page/status summary; full incremental narration is still incomplete.
