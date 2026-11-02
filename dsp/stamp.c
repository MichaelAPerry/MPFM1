/* SPDX-License-Identifier: MIT */
#include "stamp.h"
#include "libc_shim.h"
#include "bank.h"
const char *const sc_names[15]={"PIANO","KICK","ZIP","BELL","TRUMPET","PULSE","DOG","CAT","PIG","HORN","BABY","GUITAR","BLOCK","ORGAN","BASS"};
void sc_default(ScSong *s) { memset(s,0,sizeof *s); s->tempo=120; s->end=1536; s->beats=4; memset(s->volume,127,15); memset(s->layers,127,8); memset(s->tone,127,15); }
int sc_black(unsigned p) { return 0x54A>>p%12&1; }
int sc_spell(unsigned p, unsigned m) { return sc_black(p) && (m==2 || (m==1 && 0x508>>p%12&1)) ? SC_FLAT : 0; }
static int valid_note(ScNote n, unsigned end) { return (!(n.layer&SC_FLAT) || sc_black(n.pitch)) && n.tick<end && n.duration && n.pitch>=36 && n.pitch<=84 && n.instrument<15 && n.velocity<=127 && (n.layer&127)<8; }
static unsigned hj(const ScSong *s){return s->hmode==2?0:s->human*8u/100;} /* human timing range, ticks */
int sc_valid(const ScSong *s) {
 unsigned i; if(s->count>SC_EVENTS || s->tempo<30 || s->tempo>300 || !s->end || s->loop_start>=s->end || s->loop>1 || s->beats<1 || s->beats>16) return 0;
 for(i=0;i<15;i++) if(s->volume[i]>127) return 0;
 for(i=0;i<8;i++) if(s->layers[i]>127) return 0;
 for(i=0;i<15;i++) if(s->tune[i]<-12 || s->tune[i]>12 || s->tone[i]>127 || s->decay[i]>127 || s->pan[i]<-64 || s->pan[i]>63 || s->midi_ch[i]>16) return 0;
 if(s->midi_mode>2 || s->midi_clock>2 || s->swing>75 || s->human>100 || s->sgrid>1 || s->hmode>2 || s->midi_in>16) return 0;
 for(i=0;i<s->count;i++) if(!valid_note(s->notes[i],s->end) || (i && s->notes[i].tick<s->notes[i-1].tick)) return 0;
 return 1;
}
int sc_add(ScSong *s, ScNote n) {
 unsigned at=s->count; if(at==SC_EVENTS || !valid_note(n,s->end)) return 0;
 while(at && s->notes[at-1].tick>n.tick) { s->notes[at]=s->notes[at-1]; at--; }
 s->notes[at]=n; s->count++; return 1;
}
/* edits while playing: the render scan index e->next stays on the same note (a note at or before the scan point is never triggered by the scan) */
int sc_insert(ScEngine *e,ScSong *s,ScNote n) {
 unsigned at=0; if(sc_find(s,n)>=0) return SC_E_NONE; if(s->count==SC_EVENTS) return SC_E_FULL;
 while(at<s->count && s->notes[at].tick<=n.tick) at++;
 if(!sc_add(s,n)) return 0;
 if(at<e->next || (at==e->next && n.tick<=e->tick+hj(s))) e->next++;
 return 1;
}
int sc_remove(ScEngine *e,ScSong *s,ScNote n) {
 int i=sc_find(s,n); if(i<0) return 0;
 memmove(s->notes+i,s->notes+i+1,(s->count---i-1)*sizeof(ScNote)); if((unsigned)i<e->next) e->next--; return 1;
}
int sc_duplicate(ScSong *s, uint16_t start, uint16_t length) {
 ScNote copy[SC_EVENTS]; unsigned n=0,i,end=(unsigned)start+2u*length;
 if(!length || end>65535) return 0;
 for(i=0;i<s->count;i++) if(s->notes[i].tick>=start && s->notes[i].tick<(unsigned)start+length) copy[n++]=s->notes[i];
 if(!n || n+s->count>SC_EVENTS) return 0;
 if(end>s->end) s->end=(uint16_t)end;
 for(i=0;i<n;i++) { copy[i].tick+=length; sc_add(s,copy[i]); } return 1;
}
/* Range ops on [from,from+len) ticks; inst < 0: all sounds. Return 1, or SC_E_*: checked first, a refusal leaves the song untouched. */
static ScNote tmp[SC_EVENTS]; /* scratch for copy/move (not reentrant) */
static int hit(const ScNote *n,unsigned from,unsigned len,int inst) { return n->tick>=from && n->tick-from<len && (inst<0 || n->instrument==inst); }
int sc_find(const ScSong *s,ScNote n) { unsigned i; for(i=0;i<s->count;i++) if(s->notes[i].tick==n.tick && s->notes[i].pitch==n.pitch && s->notes[i].instrument==n.instrument && !((s->notes[i].layer^n.layer)&127)) return (int)i; return -1; }
#define has(s,n) (sc_find(s,n)>=0)
/* copy (mv 0) or move (mv 1) the range's notes to `to`, skipping ones already there; end grows to cover the target */
static int paste(ScSong *s,unsigned from,unsigned len,unsigned to,int inst,int mv) {
 unsigned i,n=0,nd=0;
 if(!len || to==from) return SC_E_ARG; if(to+len>65535) return SC_E_TICK;
 for(i=0;i<s->count;i++) if(hit(s->notes+i,from,len,inst)) tmp[n++]=s->notes[i];
 if(!n) return SC_E_NONE;
 for(i=0;i<n;i++) tmp[i].tick=tmp[i].tick-from+to;
 if(mv) { for(i=0;i<s->count;i++) if(!hit(s->notes+i,from,len,inst)) s->notes[nd++]=s->notes[i]; s->count=nd; nd=0; } /* cannot fail from here: never more notes than before */
 else { for(i=0;i<n;i++) if(!has(s,tmp[i])) nd++; if(!nd) return SC_E_NONE; if(s->count+nd>SC_EVENTS) return SC_E_FULL; }
 if(to+len>s->end) s->end=to+len;
 for(i=0;i<n;i++) if(!has(s,tmp[i])) sc_add(s,tmp[i]);
 return 1;
}
int sc_copy(ScSong *s,unsigned from,unsigned len,unsigned to,int inst) { return paste(s,from,len,to,inst,0); }
int sc_move(ScSong *s,unsigned from,unsigned len,unsigned to,int inst) { return paste(s,from,len,to,inst,1); }
int sc_delete_bars(ScSong *s,unsigned from,unsigned len) {
 unsigned i,j=0,to; if(from>=s->end || !len) return SC_E_ARG;
 if(len>s->end-from) len=s->end-from; if(len>=s->end) return SC_E_ARG; to=from+len;
 for(i=0;i<s->count;i++) { ScNote n=s->notes[i]; if(n.tick>=from && n.tick<to) continue; if(n.tick>=to) n.tick-=len; s->notes[j++]=n; }
 s->count=j; s->end-=len; s->loop_start=s->loop_start>=to?s->loop_start-len:s->loop_start>from?from:s->loop_start;
 if(s->loop_start>=s->end) s->loop_start=s->end-1; return 1;
}
int sc_insert_bars(ScSong *s,unsigned from,unsigned len) {
 unsigned i; if(from>=s->end || !len) return SC_E_ARG; if(s->end+len>65535) return SC_E_TICK;
 for(i=0;i<s->count;i++) if(s->notes[i].tick>=from) s->notes[i].tick+=len;
 if(s->loop_start>=from) s->loop_start+=len; s->end+=len; return 1;
}
int sc_transpose(ScSong *s,unsigned from,unsigned len,int semis,int inst,unsigned sp) {
 unsigned i,n=0; if(!len || !semis) return SC_E_ARG;
 for(i=0;i<s->count;i++) if(hit(s->notes+i,from,len,inst)) { int p=s->notes[i].pitch+semis; if(p<36 || p>84) return SC_E_PITCH; n++; }
 if(!n) return SC_E_NONE;
 for(i=0;i<s->count;i++) if(hit(s->notes+i,from,len,inst)) { ScNote *p=s->notes+i; int was=sc_black(p->pitch); p->pitch+=semis; p->layer=(p->layer&127)|(sc_black(p->pitch)?was?p->layer&SC_FLAT:sc_spell(p->pitch,sp):0); } return 1;
}
static void put16(uint8_t *p,unsigned v) { p[0]=v; p[1]=v>>8; }
static unsigned get16(const uint8_t *p) { return p[0]|((unsigned)p[1]<<8); }
static uint32_t crc(const uint8_t *p,unsigned n) { uint32_t c=~0u; unsigned j; while(n--) { c^=*p++; for(j=0;j<8;j++) c=(c>>1)^((0u-(c&1))&0xedb88320u); } return ~c; }
unsigned sc_encode(const ScSong *s,uint8_t *out) {
 unsigned i,n=128+8*s->count; uint32_t c; if(!sc_valid(s)) return 0;
 memset(out,0,n+4); memcpy(out,"SCF1",4); put16(out+4,2); put16(out+6,s->count); put16(out+8,s->tempo); put16(out+10,s->end); put16(out+12,s->loop_start); out[14]=s->loop; out[15]=s->beats;
 memcpy(out+16,s->volume,15); memcpy(out+31,s->layers,8);
 memcpy(out+48,s->tune,15); memcpy(out+63,s->tone,15); memcpy(out+78,s->decay,15); memcpy(out+93,s->pan,15); memcpy(out+108,s->midi_ch,15); out[123]=s->midi_mode; out[124]=s->midi_clock; out[125]=s->swing; out[126]=s->human; out[127]=s->sgrid|s->hmode<<1|s->midi_in<<3;
 for(i=0;i<s->count;i++){ uint8_t *p=out+128+8*i; ScNote a=s->notes[i]; put16(p,a.tick);put16(p+2,a.duration);p[4]=a.pitch;p[5]=a.instrument;p[6]=a.velocity;p[7]=a.layer; }
 c=crc(out,n); for(i=0;i<4;i++) out[n+i]=c>>(8*i); return n+4;
}
int sc_decode(ScSong *s,const uint8_t *in,unsigned len) {
 ScSong t; unsigned i,n,v,b; uint32_t c=0; if(len<52 || memcmp(in,"SCF1",4)) return 0;
 v=get16(in+4); if(v<1 || v>2) return 0; b=v==1?48:128;
 n=get16(in+6); if(n>SC_EVENTS || len!=b+4+8*n) return 0;
 for(i=0;i<4;i++) c|=(uint32_t)in[len-4+i]<<(8*i); if(c!=crc(in,len-4)) return 0;
 for(i=39;i<48;i++) if(in[i]) return 0;
 sc_default(&t);t.count=n;t.tempo=get16(in+8);t.end=get16(in+10);t.loop_start=get16(in+12);t.loop=in[14];t.beats=in[15];memcpy(t.volume,in+16,15);memcpy(t.layers,in+31,8);
 for(i=0;i<n;i++){const uint8_t *p=in+b+8*i;t.notes[i]=(ScNote){get16(p),get16(p+2),p[4],p[5],p[6],p[7]}; if(v==1&&p[7]>7) return 0;}
 if(v==2){t.sgrid=in[127]&1;t.hmode=in[127]>>1&3;t.midi_in=in[127]>>3;memcpy(t.tune,in+48,15);memcpy(t.tone,in+63,15);memcpy(t.decay,in+78,15);memcpy(t.pan,in+93,15);memcpy(t.midi_ch,in+108,15);t.midi_mode=in[123];t.midi_clock=in[124];t.swing=in[125];t.human=in[126];}
 if(!sc_valid(&t)) return 0; *s=t; return 1;
}
void sc_engine_init(ScEngine *e,unsigned voices){memset(e,0,sizeof *e);e->limit=voices>=1 && voices<=16?voices:8;}
#define XT (60u*44100) /* phase units per tick, with tempo*96 added per frame */
static uint32_t rate(const ScEngine *e,const ScSong *s){return s->midi_clock==2 && e->xinc?(uint32_t)(e->xinc+.5f):s->tempo*96u;} /* ext: measured */
static void mp(ScEngine *e,unsigned cin,unsigned st,unsigned a,unsigned b){e->midi(cin|st<<8|a<<16|b<<24);} /* USB-MIDI packet, cable 0 */
static void moff(ScEngine *e,unsigned k){mp(e,8,0x80|e->pend[k].ch,e->pend[k].note,0);e->pend[k]=e->pend[--e->np];}
static void mdue(ScEngine *e){unsigned k;
 for(k=0;k<e->np;)if((int32_t)(e->frames-e->pend[k].due)>=0)moff(e,k);else k++;
 if(e->np){e->due=e->pend[0].due;for(k=1;k<e->np;k++)if((int32_t)(e->pend[k].due-e->due)<0)e->due=e->pend[k].due;}
}
static void mon(ScEngine *e,unsigned ch,unsigned note,unsigned vel,uint32_t len){unsigned k,o=0;
 for(k=0;k<e->np;k++){if(e->pend[k].ch==ch && e->pend[k].note==note){moff(e,k);break;}}
 if(e->np==SC_MIDIQ){for(k=1;k<e->np;k++)if((int32_t)(e->pend[k].due-e->pend[o].due)<0)o=k;moff(e,o);} /* full: oldest off early */
 mp(e,9,0x90|ch,note,vel);e->chused|=1u<<ch;
 e->pend[e->np].due=e->frames+len;e->pend[e->np].ch=ch;e->pend[e->np++].note=note;mdue(e);
}
void sc_stop(ScEngine *e){unsigned i;e->playing=0;e->npd=0;for(i=0;i<e->limit;i++)if(e->voices[i].left>128){e->voices[i].left=128;e->voices[i].rel=0;}
 if(e->midi){while(e->np)moff(e,0);for(i=0;i<16;i++)if(e->chused>>i&1)mp(e,11,0xB0|i,123,0);e->chused=0;if(e->clk)mp(e,15,0xFC,0,0);}
 e->clk=0;e->xl=e->xw=0;
}
void sc_release(ScEngine *e,const ScSong *s,unsigned pitch,unsigned ins){ /* gate off: voices fade out (decay tail), MIDI note-off now */
 unsigned i;int pi=pitch+s->tune[ins];if(ins>=15)return;pi=pi<0?0:pi>127?127:pi;
 for(i=0;i<e->limit;i++){ScVoice *v=&e->voices[i];if(v->active && v->instrument==ins && v->pitch==pitch && !v->relp)v->left=v->rel?0:v->left>128?128:v->left;}
 if(e->midi && s->midi_ch[ins])for(i=0;i<e->np;i++)if(e->pend[i].ch==s->midi_ch[ins]-1 && e->pend[i].note==pi){e->pend[i].due=e->frames;mdue(e);break;}
}
void sc_start(ScEngine *e){sc_stop(e);e->tick=0;e->phase=0;e->next=0;e->clkc=0;e->cont=0;e->rng=0x2545F491u;e->playing=1;}
void sc_start_at(ScEngine *e,const ScSong *s,unsigned tick){
 sc_start(e);if(tick>=s->end)return;e->tick=tick;e->clkc=tick;e->cont=tick>0;while(e->next<s->count && s->notes[e->next].tick<tick)e->next++;}
