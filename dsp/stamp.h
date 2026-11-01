/* SPDX-License-Identifier: MIT */
#pragma once
#include <stdint.h>
#define SC_EVENTS 384
#define SC_VOICES 16
#define SC_PPQN 96
#define SC_BYTES (48+SC_EVENTS*8+4)
typedef struct { uint16_t tick, duration; uint8_t pitch, instrument, velocity, layer; } ScNote;
typedef struct {
 uint16_t count, tempo, end, loop_start;
 uint8_t loop, beats, volume[15], layers[8];
 ScNote notes[SC_EVENTS];
} ScSong;
typedef struct { uint32_t pos, inc, left, age; float gain, last, tail; uint8_t instrument, active, fade; } ScVoice;
typedef struct {
 ScVoice voices[SC_VOICES]; uint32_t phase, tick, age, triggers, steals;
 uint16_t next; uint8_t playing, limit; float master;
} ScEngine;
extern const char *const sc_names[15];
void sc_default(ScSong *s);
int sc_valid(const ScSong *s);
int sc_add(ScSong *s, ScNote n);
int sc_duplicate(ScSong *s, uint16_t start, uint16_t length);
unsigned sc_encode(const ScSong *s, uint8_t *out);
int sc_decode(ScSong *s, const uint8_t *in, unsigned len);
void sc_engine_init(ScEngine *e, unsigned voices);
void sc_start(ScEngine *e);
void sc_stop(ScEngine *e);
void sc_note(ScEngine *e, const ScSong *s, ScNote n);
void sc_render(ScEngine *e, const ScSong *s, int32_t *lr, unsigned n, unsigned master);
