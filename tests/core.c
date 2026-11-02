/* SPDX-License-Identifier: MIT */
#include "stamp.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
static int32_t lr[512];
static void render(ScEngine *e,ScSong *s,unsigned n){while(n){unsigned b=n>256?256:n;sc_render(e,s,lr,b,4096);n-=b;}}
static uint32_t gold(void){
 ScSong s;ScEngine e;unsigned i,f;uint32_t h=2166136261u;static int32_t o[512];
 sc_default(&s);s.tempo=137;s.end=384;s.loop=1;s.loop_start=96;
 for(i=0;i<60;i++)sc_add(&s,(ScNote){i*6,6+i%40,(uint8_t)(36+(i*7)%49),i%SC_SOUNDS,(uint8_t)(40+i%88),i%8});
 sc_engine_init(&e,8);sc_start(&e);
 for(f=0;f<44100*6;f+=256){sc_render(&e,&s,o,256,4096);for(i=0;i<512;i++){h^=(uint32_t)o[i];h*=16777619u;}}
 return h;}

typedef char fits[8+SC_BYTES<=3840?1:-1]; /* autosave record (8 + wire) in the 3840-byte store object */
static uint32_t render_hash(const ScSong *s){ScEngine e;unsigned i,f;uint32_t h=2166136261u;static int32_t o[512];sc_engine_init(&e,8);sc_start(&e);
 for(f=0;f<44100*3;f+=256){sc_render(&e,s,o,256,4096);for(i=0;i<512;i++){h^=(uint32_t)o[i];h*=16777619u;}}return h;}
/* reference IMA ADPCM decoder, independent of the engine's: frames [a,b) of sound k, from state st (pred,idx) */
static const int RSTEP[89]={7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,80,88,97,107,118,130,143,157,173,190,209,230,253,279,307,337,371,408,449,494,544,598,658,724,796,876,963,1060,1166,1282,1411,1552,1707,1878,2066,2272,2499,2749,3024,3327,3660,4026,4428,4871,5358,5894,6484,7132,7845,8630,9493,10442,11487,12635,13899,15289,16818,18500,20350,22385,24623,27086,29794,32767};
static void refdec(unsigned k,unsigned a,unsigned b,int p,int ix,int *out){const ScInst *q=sc_inst+k;unsigned j;
 for(j=a;j<b;j++){unsigned c=sc_adpcm[q->off+j/256*132+4+j%256/2];int s=RSTEP[ix],d=s>>3;c=j&1?c>>4:c&15;if(c&4)d+=s;if(c&2)d+=s>>1;if(c&1)d+=s>>2;if(c&8)d=-d;
  p+=d;p=p<-32768?-32768:p>32767?32767:p;ix+=(int[]){-1,-1,-1,-1,2,4,6,8}[c&7];ix=ix<0?0:ix>88?88:ix;out[j]=p;}}
/* expected 16.16 increment (32768 = unity) of sound k at key p, from the pack's root/scale/cents */
static double incf(unsigned k,int p){const ScInst *q=sc_inst+k;return 32768*pow(2,((p-q->root)*q->scale+q->cents)/1200.0);}
static int near(uint32_t a,double b){return fabs(a-b)<=1+b*2e-6;}
static int dec[70000];static void fix(uint8_t *b,unsigned n);
static void sounds(void){
 ScSong s;ScEngine e;unsigned k,i,f,nloop=0;static int32_t o[512];
 for(k=0;k<SC_SOUNDS;k++){const ScInst *q=sc_inst+k;const uint8_t *h=sc_adpcm+q->off;unsigned blk;static int seq[70000],tmp[70000];
  assert(q->frames>=2 && q->frames<70000 && q->root>=24 && q->root<=96 && q->scale>=50);
  refdec(k,0,q->frames,(int16_t)(h[0]|h[1]<<8),h[2],seq);
  for(blk=1;blk*256<q->frames;blk++){const uint8_t *g=h+blk*132;refdec(k,blk*256,q->frames,(int16_t)(g[0]|g[1]<<8),g[2],tmp);assert(!memcmp(tmp+blk*256,seq+blk*256,(q->frames-blk*256)*sizeof(int)));} /* restart at any block header: same samples */
  if(q->flags&1){nloop++;assert(q->ls<q->le && q->le==q->frames);refdec(k,q->ls,q->frames,q->lp,q->li,tmp);assert(!memcmp(tmp+q->ls,seq+q->ls,(q->frames-q->ls)*sizeof(int)));} /* loop state: exact */
  if(q->cents==0){unsigned want=q->flags&1?q->frames+3*(q->le-q->ls):q->frames,pit=q->root<36?36:q->root>84?84:q->root;int maxd=0;
   for(i=0;i<want;i++)dec[i]=i<q->frames?seq[i]:seq[q->ls+(i-q->ls)%(q->le-q->ls)];
   sc_default(&s);s.tempo=120;s.end=3000;sc_engine_init(&e,8);e.master=1;sc_note(&e,&s,(ScNote){0,1536,pit,k,127,0}); /* engine == reference at unity pitch (every even output frame = a stored sample; the first 64 fade in) */
   for(f=0;f<want*2;f+=256){sc_render(&e,&s,o,256,4096);for(i=0;i<256 && f+i<want*2;i+=2)if(f+i>=64 && f+i<want*2-4){int ex=(int)(dec[(f+i)/2]/32768.0f*0.055f*8388607),d=o[2*i]-ex;if(d<0)d=-d;if(d>maxd)maxd=d;}}
   assert(maxd<=4);}
  for(i=0;i<3;i++){unsigned pit=(unsigned[]){36,60,84}[i],peak=0; /* audible at C2, C4, C6 */
   sc_default(&s);sc_engine_init(&e,8);e.master=1;sc_note(&e,&s,(ScNote){0,96,pit,k,127,0});for(f=0;f<2048;f+=256){unsigned j;sc_render(&e,&s,o,256,4096);for(j=0;j<512;j++){unsigned a=abs(o[j]);if(a>peak)peak=a;}}
   assert(peak>2000 && peak<8388607);}
  /* long held note: a looped sound sustains well past its sample, then ends after the note; a one-shot ends by itself */
  sc_default(&s);s.tempo=120;s.end=3000;sc_engine_init(&e,8);sc_note(&e,&s,(ScNote){0,1536,60,k,127,0});render(&e,&s,44100*3);
  if(q->flags&1){unsigned pk=0;assert(e.voices[0].active && q->frames*2<44100*3);sc_render(&e,&s,lr,256,4096);for(i=0;i<512;i++)pk|=lr[i]!=0;assert(pk);render(&e,&s,44100*6);assert(!e.voices[0].active);}
  else assert(!e.voices[0].active);
 }
 assert(sc_placeholder?nloop>=2:1);
 /* the last sound's shaping and mute (mask is 32 bits) */
 sc_default(&s);s.volume[30]=0;sc_engine_init(&e,8);sc_note(&e,&s,(ScNote){0,48,60,30,100,0});assert(e.triggers==0);s.volume[30]=127;e.smute=1u<<30;sc_note(&e,&s,(ScNote){0,48,60,30,100,0});assert(e.triggers==0);e.smute=0;sc_note(&e,&s,(ScNote){0,48,60,30,100,0});assert(e.triggers==1);
 assert(!sc_add(&s,(ScNote){0,48,60,31,100,0}) && sc_add(&s,(ScNote){0,48,60,30,100,0}));
 {ScSong a=s,b;uint8_t w[SC_BYTES];unsigned n;n=sc_encode(&a,w);assert(n && sc_decode(&b,w,n));w[216+5]=31;fix(w,n);assert(!sc_decode(&b,w,n));} /* instrument 31 rejected on the wire */
 /* defaults are neutral: a song with every shaping value set explicitly to its neutral value renders exactly as sc_default's */
 {ScSong a,b;unsigned j;sc_default(&a);for(j=0;j<12;j++)sc_add(&a,(ScNote){j*8,24,(uint8_t)(40+j*3),j*3%SC_SOUNDS,90,j%8});b=a;
  for(j=0;j<SC_SOUNDS;j++){b.volume[j]=127;b.tone[j]=127;b.decay[j]=0;b.pan[j]=0;b.tune[j]=0;b.midi_ch[j]=0;}b.swing=0;b.human=0;b.midi_mode=0;assert(render_hash(&a)==render_hash(&b));}
}
static uint32_t pk[256],pf[256];static unsigned np_,fr_;
static void cap(uint32_t p){if(np_<256){pk[np_]=p;pf[np_]=fr_;}np_++;}
static void run(ScEngine *e,ScSong *s,unsigned n){while(n--){fr_++;sc_render(e,s,lr,1,4096);}} /* fr_ = frames done */
static void fix(uint8_t *b,unsigned n){uint32_t c=~0u;unsigned i,j;for(i=0;i<n-4;i++){c^=b[i];for(j=0;j<8;j++)c=(c>>1)^((0u-(c&1))&0xedb88320u);}c=~c;for(i=0;i<4;i++)b[n-4+i]=c>>(8*i);}
static unsigned ep=36;
static float energy(const ScSong *s,unsigned ch,unsigned a,unsigned b,float *hf){ /* sound 3 pitch 36, 24 ticks; sum |x| over channel ch, frames a..b; hf = sum |dx| */
 ScEngine e;unsigned f;float sum=0;int32_t pv=0;*hf=0;sc_engine_init(&e,8);e.master=1;
 sc_note(&e,s,(ScNote){0,24,ep,3,127,0});
 for(f=0;f<b;f++){sc_render(&e,s,lr,1,4096);if(f>a){sum+=(float)abs(lr[ch]);*hf+=(float)abs(lr[ch]-pv);}pv=lr[ch];}
 return sum;}

