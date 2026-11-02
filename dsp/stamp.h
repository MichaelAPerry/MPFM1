/* SPDX-License-Identifier: MIT */
#pragma once
#include <stdint.h>
#define SC_EVENTS 384
#define SC_VOICES 16
#define SC_PPQN 96
#define SC_MIDIQ 32 /* pending MIDI note-offs */
#define SC_BYTES (130+SC_EVENTS*8) /* v2: 48 head + 77 sound block + 1 pad + events + CRC */
typedef struct { uint16_t tick, duration; uint8_t pitch, instrument, velocity, layer; } ScNote;
typedef struct {
 uint16_t count, tempo, end, loop_start;
 uint8_t loop, beats, volume[15], layers[8];
 int8_t tune[15], pan[15]; uint8_t tone[15], decay[15], midi_ch[15], midi_mode, midi_clock; /* tune -12..12, tone 127 open, decay 0 = short, pan -64..63, ch 0 off/1..16, mode 0 off/1 +internal/2 MIDI only, clock 0 off/1 out/2 in */
 ScNote notes[SC_EVENTS];
} ScSong;
typedef struct { uint32_t pos, inc, left, rel, age; float gain, last, tail, lp, k, gl, gr; uint8_t instrument, active, fade, relp; } ScVoice;
typedef struct {
 ScVoice voices[SC_VOICES]; uint32_t phase, tick, age, triggers, steals;
 uint16_t next; uint8_t playing, limit; float master;
 void (*midi)(uint32_t pkt); /* NULL = no MIDI; set after sc_engine_init. USB-MIDI packet: cin | status<<8 | d1<<16 | d2<<24; called from sc_note/sc_render/sc_stop */
 uint32_t frames, due, clkc; uint16_t chused, np; uint8_t clk; /* frame clock, next note-off due, tick count, channels used, pending count, clock running */
 struct { uint32_t due; uint8_t ch, note; } pend[SC_MIDIQ];
 uint32_t xr[25], xn, xlast; int32_t xt; float xinc; uint8_t xl, xw; /* ext clock: last 25 stamps (us), count, frame of last clock, target tick (4/clock), measured tick rate (tempo*96, 0 = none), locked, waiting for 1st clock */
} ScEngine;
extern const char *const sc_names[15];
void sc_default(ScSong *s);
int sc_valid(const ScSong *s);
int sc_add(ScSong *s, ScNote n);
int sc_duplicate(ScSong *s, uint16_t start, uint16_t length);
unsigned sc_encode(const ScSong *s, uint8_t *out);
int sc_decode(ScSong *s, const uint8_t *in, unsigned len);
void sc_engine_init(ScEngine *e, unsigned voices);
void sc_start(ScEngine *e); /* MIDI Start (0xFA) goes out with the first sc_render after this, if midi_clock */
void sc_stop(ScEngine *e); /* pending note-offs, CC123 on used channels, 0xFC if clock ran */
void sc_note(ScEngine *e, const ScSong *s, ScNote n);
void sc_render(ScEngine *e, const ScSong *s, int32_t *lr, unsigned n, unsigned master);
/* External clock (song midi_clock 2; the caller guards). Call under the render's lock. us: any monotonic microsecond count, stamped as near the arrival as the platform allows.
 * sc_ext_clock: 0xF8. Tempo = mean of the last 24 intervals; once locked (sc_ext_start) the render holds the playhead within a few ticks of 4 per clock. sc_stop is 0xFC. */
void sc_ext_clock(ScEngine *e, const ScSong *s, uint32_t us);
void sc_ext_start(ScEngine *e, const ScSong *s, int cont); /* 0xFA from tick 0 (cont 0) or 0xFB from the playhead (cont 1); playback begins at the next clock */