void sc_note(ScEngine *e,const ScSong *s,ScNote n){
 unsigned i,best=0,ins=n.instrument;ScVoice *v;uint32_t oldest=0,len;int pi;
 if(!valid_note(n,65535) || !n.velocity || !s->volume[ins] || !s->layers[n.layer&127] || e->mute>>(n.layer&127)&1 || e->smute>>ins&1 || (e->solo && !(e->solo>>(n.layer&127)&1)))return;
 len=(uint32_t)n.duration*55125u/(rate(e,s)/48u); /* =duration*60*44100/(tempo*96), 32-bit exact: no 64-bit multiply (bsp docs/CPU.md) */
 if(s->midi_mode && s->midi_ch[ins] && e->midi){pi=n.pitch+s->tune[ins];mon(e,s->midi_ch[ins]-1,pi<0?0:pi>127?127:pi,n.velocity,len);}
 if(s->midi_mode==2 && s->midi_ch[ins]){e->triggers++;return;}
 for(i=0;i<e->limit;i++){if(!e->voices[i].active){best=i;break;}if(e->age-e->voices[i].age>=oldest){oldest=e->age-e->voices[i].age;best=i;}}
 v=&e->voices[best]; if(v->active)e->steals++;
 pi=n.pitch-36+s->tune[ins];pi=pi<0?0:pi>48?48:pi;
 v->tail=v->last;v->fade=64;v->pos=0;v->inc=sc_increments[pi];v->instrument=ins;v->pitch=n.pitch;v->active=1;v->age=++e->age;
 v->left=len;v->rel=s->decay[ins]*520u;v->relp=0;v->lp=0; /* release up to ~1.5 s */
 {float t=s->tone[ins]/127.0f;v->k=s->tone[ins]>=127?1.0f:0.01f+0.99f*t*t;}
 v->gl=v->gr=1.0f;if(s->pan[ins]>0)v->gl=(64-s->pan[ins])/64.0f;else if(s->pan[ins]<0)v->gr=(64+s->pan[ins])/64.0f;
 v->gain=(float)n.velocity*s->volume[ins]*s->layers[n.layer&127]/(127.0f*127.0f*127.0f)*0.055f;e->triggers++;
}
/* swing: sgrid 0: off-beat 8ths (tick%96==48) late by swing% of 48 ticks; 1: off-beat 16ths (tick%48==24) of 24; human: random +-8 ticks and +-20 velocity at 100 (hmode: both/timing/velocity; the PRNG draws are the same).
 * Only the scan uses it (not auditions). Due now: straight to sc_note; later: pd[] (kept in scan order); never before the playhead or after end-1. */
