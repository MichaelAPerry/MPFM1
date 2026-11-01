/* SPDX-License-Identifier: MIT */
#include "stamp.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
static int32_t lr[512];
static void render(ScEngine *e,ScSong *s,unsigned n){while(n){unsigned b=n>256?256:n;sc_render(e,s,lr,b,4096);n-=b;}}
int main(int argc,char **argv){
 ScSong s,t;ScEngine e;uint8_t bytes[SC_BYTES];unsigned n,i,j;
 sc_default(&s);assert(sc_add(&s,(ScNote){0,96,60,0,100,0}));assert(sc_add(&s,(ScNote){96,96,64,6,100,1}));n=sc_encode(&s,bytes);assert(n==68);assert(sc_decode(&t,bytes,n));assert(!memcmp(&s,&t,sizeof s));bytes[49]^=1;assert(!sc_decode(&t,bytes,n));assert(!memcmp(&s,&t,sizeof s));
 for(i=30;i<=300;i+=27){sc_default(&s);s.tempo=i;s.end=65535;sc_engine_init(&e,16);sc_start(&e);render(&e,&s,44100*7);assert(e.tick==i*96u*7/60);}
 for(i=24;i<=96;i*=2){sc_default(&s);s.end=i*3;sc_add(&s,(ScNote){0,24,60,0,100,0});sc_add(&s,(ScNote){i,24,64,3,100,0});sc_engine_init(&e,8);sc_start(&e);render(&e,&s,(44100*i+191)/(96*2));assert(e.triggers==1);render(&e,&s,1);assert(e.triggers==2);}
 sc_default(&s);s.end=192;sc_add(&s,(ScNote){0,96,60,0,100,0});sc_add(&s,(ScNote){96,96,64,3,100,0});sc_engine_init(&e,8);sc_start(&e);render(&e,&s,44100);assert(!e.playing && e.triggers==2);
 s.loop=1;s.loop_start=96;sc_start(&e);e.triggers=0;render(&e,&s,44100+22050+1);assert(e.playing && e.triggers==4);
 sc_engine_init(&e,8);for(i=0;i<20;i++)sc_note(&e,&s,(ScNote){0,96,60,i%15,127,0});assert(e.steals==12);render(&e,&s,44100*4);for(i=0;i<8;i++)assert(!e.voices[i].active && e.voices[i].last==0);
 sc_default(&s);for(i=0;i<384;i++)assert(sc_add(&s,(ScNote){i,24,60,i%15,100,i%8}));t=s;assert(!sc_duplicate(&s,0,384));assert(!memcmp(&s,&t,sizeof s));assert(!sc_add(&s,(ScNote){0,24,60,0,100,0}));
 if(argc>1){FILE *fp=fopen(argv[1],"rb");assert(fp);n=fread(bytes,1,sizeof bytes,fp);fclose(fp);assert(sc_decode(&s,bytes,n));assert(s.count==11);assert(s.notes[0].pitch==60);}
 printf("sizeof song=%zu, note=%zu, engine=%zu, voice=%zu; wire max=%d; history=%zu\n",sizeof s,sizeof(ScNote),sizeof e,sizeof(ScVoice),SC_BYTES,17*sizeof s);
 for(i=8;i<=16;i+=4){clock_t begin;double sec;sc_default(&s);sc_engine_init(&e,i);begin=clock();
  for(j=0;j<4000;j++){unsigned k;for(k=0;k<i;k++)sc_note(&e,&s,(ScNote){0,96,36+k,k%15,100,0});sc_render(&e,&s,lr,256,4096);}
  sec=(double)(clock()-begin)/CLOCKS_PER_SEC;printf("HOST voices=%u audio=%.3fs wallCPU=%.4fs load=%.3f%% steals=%u (not device benchmark)\n",i,4000*256/44100.0,sec,sec/(4000*256/44100.0)*100,e.steals);
 }
 puts("core behavior checks passed");return 0;
}
