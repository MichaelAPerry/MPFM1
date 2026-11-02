/* SPDX-License-Identifier: MIT */
#include "stamp.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
static int32_t lr[512];
static void render(ScEngine *e,ScSong *s,unsigned n){while(n){unsigned b=n>256?256:n;sc_render(e,s,lr,b,4096);n-=b;}}
static uint32_t gold(void){
 ScSong s;ScEngine e;unsigned i,f;uint32_t h=2166136261u;static int32_t o[512];
 sc_default(&s);s.tempo=137;s.end=384;s.loop=1;s.loop_start=96;
 for(i=0;i<60;i++)sc_add(&s,(ScNote){i*6,6+i%40,(uint8_t)(36+(i*7)%49),i%15,(uint8_t)(40+i%88),i%8});
 sc_engine_init(&e,8);sc_start(&e);
 for(f=0;f<44100*6;f+=256){sc_render(&e,&s,o,256,4096);for(i=0;i<512;i++){h^=(uint32_t)o[i];h*=16777619u;}}
 return h;}

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
int main(int argc,char **argv){
 ScSong s,t;ScEngine e;uint8_t bytes[SC_BYTES];unsigned n,i,j;
 sc_default(&s);assert(sc_add(&s,(ScNote){0,96,60,0,100,0}));assert(sc_add(&s,(ScNote){96,96,64,6,100,1}));n=sc_encode(&s,bytes);assert(n==146);assert(sc_decode(&t,bytes,n));assert(!memcmp(&s,&t,sizeof s));bytes[49]^=1;assert(!sc_decode(&t,bytes,n));assert(!memcmp(&s,&t,sizeof s));
 for(i=30;i<=300;i+=27){sc_default(&s);s.tempo=i;s.end=65535;sc_engine_init(&e,16);sc_start(&e);render(&e,&s,44100*7);assert(e.tick==i*96u*7/60);}
 for(i=24;i<=96;i*=2){sc_default(&s);s.end=i*3;sc_add(&s,(ScNote){0,24,60,0,100,0});sc_add(&s,(ScNote){i,24,64,3,100,0});sc_engine_init(&e,8);sc_start(&e);render(&e,&s,(44100*i+191)/(96*2));assert(e.triggers==1);render(&e,&s,1);assert(e.triggers==2);}
 sc_default(&s);s.end=192;sc_add(&s,(ScNote){0,96,60,0,100,0});sc_add(&s,(ScNote){96,96,64,3,100,0});sc_engine_init(&e,8);sc_start(&e);render(&e,&s,44100);assert(!e.playing && e.triggers==2);
 s.loop=1;s.loop_start=96;sc_start(&e);e.triggers=0;render(&e,&s,44100+22050+1);assert(e.playing && e.triggers==4);
 sc_engine_init(&e,8);for(i=0;i<20;i++)sc_note(&e,&s,(ScNote){0,96,60,i%15,127,0});assert(e.steals==12);render(&e,&s,44100*4);for(i=0;i<8;i++)assert(!e.voices[i].active && e.voices[i].last==0);
 sc_default(&s);for(i=0;i<384;i++)assert(sc_add(&s,(ScNote){i,24,60,i%15,100,i%8}));t=s;assert(!sc_duplicate(&s,0,384));assert(!memcmp(&s,&t,sizeof s));assert(!sc_add(&s,(ScNote){0,24,60,0,100,0}));

 /* v1 / v2 wire */
 sc_default(&s);sc_add(&s,(ScNote){0,96,60,0,100,0});n=sc_encode(&s,bytes);memmove(bytes+48,bytes+126,12);n=48+8+4;bytes[4]=1;fix(bytes,n);assert(sc_decode(&t,bytes,n));assert(!memcmp(&s,&t,sizeof s));assert(t.tone[14]==127 && !t.tune[3] && !t.midi_mode);
 sc_default(&s);for(i=0;i<15;i++){s.tune[i]=i-7;s.tone[i]=i*9;s.decay[i]=127-i;s.pan[i]=i*9-64;s.midi_ch[i]=i+1;}s.midi_mode=2;s.midi_clock=1;sc_add(&s,(ScNote){5,9,40,2,90,3});
 n=sc_encode(&s,bytes);assert(n==130+8 && bytes[4]==2);assert(sc_decode(&t,bytes,n));assert(!memcmp(&s,&t,sizeof s));
 for(i=0;i<7;i++){unsigned off[7]={48+0,63,78,93,108,123,124},bad[7]={13,128,128,64,17,3,2};uint8_t k=bytes[off[i]];bytes[off[i]]=bad[i];fix(bytes,n);assert(!sc_decode(&t,bytes,n));bytes[off[i]]=k;} /* tune,tone,decay,pan,ch,mode,clock */
 bytes[48]=-13;fix(bytes,n);assert(!sc_decode(&t,bytes,n));bytes[48]=-7;bytes[93]=-65;fix(bytes,n);assert(!sc_decode(&t,bytes,n));bytes[93]=-64;bytes[125]=1;fix(bytes,n);assert(!sc_decode(&t,bytes,n));bytes[125]=0;fix(bytes,n);assert(sc_decode(&t,bytes,n));
 bytes[60]^=1;assert(!sc_decode(&t,bytes,n));bytes[60]^=1;assert(!sc_decode(&t,bytes,n-1));
 s.midi_ch[0]=17;assert(!sc_encode(&s,bytes));s.midi_ch[0]=1;s.pan[0]=64;assert(!sc_encode(&s,bytes));
 /* defaults render exactly as before the shaping/MIDI work */
 assert(gold()==0xdf51e7e3u);
 /* tune, tone, decay, pan */
 sc_default(&s);sc_engine_init(&e,8);s.tune[3]=12;sc_note(&e,&s,(ScNote){0,24,36,3,127,0});s.tune[3]=0;sc_note(&e,&s,(ScNote){0,24,48,3,127,0});assert(e.voices[0].inc==e.voices[1].inc && e.voices[0].inc>8192);
 sc_engine_init(&e,8);s.tune[3]=-12;sc_note(&e,&s,(ScNote){0,24,50,3,127,0});sc_note(&e,&s,(ScNote){0,24,36,3,127,0});assert(e.voices[0].inc>e.voices[1].inc && e.voices[1].inc==8192);
 s.tune[3]=12;sc_note(&e,&s,(ScNote){0,24,84,3,127,0});assert(e.voices[2].inc==131072);
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
 if(argc>1){FILE *fp=fopen(argv[1],"rb");assert(fp);n=fread(bytes,1,sizeof bytes,fp);fclose(fp);assert(sc_decode(&s,bytes,n));assert(s.count==11);assert(s.notes[0].pitch==60);}
 printf("sizeof song=%zu, note=%zu, engine=%zu, voice=%zu; wire max=%d; history=%zu\n",sizeof s,sizeof(ScNote),sizeof e,sizeof(ScVoice),SC_BYTES,17*sizeof s);
 for(i=8;i<=16;i+=4){clock_t begin;double sec;sc_default(&s);sc_engine_init(&e,i);begin=clock();
  for(j=0;j<4000;j++){unsigned k;for(k=0;k<i;k++)sc_note(&e,&s,(ScNote){0,96,36+k,k%15,100,0});sc_render(&e,&s,lr,256,4096);}
  sec=(double)(clock()-begin)/CLOCKS_PER_SEC;printf("HOST voices=%u audio=%.3fs wallCPU=%.4fs load=%.3f%% steals=%u (not device benchmark)\n",i,4000*256/44100.0,sec,sec/(4000*256/44100.0)*100,e.steals);
 }
 puts("core behavior checks passed");return 0;
}