/* external clock: clocks arrive at the first 64-frame block boundary after their true time (as the render's blocks do), stamped true time +-1 ms */
static uint32_t rng_=1;static double nt_,worst_;static int ck_;
static int jit(void){rng_=rng_*1664525u+1013904223u;return (int)(rng_>>16)%2001-1000;}
static double pos(const ScEngine *e){return e->tick+e->phase/(60.0*44100);}
static void feed(ScEngine *e,ScSong *s,double bpm,unsigned n,int chk){ /* n clocks; chk: |pos-4*ck_| <= 2 after each */
 while(n--){nt_+=60e6/(bpm*24);while(e->frames*1e6/44100<nt_)sc_render(e,s,lr,64,4096);
  sc_ext_clock(e,s,(uint32_t)(nt_+jit()));
  if(chk&&e->playing&&!e->xw){double d=pos(e)-4.0*ck_;if(d<0)d=-d;if(d>worst_)worst_=d;assert(d<=2.0);}
  ck_++;}}
static void ext(ScSong *s,ScEngine *e,unsigned song_bpm){sc_default(s);s->tempo=song_bpm;s->end=65535;s->midi_clock=2;sc_engine_init(e,8);nt_=1e6;worst_=0;ck_=0;}
/* range ops (bar = beats*96): merge/overlap rules, shifts, loop_start, atomic refusals; beats 4 and 3 */
#define SAME (!memcmp(&s,&t,sizeof s))
static void bars(void){
 ScSong s,t;unsigned b,i,bar;
 for(b=4;b>=3;b--){
  bar=b*96;sc_default(&s);s.beats=b;s.end=bar*4;s.loop_start=bar*2;for(i=0;i<4;i++)sc_add(&s,(ScNote){i*bar+12,24,40+i,i%2,100,0});t=s; /* one note per bar: pitch 40+bar, sound = bar%2 */
  assert(sc_copy(&s,0,bar,bar*2,-1)==1 && s.count==5 && s.notes[2].pitch==42 && s.notes[3].pitch==40 && s.notes[3].tick==bar*2+12 && sc_valid(&s)); /* existing note at the target kept */
  assert(sc_copy(&s,0,bar,bar*2,-1)==SC_E_NONE && s.count==5 && sc_valid(&s)); /* all duplicates: nothing to do */
  s=t;assert(sc_copy(&s,0,bar*2,bar,-1)==1 && s.count==6 && s.end==bar*4 && s.notes[1].pitch==41 && s.notes[2].pitch==40 && s.notes[2].tick==bar+12 && s.notes[3].pitch==42 && s.notes[4].pitch==41 && s.notes[4].tick==bar*2+12 && sc_valid(&s)); /* overlapping target: bars 1-2 onto 2-3 from a snapshot */
  s=t;assert(sc_copy(&s,bar*3,bar,bar*5,-1)==1 && s.end==bar*6 && s.count==5 && s.notes[4].tick==bar*5+12 && s.loop_start==bar*2 && sc_valid(&s)); /* past the end: end grows */
  s=t;assert(sc_copy(&s,0,bar,bar*2,1)==SC_E_NONE && SAME && sc_copy(&s,bar,bar,bar*2,1)==1 && s.count==5 && s.notes[3].pitch==41 && sc_valid(&s)); /* sound filter */
  s=t;assert(sc_move(&s,0,bar,bar*3,-1)==1 && s.count==4 && s.notes[0].pitch==41 && s.notes[3].pitch==40 && s.notes[3].tick==bar*3+12 && sc_valid(&s));
  s=t;assert(sc_move(&s,0,bar*2,bar,-1)==1 && s.count==4 && s.notes[0].pitch==40 && s.notes[0].tick==bar+12 && s.notes[1].pitch==42 && s.notes[2].pitch==41 && s.notes[2].tick==bar*2+12 && s.notes[3].tick==bar*3+12 && sc_valid(&s)); /* overlap: source bar 2 is emptied, bar 1 lands in it */
  s=t;s.notes[1].pitch=40;s.notes[1].instrument=0;assert(sc_move(&s,0,bar,bar,-1)==1 && s.count==3 && s.notes[0].tick==bar+12 && sc_valid(&s)); /* same note already at the target: kept, moved one dropped */
  s=t;assert(sc_move(&s,0,bar,bar*5,-1)==1 && s.end==bar*6 && s.count==4 && sc_valid(&s));
  s=t;assert(sc_move(&s,0,bar,0,-1)==SC_E_ARG && sc_copy(&s,0,0,bar,-1)==SC_E_ARG && sc_copy(&s,0,bar,65535,-1)==SC_E_TICK && sc_move(&s,bar*3,bar,bar,0)==SC_E_NONE && SAME);
  assert(sc_delete_bars(&s,bar,bar)==1 && s.count==3 && s.end==bar*3 && s.loop_start==bar && s.notes[1].tick==bar+12 && s.notes[1].pitch==42 && sc_valid(&s)); /* later loop_start shifts */
  s=t;assert(sc_delete_bars(&s,bar,bar*2)==1 && s.end==bar*2 && s.loop_start==bar && s.count==2 && s.notes[1].tick==bar+12 && s.notes[1].pitch==43 && sc_valid(&s)); /* loop_start inside the cut goes to its start */
  s=t;assert(sc_delete_bars(&s,bar*3,bar)==1 && s.end==bar*3 && s.loop_start==bar*2 && s.count==3 && sc_valid(&s));
  s=t;s.loop_start=0;assert(sc_delete_bars(&s,0,bar*3)==1 && s.end==bar && s.loop_start==0 && s.count==1 && s.notes[0].tick==12 && sc_valid(&s));
  s=t;s.end=bar*3+50;assert(sc_delete_bars(&s,bar*3,bar)==1 && s.end==bar*3 && s.count==3 && sc_valid(&s)); /* partial last bar clamps */
  s=t;assert(sc_delete_bars(&s,0,bar*4)==SC_E_ARG && sc_delete_bars(&s,bar*4,bar)==SC_E_ARG && sc_delete_bars(&s,0,0)==SC_E_ARG && SAME);
  assert(sc_insert_bars(&s,bar,bar*2)==1 && s.end==bar*6 && s.loop_start==bar*4 && s.count==4 && s.notes[0].tick==12 && s.notes[1].tick==bar*3+12 && s.notes[3].tick==bar*5+12 && sc_valid(&s));
  s=t;s.loop_start=0;assert(sc_insert_bars(&s,0,bar)==1 && s.loop_start==bar && s.notes[0].tick==bar+12 && sc_valid(&s));
  s=t;s.end=65536-bar;assert(sc_insert_bars(&s,0,bar)==SC_E_TICK && sc_insert_bars(&s,s.end,bar)==SC_E_ARG && sc_insert_bars(&s,0,0)==SC_E_ARG);s.end=bar*4;assert(SAME);
  assert(sc_transpose(&s,bar,bar*2,3,-1,0)==1 && s.notes[0].pitch==40 && s.notes[1].pitch==44 && s.notes[2].pitch==45 && s.notes[3].pitch==43 && sc_valid(&s));
  s=t;assert(sc_transpose(&s,0,bar*4,-12,-1,0)==SC_E_PITCH && SAME && sc_transpose(&s,0,bar*4,0,-1,0)==SC_E_ARG && sc_transpose(&s,0,bar*4,2,5,0)==SC_E_NONE && SAME);
  assert(sc_transpose(&s,0,bar*4,2,1,0)==1 && s.notes[0].pitch==40 && s.notes[1].pitch==43 && s.notes[3].pitch==45);
  s=t;s.notes[3].pitch=84;assert(sc_transpose(&s,0,bar*4,1,-1,0)==SC_E_PITCH && sc_transpose(&s,0,bar*3,1,-1,0)==1 && s.notes[0].pitch==41 && s.notes[3].pitch==84 && sc_valid(&s));
  s=t;s.notes[0].pitch=36;t=s;assert(sc_transpose(&s,0,bar,-1,-1,0)==SC_E_PITCH && SAME);
 }
 sc_default(&s);s.end=65535;for(i=0;i<SC_EVENTS-1;i++)sc_add(&s,(ScNote){i,24,36+i%49,i%SC_SOUNDS,100,0});t=s; /* full: copy refused, move still fits */
 assert(sc_copy(&s,0,384,384,-1)==SC_E_FULL && SAME && sc_valid(&s));
 sc_add(&s,(ScNote){500,24,60,0,100,0});assert(sc_move(&s,0,384,384,-1)==1 && s.count==SC_EVENTS && sc_valid(&s));
 sc_default(&s);s.end=65535-384;sc_add(&s,(ScNote){s.end-1,24,60,0,100,0});assert(sc_copy(&s,0,s.end,0,-1)==SC_E_ARG && sc_copy(&s,s.end-384,384,s.end,-1)==1 && s.end==65535 && sc_valid(&s)); /* ends exactly at the limit */
}
/* live edits while playing keep the scan index; sc_release gates a long note */
static unsigned cnt(unsigned m,unsigned pitch){unsigned i,c=0;for(i=0;i<np_&&i<256;i++)if((pk[i]&0xF0FF)==m && (pk[i]>>16&255)==pitch)c++;return c;}
static void live_t(void){
 ScSong s;ScEngine e;unsigned i,nx;
 sc_default(&s);s.midi_mode=1;s.midi_ch[0]=1;for(i=0;i<4;i++)sc_add(&s,(ScNote){i*96,24,40+i,0,100,0});
 sc_engine_init(&e,8);e.midi=cap;np_=fr_=0;sc_start(&e);
 while(e.tick<100)run(&e,&s,64);
 assert(e.next==2);
 assert(sc_insert(&e,&s,(ScNote){48,24,50,0,100,0})==1 && e.next==3); /* behind the playhead: next moves with its note */
 assert(sc_insert(&e,&s,(ScNote){(uint16_t)e.tick,24,51,0,100,0})==1 && e.next==4); /* due now: the scan must not trigger it */
 assert(sc_insert(&e,&s,(ScNote){200,24,52,0,100,0})==1 && e.next==4); /* ahead: plays when reached */
 assert(sc_insert(&e,&s,(ScNote){200,24,52,0,100,0})==SC_E_NONE && sc_valid(&s));
 assert(sc_remove(&e,&s,(ScNote){288,24,43,0,100,0})==1 && e.next==4 && !sc_remove(&e,&s,(ScNote){288,24,43,0,100,0}));
 nx=e.next;assert(sc_remove(&e,&s,(ScNote){0,24,40,0,100,0})==1 && e.next==nx-1); /* behind: next follows */
 while(e.tick<400)run(&e,&s,64);
 assert(cnt(0x9009,40)==1 && cnt(0x9009,41)==1 && cnt(0x9009,42)==1 && cnt(0x9009,43)==0 && cnt(0x9009,50)==0 && cnt(0x9009,51)==0 && cnt(0x9009,52)==1);
 while(s.count<SC_EVENTS)sc_add(&s,(ScNote){1000,24,60,1,100,0});
 assert(s.count==SC_EVENTS && sc_insert(&e,&s,(ScNote){1100,24,60,1,100,0})==SC_E_FULL);
 sc_default(&s);s.midi_mode=1;s.midi_ch[0]=1;s.decay[0]=10;sc_engine_init(&e,8);e.midi=cap;np_=0;e.master=1;
 sc_note(&e,&s,(ScNote){0,65535,60,0,100,0});run(&e,&s,2000);assert(e.voices[0].active && np_==1 && cnt(0x9009,60)==1);
 sc_release(&e,&s,60,0);assert(np_==2 && cnt(0x8008,60)==1); /* note-off now */
 run(&e,&s,100);assert(e.voices[0].active); /* decay tail */
 run(&e,&s,10*520+300);assert(!e.voices[0].active);
 sc_release(&e,&s,60,0);assert(np_==2); /* once */
 s.decay[0]=0;sc_note(&e,&s,(ScNote){0,65535,62,0,100,0});run(&e,&s,500);sc_release(&e,&s,62,0);run(&e,&s,200);assert(!e.voices[0].active && cnt(0x8008,62)==1);
}
int main(int argc,char **argv){
 ScSong s,t;ScEngine e;uint8_t bytes[SC_BYTES];unsigned n,i,j;
 sc_default(&s);assert(sc_add(&s,(ScNote){0,96,60,0,100,0}));assert(sc_add(&s,(ScNote){96,96,64,6,100,1}));n=sc_encode(&s,bytes);assert(n==236);assert(sc_decode(&t,bytes,n));assert(!memcmp(&s,&t,sizeof s));bytes[49]^=1;assert(!sc_decode(&t,bytes,n));assert(!memcmp(&s,&t,sizeof s));
 for(i=30;i<=300;i+=27){sc_default(&s);s.tempo=i;s.end=65535;sc_engine_init(&e,16);sc_start(&e);render(&e,&s,44100*7);assert(e.tick==i*96u*7/60);}
 for(i=24;i<=96;i*=2){sc_default(&s);s.end=i*3;sc_add(&s,(ScNote){0,24,60,0,100,0});sc_add(&s,(ScNote){i,24,64,3,100,0});sc_engine_init(&e,8);sc_start(&e);render(&e,&s,(44100*i+191)/(96*2));assert(e.triggers==1);render(&e,&s,1);assert(e.triggers==2);}
 sc_default(&s);s.end=192;sc_add(&s,(ScNote){0,96,60,0,100,0});sc_add(&s,(ScNote){96,96,64,3,100,0});sc_engine_init(&e,8);sc_start(&e);render(&e,&s,44100);assert(!e.playing && e.triggers==2);
 s.loop=1;s.loop_start=96;sc_start(&e);e.triggers=0;render(&e,&s,44100+22050+1);assert(e.playing && e.triggers==4);
 sc_engine_init(&e,8);for(i=0;i<20;i++)sc_note(&e,&s,(ScNote){0,96,60,i%SC_SOUNDS,127,0});assert(e.steals==12);render(&e,&s,44100*4);for(i=0;i<8;i++)assert(!e.voices[i].active && e.voices[i].last==0);
 sc_default(&s);for(i=0;i<384;i++)assert(sc_add(&s,(ScNote){i,24,60,i%SC_SOUNDS,100,i%8}));t=s;assert(!sc_duplicate(&s,0,384));assert(!memcmp(&s,&t,sizeof s));assert(!sc_add(&s,(ScNote){0,24,60,0,100,0}));

 bars();live_t();
 /* v1 / v2 wire */
 sc_default(&s);sc_add(&s,(ScNote){0,96,60,0,100,0});n=sc_encode(&s,bytes);memset(bytes+39,0,9);memmove(bytes+48,bytes+216,12);n=48+8+4;bytes[4]=1;fix(bytes,n);assert(sc_decode(&t,bytes,n));assert(!memcmp(&s,&t,sizeof s));assert(t.tone[14]==127 && !t.tune[3] && !t.midi_mode);
 sc_default(&s);for(i=0;i<SC_SOUNDS;i++){s.volume[i]=127-i;s.tune[i]=i%25-12;s.tone[i]=i*4;s.decay[i]=127-i;s.pan[i]=i*4-64;s.midi_ch[i]=i%16+1;}s.midi_mode=2;s.midi_clock=1;s.swing=75;s.human=100;s.sgrid=1;s.hmode=2;s.midi_in=16;sc_add(&s,(ScNote){5,9,40,2,90,3});
 n=sc_encode(&s,bytes);assert(n==220+8 && bytes[4]==2);assert(sc_decode(&t,bytes,n));assert(!memcmp(&s,&t,sizeof s));
 for(i=0;i<11;i++){unsigned off[11]={55,86,117,148,179,210,211,212,213,16+30,55+30},bad[11]={13,128,128,64,17,3,3,76,101,128,13};uint8_t k=bytes[off[i]];bytes[off[i]]=bad[i];fix(bytes,n);assert(!sc_decode(&t,bytes,n));bytes[off[i]]=k;} /* tune,tone,decay,pan,ch,mode,clock,swing,human, volume[30], tune[30]: the last sound is checked too */
 bytes[55]=-13;fix(bytes,n);assert(!sc_decode(&t,bytes,n));bytes[55]=-7;bytes[148]=-65;fix(bytes,n);assert(!sc_decode(&t,bytes,n));bytes[148]=-64;bytes[214]=6|1<<7;fix(bytes,n);assert(!sc_decode(&t,bytes,n));bytes[214]=1|2<<1|17<<3;fix(bytes,n);assert(!sc_decode(&t,bytes,n));bytes[214]=1|2<<1|16<<3;fix(bytes,n);assert(sc_decode(&t,bytes,n)&&t.sgrid==1&&t.hmode==2&&t.midi_in==16); /* byte 214: grid | human mode<<1 | MIDI-in channel<<3 */
 bytes[60]^=1;assert(!sc_decode(&t,bytes,n));bytes[60]^=1;assert(!sc_decode(&t,bytes,n-1));
 s.swing=76;assert(!sc_encode(&s,bytes));s.swing=75;s.human=101;assert(!sc_encode(&s,bytes));s.human=100;s.midi_ch[0]=17;assert(!sc_encode(&s,bytes));s.midi_ch[0]=1;s.pan[0]=64;assert(!sc_encode(&s,bytes));s.pan[0]=0;s.sgrid=2;assert(!sc_encode(&s,bytes));s.sgrid=1;s.hmode=3;assert(!sc_encode(&s,bytes));s.hmode=0;s.midi_in=17;assert(!sc_encode(&s,bytes));
 /* flats: SC_FLAT (layer bit 7) round-trips, black keys only; a v1 file decodes with it clear; spelling changes no sound and no identity */
 sc_default(&s);assert(sc_add(&s,(ScNote){0,96,58,0,100,SC_FLAT|3}) && sc_add(&s,(ScNote){96,96,61,2,100,1}) && !sc_add(&s,(ScNote){0,96,60,0,100,SC_FLAT}) && s.count==2 && sc_valid(&s));
 n=sc_encode(&s,bytes);assert(n==220+16 && bytes[216+7]==(0x80|3) && bytes[224+7]==1);assert(sc_decode(&t,bytes,n) && !memcmp(&s,&t,sizeof s) && t.notes[0].layer==(SC_FLAT|3) && t.notes[1].layer==1);
 bytes[220]=60;fix(bytes,n);assert(!sc_decode(&t,bytes,n));bytes[220]=58; /* flag on a white key (C4): rejected */
 bytes[216+7]=0x80|8;fix(bytes,n);assert(!sc_decode(&t,bytes,n)); /* layer 8 */
 s.notes[1].pitch=62;s.notes[1].layer|=SC_FLAT;assert(!sc_valid(&s) && !sc_encode(&s,bytes));s.notes[1].layer&=127;assert(sc_valid(&s));
 sc_default(&s);sc_add(&s,(ScNote){0,96,58,0,100,SC_FLAT|3});n=sc_encode(&s,bytes);memset(bytes+39,0,9);memmove(bytes+48,bytes+216,12);n=48+8+4;bytes[4]=1;fix(bytes,n);assert(!sc_decode(&t,bytes,n)); /* v1 has no flag: layer 0x83 invalid */
 bytes[48+7]=3;fix(bytes,n);assert(sc_decode(&t,bytes,n) && t.notes[0].layer==3 && t.notes[0].pitch==58);
 assert(sc_find(&s,(ScNote){0,1,58,0,1,3})==0 && sc_find(&s,(ScNote){0,1,58,0,1,3|SC_FLAT})==0 && sc_find(&s,(ScNote){0,1,58,0,1,4})<0 && sc_insert(&e,&s,(ScNote){0,24,58,0,100,3})==SC_E_NONE);
 for(i=36;i<=84;i++){assert(sc_spell(i,0)==0 && sc_spell(i,2)==(sc_black(i)?SC_FLAT:0) && sc_spell(i,1)==(i%12==3||i%12==8||i%12==10?SC_FLAT:0) && sc_black(i)==(i%12==1||i%12==3||i%12==6||i%12==8||i%12==10));}
 {ScSong a,b;ScEngine ea,eb;static int32_t oa[512],ob[512];sc_default(&a);b=a;sc_add(&a,(ScNote){0,96,58,3,100,0});sc_add(&b,(ScNote){0,96,58,3,100,SC_FLAT});sc_engine_init(&ea,8);eb=ea;sc_start(&ea);sc_start(&eb);for(i=0;i<40;i++){sc_render(&ea,&a,oa,256,4096);sc_render(&eb,&b,ob,256,4096);assert(!memcmp(oa,ob,sizeof oa));}} /* the same sound */
 /* transpose: a flat stays on a black key, a white landing clears it, a white key turning black takes the mode */
 sc_default(&s);sc_add(&s,(ScNote){0,24,58,0,100,SC_FLAT});sc_add(&s,(ScNote){10,24,61,0,100,0});sc_add(&s,(ScNote){20,24,60,0,100,0});sc_add(&s,(ScNote){30,24,66,0,100,SC_FLAT});t=s;
 assert(sc_transpose(&s,0,96,1,-1,2)==1 && s.notes[0].pitch==59 && s.notes[0].layer==0 && s.notes[1].pitch==62 && s.notes[1].layer==0 && s.notes[2].pitch==61 && s.notes[2].layer==SC_FLAT && s.notes[3].pitch==67 && s.notes[3].layer==0 && sc_valid(&s));
 s=t;assert(sc_transpose(&s,0,96,1,-1,0)==1 && s.notes[2].pitch==61 && s.notes[2].layer==0 && sc_valid(&s));
 s=t;assert(sc_transpose(&s,0,96,2,-1,2)==1 && s.notes[0].pitch==60 && s.notes[0].layer==0 && s.notes[1].pitch==63 && s.notes[1].layer==0 && s.notes[2].pitch==62 && s.notes[2].layer==0 && s.notes[3].pitch==68 && s.notes[3].layer==SC_FLAT && sc_valid(&s)); /* C#4 keeps its sharp, Gb4 its flat */
 s=t;assert(sc_transpose(&s,0,96,-12,-1,1)==1 && s.notes[0].layer==SC_FLAT && s.notes[1].layer==0 && s.notes[3].layer==SC_FLAT);
 /* defaults render exactly as before the shaping/MIDI work */
 if(sc_placeholder)assert(gold()==0x19bc7683u); /* placeholder pack (make_bank.py --placeholder): regenerated with the ADPCM engine; the SMP pack has no pinned hash */sounds();

 /* swing/human: defaults identical (gold above); swing 50, 8TH grid: delays the off-beat 8th (tick 48) by 24 ticks = 5512 frames, nothing else; 16TH grid: the off-beat 16ths (24, 72) by 12, nothing else */
 sc_default(&s);s.midi_mode=1;s.midi_ch[0]=1;s.end=384;s.swing=50;sc_add(&s,(ScNote){0,6,60,0,100,0});sc_add(&s,(ScNote){24,6,61,0,100,0});sc_add(&s,(ScNote){48,6,62,0,100,0});sc_add(&s,(ScNote){96,6,63,0,100,0});
 sc_engine_init(&e,8);e.midi=cap;np_=fr_=0;sc_start(&e);run(&e,&s,44100);
 {unsigned on[4],c=0;for(i=0;i<np_;i++)if((pk[i]&0xF0FF)==0x9009)on[c++]=pf[i];assert(c==4 && on[0]==1);
  assert(on[1]-on[0]>=5510 && on[1]-on[0]<=5514 && on[2]-on[0]>=16536 && on[2]-on[0]<=16539 && on[3]-on[0]>=22049 && on[3]-on[0]<=22052);} /* order kept: 60,61,62,63 */
 {unsigned c=0,last=0;for(i=0;i<np_;i++)if((pk[i]&0xF0FF)==0x9009){assert((pk[i]>>16&255)==60+c++ && pf[i]>=last);last=pf[i];}assert(c==4);}
 s.sgrid=1;sc_engine_init(&e,8);e.midi=cap;np_=fr_=0;sc_start(&e);run(&e,&s,44100);
 {unsigned on[4],c=0;for(i=0;i<np_;i++)if((pk[i]&0xF0FF)==0x9009)on[c++]=pf[i];assert(c==4 && on[1]-on[0]>=8267 && on[1]-on[0]<=8271 && on[2]-on[0]>=11023 && on[2]-on[0]<=11027 && on[3]-on[0]>=22049 && on[3]-on[0]<=22052);}
 /* delayed trigger never crosses the song end; mute/solo; human deterministic, bounded */
 sc_default(&s);s.end=50;s.swing=75;sc_add(&s,(ScNote){48,6,60,0,100,0});sc_engine_init(&e,8);sc_start(&e);render(&e,&s,10300);assert(e.playing && e.triggers==0);render(&e,&s,1500);assert(e.triggers==1 && !e.playing); /* tick 48+36 clamps to 49 */
 {unsigned a[2][18],r,tk;for(r=0;r<2;r++){sc_default(&s);s.human=100;s.midi_mode=1;s.midi_ch[0]=1;s.end=480;for(i=0;i<8;i++)sc_add(&s,(ScNote){i*3,6,60+i,0,100,0});sc_add(&s,(ScNote){96,6,70,0,100,0});
   sc_engine_init(&e,8);e.midi=cap;np_=fr_=0;sc_start(&e);run(&e,&s,44100);tk=0;for(i=0;i<np_;i++)if((pk[i]&0xF0FF)==0x9009){a[r][tk++]=pf[i];a[r][9+tk-1]=pk[i]>>24;}assert(tk==9);}
  assert(!memcmp(a[0],a[1],sizeof a[0]));for(i=0;i<9;i++){assert(a[0][i]<=105*230);assert(a[0][9+i]>=80 && a[0][9+i]<=120);}assert(a[0][9]!=100 || a[0][10]!=100 || a[0][11]!=100 || a[0][12]!=100);
  sc_default(&s);s.human=100;sc_add(&s,(ScNote){0,6,60,0,100,0});sc_engine_init(&e,8);sc_start(&e);render(&e,&s,2000);assert(e.triggers==1);} /* tick 0 note never before 0 */
 /* human modes: TIME+VEL (0), TIME (1) and VEL (2) are deterministic; 1 moves like 0 with velocity exact, 2 has velocity like 0 with exact timing */
 {unsigned a[3][18],r,tk;for(r=0;r<3;r++){sc_default(&s);s.human=100;s.hmode=r;s.midi_mode=1;s.midi_ch[0]=1;s.end=480;for(i=0;i<8;i++)sc_add(&s,(ScNote){i*3,6,60+i,0,100,0});sc_add(&s,(ScNote){96,6,70,0,100,0});
   sc_engine_init(&e,8);e.midi=cap;np_=fr_=0;sc_start(&e);run(&e,&s,44100);tk=0;for(i=0;i<np_;i++)if((pk[i]&0xF0FF)==0x9009){a[r][tk++]=pf[i];a[r][9+tk-1]=pk[i]>>24;}assert(tk==9);}
  for(i=0;i<9;i++)assert(a[0][i]==a[1][i] && a[1][9+i]==100);
  assert(memcmp(a[0],a[2],9*sizeof a[0][0]));for(i=0,tk=0;i<9;i++)tk|=a[2][9+i]!=100;assert(tk);
  for(i=0;i<8;i++)assert(a[2][i]>=(i*3*2297+5)/10 && a[2][i]<=i*3*2297/10+3);} /* VEL only: frame ~ tick*229.7, no jitter */
 /* sound mute: no audio, no MIDI, nothing triggered; unmuting restores it; the volume is untouched */
 sc_default(&s);s.midi_mode=1;s.midi_ch[2]=1;sc_engine_init(&e,8);e.midi=cap;np_=0;e.smute=1u<<2;sc_note(&e,&s,(ScNote){0,48,60,2,100,0});assert(e.triggers==0 && np_==0 && !e.voices[0].active);
 sc_note(&e,&s,(ScNote){0,48,60,3,100,0});assert(e.triggers==1);e.smute=1u<<3;run(&e,&s,2000);{unsigned z=0;int32_t o[512];sc_render(&e,&s,o,256,4096);for(i=0;i<512;i++)z|=o[i]!=0;assert(!z);} /* a sounding voice is silenced too */
 e.smute=0;sc_note(&e,&s,(ScNote){0,48,60,2,100,0});assert(e.triggers==2 && np_==1 && s.volume[2]==127);
 sc_default(&s);sc_add(&s,(ScNote){0,48,60,0,100,2});sc_add(&s,(ScNote){0,48,64,0,100,5});sc_engine_init(&e,8);e.midi=cap;s.midi_mode=1;s.midi_ch[0]=1;
 e.mute=1u<<2;np_=0;sc_start(&e);run(&e,&s,100);assert(e.triggers==1 && np_==1 && (pk[0]>>16&255)==64 && e.voices[0].active && !e.voices[1].active);
 e.mute=0;e.solo=1u<<2;sc_stop(&e);np_=0;e.triggers=0;sc_start(&e);run(&e,&s,100);assert(e.triggers==1 && (pk[0]>>16&255)==60);
 e.solo=1u<<2;e.mute=1u<<2;sc_stop(&e);np_=0;e.triggers=0;sc_start(&e);run(&e,&s,100);assert(e.triggers==0 && np_==0);e.mute=e.solo=0;
 /* start at a tick: next is the first note at or after it; MIDI Start becomes SPP + Continue */
 sc_default(&s);s.midi_clock=1;s.end=1000;for(i=0;i<6;i++)sc_add(&s,(ScNote){i*96,24,60,0,100,0});sc_engine_init(&e,8);e.midi=cap;
 sc_start_at(&e,&s,288);assert(e.playing && e.tick==288 && e.next==3);sc_start_at(&e,&s,289);assert(e.next==4);sc_start_at(&e,&s,2000);assert(e.tick==0 && e.next==0);
 np_=0;sc_start_at(&e,&s,192);run(&e,&s,1);assert(pk[0]==(3u|0xF2u<<8|8u<<16) && pk[1]==(0xFu|0xFBu<<8) && pk[2]==(0xFu|0xF8u<<8) && e.triggers>=1);
 sc_stop(&e);np_=0;sc_start_at(&e,&s,0);run(&e,&s,1);assert(pk[0]==(0xFu|0xFAu<<8));
 /* tune, tone, decay, pan */
 sc_default(&s);sc_engine_init(&e,8);s.tune[3]=12;sc_note(&e,&s,(ScNote){0,24,36,3,127,0});s.tune[3]=0;sc_note(&e,&s,(ScNote){0,24,48,3,127,0});assert(e.voices[0].inc==e.voices[1].inc && e.voices[0].inc>8192);
 sc_engine_init(&e,8);s.tune[3]=-12;sc_note(&e,&s,(ScNote){0,24,50,3,127,0});sc_note(&e,&s,(ScNote){0,24,36,3,127,0});assert(e.voices[0].inc>e.voices[1].inc && near(e.voices[1].inc,incf(3,36)));
 s.tune[3]=12;sc_note(&e,&s,(ScNote){0,24,84,3,127,0});assert(near(e.voices[2].inc,incf(3,84)));
 {float a,b,x,y;ep=84;sc_default(&s);a=energy(&s,0,0,6000,&x);s.tone[3]=0;b=energy(&s,0,0,6000,&y);assert(y<x*0.5f && b>0 && b<a);ep=36;}
 {float x;float a,b;sc_default(&s);s.tempo=300;a=energy(&s,0,12000,30000,&x);s.decay[3]=100;b=energy(&s,0,12000,30000,&x);assert(a==0 && b>0);}
 {float x;float l,r;sc_default(&s);s.pan[3]=-64;l=energy(&s,0,0,5000,&x);r=energy(&s,1,0,5000,&x);assert(l>0 && r==0);s.pan[3]=63;l=energy(&s,0,0,5000,&x);r=energy(&s,1,0,5000,&x);assert(r>0 && l<r*0.05f);s.pan[3]=0;l=energy(&s,0,0,5000,&x);r=energy(&s,1,0,5000,&x);assert(l==r && l>0);}
 /* MIDI out at 120 BPM: 1 tick = 229.6875 frames; 96-tick note = 22050 frames */
 sc_default(&s);s.midi_mode=1;s.midi_ch[0]=3;s.midi_ch[1]=10;s.tune[0]=2;sc_add(&s,(ScNote){0,96,60,0,100,0});sc_add(&s,(ScNote){96,48,36,1,80,0});s.end=192;
 sc_engine_init(&e,8);e.midi=cap;np_=fr_=0;sc_start(&e);run(&e,&s,22049);assert(np_==1 && pk[0]==(9u|0x92u<<8|62u<<16|100u<<24) && pf[0]==1);
 run(&e,&s,1);assert(np_==1);run(&e,&s,1);assert(np_==3 && pk[1]==(8u|0x82u<<8|62u<<16) && pf[1]==22051 && pk[2]==(9u|0x99u<<8|36u<<16|80u<<24) && pf[2]==22051);
 run(&e,&s,11024);assert(np_==3);run(&e,&s,1);assert(np_==4 && pk[3]==(8u|0x89u<<8|36u<<16) && pf[3]==33076);
 run(&e,&s,12000);assert(!e.playing && np_==6 && (pk[4]&0xFFFF)==(0xBu|0xB2u<<8) && pk[4]>>16==123 && (pk[5]&0xFFFF)==(0xBu|0xB9u<<8) && e.np==0); /* song end stops: CC123 on ch 3, 10 */
 sc_start(&e);run(&e,&s,10);np_=0;sc_stop(&e);assert(np_==2 && pk[0]==(8u|0x82u<<8|62u<<16));
 s.midi_mode=0;np_=0;sc_start(&e);run(&e,&s,30000);assert(np_==0);
 /* mode 2: routed sound silent inside, others audible */
 s.midi_mode=2;sc_engine_init(&e,8);e.midi=cap;np_=0;sc_note(&e,&s,(ScNote){0,48,60,0,100,0});run(&e,&s,2000);assert(np_==1 && !e.voices[0].active && e.triggers==1);
 sc_note(&e,&s,(ScNote){0,48,60,5,100,0});assert(e.voices[0].active && np_==1);
 s.midi_mode=1;np_=0;sc_stop(&e);memset(e.voices,0,sizeof e.voices);sc_note(&e,&s,(ScNote){0,48,60,0,100,0});assert(e.voices[0].active);
 /* clock: 24 per beat, 0xFA first, 0xFC last */
 sc_default(&s);s.midi_clock=1;s.end=65535;sc_engine_init(&e,8);e.midi=cap;np_=fr_=0;sc_start(&e);run(&e,&s,22050*4);
 {unsigned c=0;for(i=0;i<np_;i++)c+=pk[i]==(0xFu|0xF8u<<8);assert(pk[0]==(0xFu|0xFAu<<8) && c==4*24+1);}
 np_=0;sc_stop(&e);assert(np_==1 && pk[0]==(0xFu|0xFCu<<8));np_=0;sc_stop(&e);assert(np_==0);
 /* pending table overflow: 33 simultaneous notes, first off sent early */
 sc_default(&s);s.midi_mode=1;s.midi_ch[0]=1;sc_engine_init(&e,8);e.midi=cap;np_=0;
 for(i=0;i<SC_MIDIQ;i++)sc_note(&e,&s,(ScNote){0,96,36+i,0,100,0});assert(np_==SC_MIDIQ && e.np==SC_MIDIQ);
 sc_note(&e,&s,(ScNote){0,96,70,0,100,0});assert(np_==SC_MIDIQ+2 && e.np==SC_MIDIQ && pk[SC_MIDIQ]>>24==0 && (pk[SC_MIDIQ]&0xFFFF)==(8u|0x80u<<8));
 sc_note(&e,&s,(ScNote){0,96,70,0,100,0});assert(np_==SC_MIDIQ+4 && e.np==SC_MIDIQ); /* same note again: its off first */
 np_=0;sc_stop(&e);assert(np_==SC_MIDIQ+1 && e.np==0);

 /* external clock sync (midi_clock 2): follows the clocks, not the song tempo */
 sc_default(&s);s.midi_clock=2;n=sc_encode(&s,bytes);assert(n && sc_decode(&t,bytes,n) && t.midi_clock==2);s.midi_clock=3;assert(!sc_valid(&s));
 {double bpms[2]={120,90};
  for(i=0;i<2;i++){
   ext(&s,&e,i?120:77);sc_add(&s,(ScNote){0,24,60,0,100,0});feed(&e,&s,bpms[i],30,0);sc_ext_start(&e,&s,0);assert(e.playing && e.xw && e.tick==0 && e.triggers==0);
   run(&e,&s,64);assert(e.tick==0 && e.triggers==0); /* waits for the first clock */
   ck_=0;feed(&e,&s,bpms[i],1,1);run(&e,&s,1);assert(e.triggers==1 && !e.xw);
   feed(&e,&s,bpms[i],64*4*24-1,1); /* 64 bars of 4/4 */
   assert(e.xinc/96>bpms[i]*0.99 && e.xinc/96<bpms[i]*1.01);printf("ext clock %g BPM (song %u): 64 bars, worst |tick-4*clocks| = %.2f ticks, measured %.2f BPM\n",bpms[i],s.tempo,worst_,e.xinc/96);
  }}
 /* stop, continue, start */
 ext(&s,&e,120);feed(&e,&s,100,30,0);sc_ext_start(&e,&s,0);ck_=0;feed(&e,&s,100,100,1);
 {unsigned tk=e.tick;assert(e.playing && tk>380);sc_stop(&e);assert(!e.playing);feed(&e,&s,100,24,0);assert(e.tick==tk && !e.playing); /* clocks after Stop do nothing */
  sc_ext_start(&e,&s,1);assert(e.playing && e.xw && e.tick==tk);run(&e,&s,64);assert(e.tick==tk);feed(&e,&s,100,51,0);assert(e.tick>=tk+196 && e.tick<=tk+204); /* continue: from the playhead, at the next clock */
  sc_ext_start(&e,&s,1);assert(!e.xw); /* already playing: ignored */
  sc_ext_start(&e,&s,0);assert(e.tick==0 && e.xw);}
 /* tempo change 100 -> 150 BPM mid-song: locked again within 3 beats */
 ext(&s,&e,120);feed(&e,&s,100,30,0);sc_ext_start(&e,&s,0);ck_=0;feed(&e,&s,100,48,1);
 feed(&e,&s,150,72,0);assert(e.xinc/96>148 && e.xinc/96<152);worst_=0;feed(&e,&s,150,96,1);printf("ext clock tempo change 100->150: measured %.2f BPM, locked after 3 beats (worst %.2f ticks)\n",e.xinc/96,worst_);
 /* clocks lost for 1 s: plays on at the last tempo, relocks; IN sends no clock out */
 ext(&s,&e,120);e.midi=cap;np_=0;feed(&e,&s,120,30,0);sc_ext_start(&e,&s,0);feed(&e,&s,120,48,0);
 {unsigned tk=e.tick;run(&e,&s,44100);assert(e.playing && e.tick>=tk+100 && e.tick<=tk+195);nt_+=1e6; /* lock holds back to 1/4 speed for 500 ms, then free-runs */
  feed(&e,&s,120,48,0);assert(e.playing && pos(&e)-(double)e.xt<2 && pos(&e)-(double)e.xt>-2);}
 for(i=0;i<np_;i++)assert(pk[i]!=(0xFu|0xF8u<<8) && pk[i]!=(0xFu|0xFAu<<8));
 sc_stop(&e);sc_start(&e);assert(!e.xl);run(&e,&s,44100);assert(e.tick>=190 && e.tick<=194); /* local PLAY: free-runs at the measured tempo */
 if(argc>1){FILE *fp=fopen(argv[1],"rb");assert(fp);n=fread(bytes,1,sizeof bytes,fp);fclose(fp);assert(sc_decode(&s,bytes,n));assert(s.count==11);assert(s.notes[0].pitch==60);}
 if(argc>2){FILE *fp=fopen(argv[2],"rb");assert(fp);n=fread(bytes,1,sizeof bytes,fp);fclose(fp);assert(sc_decode(&s,bytes,n) && s.count==4 && s.notes[0].pitch==58 && s.notes[0].layer==SC_FLAT && s.notes[1].layer==0 && s.notes[2].pitch==63 && s.notes[2].layer==SC_FLAT && s.notes[3].layer==0 && sc_valid(&s));} /* importer v2 with flats */
 printf("sizeof song=%zu, note=%zu, engine=%zu, voice=%zu; wire max=%d; history=%zu\n",sizeof s,sizeof(ScNote),sizeof e,sizeof(ScVoice),SC_BYTES,17*sizeof s);
 for(i=8;i<=16;i+=4){clock_t begin;double sec;sc_default(&s);sc_engine_init(&e,i);begin=clock();
  for(j=0;j<4000;j++){unsigned k;for(k=0;k<i;k++)sc_note(&e,&s,(ScNote){0,96,36+k,k%SC_SOUNDS,100,0});sc_render(&e,&s,lr,256,4096);}
  sec=(double)(clock()-begin)/CLOCKS_PER_SEC;printf("HOST voices=%u audio=%.3fs wallCPU=%.4fs load=%.3f%% steals=%u (not device benchmark)\n",i,4000*256/44100.0,sec,sec/(4000*256/44100.0)*100,e.steals);
 }
 puts("core behavior checks passed");return 0;
}
