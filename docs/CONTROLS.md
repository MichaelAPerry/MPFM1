# Controls

Hardware/API source: pinned `base/include/plat.h` and `base/device/lcd.c`: 240×240 RGB565 screen, 27 chromatic keys F3–G5 at octave zero, seven detent encoders, fourteen named buttons, MASTER pot. This mapping has been exercised in the host simulator, not on physical hardware. Keys outside C2–C6 are rejected after octave shifting.

| Control | Action |
| --- | --- |
| Note keys | Insert/audition at cursor; multiple presses form a chord; cursor stays until moved |
| SELECT encoder | Move time cursor one selected grid step |
| PRESET | Choose one of fifteen sounds |
| ALGO | Choose layer 1–8 |
| K1 | Tempo 30–300 BPM; in FILE view, choose save slot |
| K2 | New-note velocity; in EDIT view, selected instrument volume (0 mutes) |
| K3 | New-note duration in 24-tick steps; in EDIT view, layer volume (0 mutes) |
| K4 | Song end in quarter-note steps; cannot cut off existing event starts |
| PLAY | Play from beginning / stop |
| OCT− / OCT+ | Shift keyboard by octave |
| SEQ | Cycle quarter/eighth/sixteenth grid |
| ARP | Toggle whole-song looping |
| FX | Mute/unmute selected sound (unmute restores level 127) |
| ENV / LFO | Undo / redo |
| REC | Duplicate cursor's whole bar into next bar, preserving existing notes; capacity checked |
| EDIT | Toggle secondary edit view; hold with a key to erase matching pitch/sound/layer at cursor |
| SEL | Show all sounds / only selected sound |
| GLO | Toggle FILE view |
| SAVE | Save current song to selected slot; rejected during playback |
| REC in FILE view | First press arms load; second loads that slot. Load can be undone |
| HOME | Return to staff, cursor zero; cancel pending load |
| MASTER | Smoothed audio level |

Editing notes and song settings is disabled during playback; navigation and sound selection remain available. Stop before saving because the platform pauses audio while writing flash. No one-press clear-song control exists. Boot loads slot 1; select other slots in FILE view. A load confirmation remains armed until slot/view changes or HOME; it is not timed.

Staff lines use treble-staff pitch spacing. The view changes octave bands for low/high notes; extreme pitches may be outside the visible band. Sharps mark chromatic pitches. Dense chords with the same position can overlap; filtering helps, but per-note selection, richer icons, ledger lines and octave-band labels still need refinement.
