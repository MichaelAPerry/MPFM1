/* SPDX-License-Identifier: MIT */
#pragma once
#include <stdint.h>
#define SC_EVENTS 384
#define SC_VOICES 16
#define SC_PPQN 96
#define SC_MIDIQ 32 /* pending MIDI note-offs */
#define SC_PEND 32 /* delayed (swing/human) triggers waiting */
#define SC_BYTES (132+SC_EVENTS*8) /* v2: 48 head + 79 sound block + 1 pad + events + CRC */
typedef struct { uint16_t tick, duration; uint8_t pitch, instrument, velocity, layer; } ScNote;
typedef struct {
 uint16_t count, tempo, end, loop_start;
 uint8_t loop, beats, volume[15], layers[8], swing, human; /* swing 0..75: % of the half-beat (8th: 48 ticks, off-beat 16th: 24) the off-beat is late; human 0..100: timing +-8 ticks, velocity +-20 at 100 */
 int8_t tune[15], pan[15]; uint8_t tone[15], decay[15], midi_ch[15], midi_mode, midi_clock; /* tune -12..12, tone 127 open, decay 0 = short, pan -64..63, ch 0 off/1..16, mode 0 off/1 +internal/2 MIDI only, clock 0 off/1 out/2 in */
 ScNote notes[SC_EVENTS];
} ScSong;
typedef struct { uint32_t pos, inc, left, rel, age; float gain, last, tail, lp, k, gl, gr; uint8_t instrument, pitch, active, fade, relp; } ScVoice;
typedef struct {
 ScVoice voices[SC_VOICES]; uint32_t phase, tick, age, triggers, steals;
 uint16_t next; uint8_t playing, limit, mute, solo, cont, npd; float master; /* mute/solo: layer bit masks (performance, not in the song); cont: next clock Start is SPP+Continue */
 uint32_t rng; struct { uint16_t at; ScNote n; } pd[SC_PEND]; /* humanize PRNG (seeded per start); triggers due at tick `at` */
 void (*midi)(uint32_t pkt); /* NULL = no MIDI; set after sc_engine_init. USB-MIDI packet: cin | status<<8 | d1<<16 | d2<<24; called from sc_note/sc_render/sc_stop */
 uint32_t frames, due, clkc; uint16_t chused, np; uint8_t clk; /* frame clock, next note-off due, tick count, channels used, pending count, clock running */
 struct { uint32_t due; uint8_t ch, note; } pend[SC_MIDIQ];
 uint32_t xr[25], xn, xlast; int32_t xt; float xinc; uint8_t xl, xw; /* ext clock: last 25 stamps (us), count, frame of last clock, target tick (4/clock), measured tick rate (tempo*96, 0 = none), locked, waiting for 1st clock */
} ScEngine;
extern const char *const sc_names[15];
void sc_default(ScSong *s);
int sc_valid(const ScSong *s);
int sc_add(ScSong *s, ScNote n);
int sc_find(const ScSong *s, ScNote n); /* index of the note with the same tick, pitch, sound and layer, or -1 */
/* While playing (call under the render's lock): add/remove one note and keep the engine's scan index right. insert: 1 added, SC_E_NONE already there, SC_E_FULL, 0 invalid; a note due at or before the playhead is not triggered by the scan (audition it with sc_note). remove: 1 removed. */
int sc_insert(ScEngine *e, ScSong *s, ScNote n);
int sc_remove(ScEngine *e, ScSong *s, ScNote n);
int sc_duplicate(ScSong *s, uint16_t start, uint16_t length);
/* Range ops on ticks [from,from+len), inst < 0: all sounds. 1 = done, else SC_E_* with the song unchanged.
 * copy/move paste at `to` keeping notes already there (same tick, pitch, sound, layer = skipped) and extend end to cover it; move removes the source first.
 * delete removes the range (end and later notes shift left; loop_start after it shifts, inside it goes to `from`); insert opens empty space (end, later notes and loop_start shift right); both take all sounds. */
enum { SC_E_ARG=-1, SC_E_FULL=-2, SC_E_TICK=-3, SC_E_PITCH=-4, SC_E_NONE=-5 }; /* bad range, capacity, >65535 ticks, outside C2-C6, nothing to do (no notes in range, or all pasted notes already there) */
int sc_copy(ScSong *s, unsigned from, unsigned len, unsigned to, int inst);
int sc_move(ScSong *s, unsigned from, unsigned len, unsigned to, int inst);
int sc_delete_bars(ScSong *s, unsigned from, unsigned len);
int sc_insert_bars(ScSong *s, unsigned from, unsigned len);
int sc_transpose(ScSong *s, unsigned from, unsigned len, int semis, int inst);
unsigned sc_encode(const ScSong *s, uint8_t *out);
int sc_decode(ScSong *s, const uint8_t *in, unsigned len);
void sc_engine_init(ScEngine *e, unsigned voices);
void sc_start_at(ScEngine *e, const ScSong *s, unsigned tick); /* play from tick (>= end: 0); with midi_clock out: SPP (24 ticks/unit) + Continue instead of Start */
void sc_start(ScEngine *e); /* MIDI Start (0xFA) goes out with the first sc_render after this, if midi_clock */
void sc_stop(ScEngine *e); /* pending note-offs, CC123 on used channels, 0xFC if clock ran */
void sc_note(ScEngine *e, const ScSong *s, ScNote n);
void sc_release(ScEngine *e, const ScSong *s, unsigned pitch, unsigned instrument); /* gate: end voices of that pitch+sound now (decay applies) and send their MIDI note-off; for sc_note with a long duration */
void sc_render(ScEngine *e, const ScSong *s, int32_t *lr, unsigned n, unsigned master);
/* External clock (song midi_clock 2; the caller guards). Call under the render's lock. us: any monotonic microsecond count, stamped as near the arrival as the platform allows.
 * sc_ext_clock: 0xF8. Tempo = mean of the last 24 intervals; once locked (sc_ext_start) the render holds the playhead within a few ticks of 4 per clock. sc_stop is 0xFC. */
void sc_ext_clock(ScEngine *e, const ScSong *s, uint32_t us);
void sc_ext_start(ScEngine *e, const ScSong *s, int cont); /* 0xFA from tick 0 (cont 0) or 0xFB from the playhead (cont 1); playback begins at the next clock */
