/* SPDX-License-Identifier: MIT */
#include "stamp.h"
#include "libc_shim.h"
#include "bank.h"
const char *const sc_names[15]={"PIANO","KICK","ZIP","BELL","TRUMPET","PULSE","DOG","CAT","PIG","HORN","BABY","GUITAR","BLOCK","ORGAN","BASS"};
void sc_default(ScSong *s) { memset(s,0,sizeof *s); s->tempo=120; s->end=1536; s->beats=4; memset(s->volume,127,15); memset(s->layers,127,8); }
static int valid_note(ScNote n, unsigned end) { return n.tick<end && n.duration && n.pitch>=36 && n.pitch<=84 && n.instrument<15 && n.velocity<=127 && n.layer<8; }
int sc_valid(const ScSong *s) {
 unsigned i; if(s->count>SC_EVENTS || s->tempo<30 || s->tempo>300 || !s->end || s->loop_start>=s->end || s->loop>1 || s->beats<1 || s->beats>16) return 0;
 for(i=0;i<15;i++) if(s->volume[i]>127) return 0;
 for(i=0;i<8;i++) if(s->layers[i]>127) return 0;
 for(i=0;i<s->count;i++) if(!valid_note(s->notes[i],s->end) || (i && s->notes[i].tick<s->notes[i-1].tick)) return 0;
 return 1;
}
int sc_add(ScSong *s, ScNote n) {
 unsigned at=s->count; if(at==SC_EVENTS || !valid_note(n,s->end)) return 0;
 while(at && s->notes[at-1].tick>n.tick) { s->notes[at]=s->notes[at-1]; at--; }
 s->notes[at]=n; s->count++; return 1;
}
int sc_duplicate(ScSong *s, uint16_t start, uint16_t length) {
 ScNote copy[SC_EVENTS]; unsigned n=0,i,end=(unsigned)start+2u*length;
 if(!length || end>65535) return 0;
 for(i=0;i<s->count;i++) if(s->notes[i].tick>=start && s->notes[i].tick<(unsigned)start+length) copy[n++]=s->notes[i];
 if(!n || n+s->count>SC_EVENTS) return 0;
 if(end>s->end) s->end=(uint16_t)end;
 for(i=0;i<n;i++) { copy[i].tick+=length; sc_add(s,copy[i]); } return 1;
}
static void put16(uint8_t *p,unsigned v) { p[0]=v; p[1]=v>>8; }
static unsigned get16(const uint8_t *p) { return p[0]|((unsigned)p[1]<<8); }
static uint32_t crc(const uint8_t *p,unsigned n) { uint32_t c=~0u; unsigned j; while(n--) { c^=*p++; for(j=0;j<8;j++) c=(c>>1)^((0u-(c&1))&0xedb88320u); } return ~c; }
unsigned sc_encode(const ScSong *s,uint8_t *out) {
 unsigned i,n=48+8*s->count; uint32_t c; if(!sc_valid(s)) return 0;
 memset(out,0,n+4); memcpy(out,"SCF1",4); put16(out+4,1); put16(out+6,s->count); put16(out+8,s->tempo); put16(out+10,s->end); put16(out+12,s->loop_start); out[14]=s->loop; out[15]=s->beats;
 memcpy(out+16,s->volume,15); memcpy(out+31,s->layers,8);
 for(i=0;i<s->count;i++){ uint8_t *p=out+48+8*i; ScNote a=s->notes[i]; put16(p,a.tick);put16(p+2,a.duration);p[4]=a.pitch;p[5]=a.instrument;p[6]=a.velocity;p[7]=a.layer; }
 c=crc(out,n); for(i=0;i<4;i++) out[n+i]=c>>(8*i); return n+4;
}
int sc_decode(ScSong *s,const uint8_t *in,unsigned len) {
 ScSong t; unsigned i,n; uint32_t c=0; if(len<52 || memcmp(in,"SCF1",4) || get16(in+4)!=1) return 0;
 n=get16(in+6); if(n>SC_EVENTS || len!=52+8*n) return 0;
 for(i=0;i<4;i++) c|=(uint32_t)in[len-4+i]<<(8*i); if(c!=crc(in,len-4)) return 0;
 for(i=39;i<48;i++) if(in[i]) return 0;
 memset(&t,0,sizeof t);t.count=n;t.tempo=get16(in+8);t.end=get16(in+10);t.loop_start=get16(in+12);t.loop=in[14];t.beats=in[15];memcpy(t.volume,in+16,15);memcpy(t.layers,in+31,8);
 for(i=0;i<n;i++){const uint8_t *p=in+48+8*i;t.notes[i]=(ScNote){get16(p),get16(p+2),p[4],p[5],p[6],p[7]};}
 if(!sc_valid(&t)) return 0; *s=t; return 1;
}
void sc_engine_init(ScEngine *e,unsigned voices){memset(e,0,sizeof *e);e->limit=voices>=1 && voices<=16?voices:8;}
void sc_stop(ScEngine *e){unsigned i;e->playing=0;for(i=0;i<e->limit;i++) if(e->voices[i].left>128)e->voices[i].left=128;}
void sc_start(ScEngine *e){sc_stop(e);e->tick=0;e->phase=0;e->next=0;e->playing=1;}
void sc_note(ScEngine *e,const ScSong *s,ScNote n){
 unsigned i,best=0;ScVoice *v;uint32_t oldest=0;
 if(!valid_note(n,65535) || !n.velocity || !s->volume[n.instrument] || !s->layers[n.layer])return;
 for(i=0;i<e->limit;i++){if(!e->voices[i].active){best=i;break;}if(e->age-e->voices[i].age>=oldest){oldest=e->age-e->voices[i].age;best=i;}}
 v=&e->voices[best]; if(v->active)e->steals++;
 v->tail=v->last;v->fade=64;v->pos=0;v->inc=sc_increments[n.pitch-36];v->instrument=n.instrument;v->active=1;v->age=++e->age;
 v->left=(uint32_t)n.duration*55125u/(s->tempo*2u); /* =duration*60*44100/(tempo*96), 32-bit exact: no 64-bit multiply (bsp docs/CPU.md) */
 v->gain=(float)n.velocity*s->volume[n.instrument]*s->layers[n.layer]/(127.0f*127.0f*127.0f)*0.055f;e->triggers++;
}
void sc_render(ScEngine *e,const ScSong *s,int32_t *lr,unsigned n,unsigned master){
 unsigned f,i; const uint32_t threshold=60u*44100;
 for(f=0;f<n;f++){
  float mix=0;
  if(e->playing){while(e->next<s->count && s->notes[e->next].tick<=e->tick)sc_note(e,s,s->notes[e->next++]);}
  for(i=0;i<e->limit;i++){
   ScVoice *v=&e->voices[i];float sample=0;
   if(v->active){unsigned at=v->pos>>16;
    if(at+1>=SC_SAMPLE_LEN || !v->left){v->active=0;v->tail=v->last;v->fade=64;}
    else {float frac=(v->pos&65535)/65536.0f;sample=(sc_bank[v->instrument][at]*(1-frac)+sc_bank[v->instrument][at+1]*frac)/32768.0f*v->gain;
     if(v->left<128)sample*=v->left/128.0f;
     if(v->fade)sample*=(64-v->fade)/64.0f;
     v->left--;v->pos+=v->inc;
    }
   }
   if(v->fade){sample+=v->tail*v->fade/64.0f;v->fade--;}
   v->last=sample;mix+=sample;
  }
  e->master+=((float)master/4096-e->master)*0.004f;
  if(mix>1)mix=1;if(mix< -1)mix=-1;
  lr[2*f]=lr[2*f+1]=(int32_t)(mix*e->master*8388607);
  if(e->playing){e->phase+=s->tempo*96u;if(e->phase>=threshold){e->phase-=threshold;e->tick++;
   if(e->tick>=s->end){if(s->loop){e->tick=s->loop_start;e->next=0;while(e->next<s->count && s->notes[e->next].tick<e->tick)e->next++;}else sc_stop(e);}
  }}
 }
}