static unsigned rnd(ScEngine *e,unsigned m){e->rng=e->rng*1664525u+1013904223u;return (e->rng>>16)%m;}
static void sched(ScEngine *e,const ScSong *s,ScNote n){
 int at=n.tick+(s->sgrid?n.tick%48==24?24:0:n.tick%96==48?48:0)*s->swing/100;
 if(s->human){int j=hj(s),v=s->hmode==1?0:s->human*20/100;at+=(int)rnd(e,2*j+1)-j;v=n.velocity+(int)rnd(e,2*v+1)-v;n.velocity=v<1?1:v>127?127:v;}
 if(at<(int)e->tick)at=e->tick;if(at>(int)s->end-1)at=s->end-1;
 if(at<=(int)e->tick || e->npd==SC_PEND){sc_note(e,s,n);return;}
 e->pd[e->npd].at=at;e->pd[e->npd++].n=n;
}
void sc_ext_clock(ScEngine *e,const ScSong *s,uint32_t us){unsigned c;uint32_t d;float r;(void)s;
 if(e->xn && us-e->xr[(e->xn-1)%25]>500000u)e->xn=0; /* gap: measure afresh */
 e->xr[e->xn++%25]=us;c=e->xn<25?e->xn-1:24;d=us-e->xr[(e->xn-1-c)%25];
 if(c>=4 && d){r=240000000.0f*c/d;e->xinc=r<2880?2880:r>28800?28800:r;} /* 4*XT/(frames per clock) = 2.4e8*c/span_us */
 if(e->xl){if(e->xw){e->xw=0;e->xt=e->tick;}else if((int32_t)(e->frames-e->xlast)>22050)e->xt=(e->tick+2)&~3; /* lost clocks >500 ms: re-anchor */ else e->xt+=4;}
 e->xlast=e->frames;
}
void sc_ext_start(ScEngine *e,const ScSong *s,int cont){
 if(cont && e->playing)return;
 if(cont && e->tick<s->end){sc_stop(e);e->phase=0;e->clkc=0;e->next=0;while(e->next<s->count && s->notes[e->next].tick<e->tick)e->next++;e->playing=1;}else sc_start(e);
 e->xl=e->xw=1;
}
void sc_render(ScEngine *e,const ScSong *s,int32_t *lr,unsigned n,unsigned master){
 unsigned f,i,lead=hj(s); const uint32_t threshold=XT;
 for(f=0;f<n;f++){
  float mix=0,mr=0;
  if(e->np && (int32_t)(e->frames-e->due)>=0)mdue(e);
  if(e->playing && !e->xw){
   if(e->midi && s->midi_clock==1 && !e->clk){e->clk=1;if(e->cont){mp(e,3,0xF2,e->tick/24&127,e->tick/24>>7&127);mp(e,15,0xFB,0,0);}else mp(e,15,0xFA,0,0);mp(e,15,0xF8,0,0);}
   for(i=0;i<e->npd;)if(e->pd[i].at<=e->tick){ScNote n=e->pd[i].n;memmove(e->pd+i,e->pd+i+1,(--e->npd-i)*sizeof *e->pd);sc_note(e,s,n);}else i++;
   while(e->next<s->count && s->notes[e->next].tick<=e->tick+lead)sched(e,s,s->notes[e->next++]);}
  for(i=0;i<e->limit;i++){
   ScVoice *v=&e->voices[i];float sample=0;
   if(v->active){unsigned at=v->pos>>16;
    if(!v->left && v->rel && !v->relp){v->relp=1;v->left=v->rel;}
    if(at+1>=SC_SAMPLE_LEN || !v->left){v->active=0;v->tail=v->last;v->fade=64;}
    else {float frac=(v->pos&65535)/65536.0f;sample=(sc_bank[v->instrument][at]*(1-frac)+sc_bank[v->instrument][at+1]*frac)/32768.0f*v->gain;
     if(v->k<1.0f){v->lp+=v->k*(sample-v->lp);sample=v->lp;}
     if(v->rel){if(v->relp)sample*=v->left/(float)v->rel;}else if(v->left<128)sample*=v->left/128.0f;
     if(v->fade)sample*=(64-v->fade)/64.0f;
     v->left--;v->pos+=v->inc;
    }
   }
   if(v->fade){sample+=v->tail*v->fade/64.0f;v->fade--;}
   if(e->smute>>v->instrument&1)sample=0;
   v->last=sample;mix+=sample*v->gl;mr+=sample*v->gr;
  }
  e->master+=((float)master/4096-e->master)*0.004f;
  if(mix>1)mix=1;if(mix< -1)mix=-1;if(mr>1)mr=1;if(mr< -1)mr=-1;
  lr[2*f]=(int32_t)(mix*e->master*8388607);lr[2*f+1]=(int32_t)(mr*e->master*8388607);
  e->frames++;
  if(e->playing && !e->xw){uint32_t inc=rate(e,s);
   if(e->xl && s->midi_clock==2 && (int32_t)(e->frames-e->xlast)<=22050){float er=(int32_t)(e->xt-e->tick)-e->phase*(1.0f/XT),g=1+(er+2)*.125f; /* ideal er -4..0; crawl at 1/4 when 10+ ticks ahead, up to 3x when behind */
    inc=(uint32_t)(inc*(g<.25f?.25f:g>3?3:g));}
   e->phase+=inc;if(e->phase>=threshold){e->phase-=threshold;e->tick++;
   if(e->midi && s->midi_clock==1 && !(++e->clkc&3))mp(e,15,0xF8,0,0);
   if(e->tick>=s->end){if(s->loop){e->xt-=e->tick-s->loop_start;e->tick=s->loop_start;e->next=0;while(e->next<s->count && s->notes[e->next].tick<e->tick)e->next++;}else sc_stop(e);}
  }}
 }
}
