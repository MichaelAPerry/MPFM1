/* SPDX-License-Identifier: MIT. Native fm1-base application; no desktop UI runtime. */
#include "dsp/stamp.h"
#include <string.h>
#include <stdio.h>
static ScSong song;
static ScEngine engine;
static PLAT_POOL ScSong history[17];
static unsigned hist_count,hist_pos;
static uint8_t wire[SC_BYTES];
static struct { unsigned cursor,instrument,layer,grid,velocity,length,slot,dirty,page,filter,erase,confirm; int octave,last; uint32_t keys,buttons,until; char message[32]; } ui;
static void checkpoint(void) {
 if(hist_pos<16){history[hist_pos++]=song;}else{memmove(history,history+1,15*sizeof *history);history[15]=song;}
 hist_count=hist_pos;ui.dirty=1;
}
static void undo(void){if(hist_pos){if(hist_pos==hist_count)history[hist_pos]=song;song=history[--hist_pos];ui.dirty=1;}}
static void redo(void){if(hist_pos<hist_count){song=history[++hist_pos];ui.dirty=1;}}
void app_say(const char *a,const char *b){snprintf(ui.message,sizeof ui.message,"%s %s",a?a:"",b?b:"");ui.until=plat_ms()+1600;}
void app_panic(void){sc_stop(&engine);}
void app_render(int32_t *lr,uint32_t n,uint32_t master){sc_render(&engine,&song,lr,n,master);}
void app_init(void){
 int n;memset(&ui,0,sizeof ui);ui.grid=96;ui.length=96;ui.velocity=100;ui.last=60;hist_count=hist_pos=0;
 sc_default(&song);sc_engine_init(&engine,8);
 n=plat_store_load(OBJ_SONG0,wire,sizeof wire);if(n>0)sc_decode(&song,wire,n);
}
static void enter(unsigned pitch){
 unsigned i;ScNote note={(uint16_t)ui.cursor,(uint16_t)ui.length,pitch,ui.instrument,ui.velocity,ui.layer};ui.last=pitch;
 if(engine.playing){app_say("STOP TO EDIT","");return;}
 if(ui.erase){for(i=0;i<song.count;i++)if(song.notes[i].tick==ui.cursor && song.notes[i].pitch==pitch && song.notes[i].instrument==ui.instrument && song.notes[i].layer==ui.layer){checkpoint();memmove(song.notes+i,song.notes+i+1,(song.count-i-1)*sizeof(ScNote));song.count--;return;}return;}
 if(song.count>=SC_EVENTS){app_say("SONG FULL","");return;}
 checkpoint();if(sc_add(&song,note))sc_note(&engine,&song,note);
}
static void input(void){
 uint32_t keys=plat_keys(),btn=plat_buttons(),pressed=btn&~ui.buttons,f=plat_lock();unsigned k;int d;
 ui.erase=(btn>>B_EDIT)&1u;
 for(k=0;k<NKEYS;k++)if((keys&~ui.keys)&(1u<<k)){int pitch=53+(int)k+12*ui.octave;if(pitch>=36 && pitch<=84)enter(pitch);else app_say("C2 TO C6","");}
 if(pressed&(1u<<B_PLAY)){if(engine.playing)sc_stop(&engine);else sc_start(&engine);}
 if(pressed&(1u<<B_OCTDN))ui.octave=clamp(ui.octave-1,-2,2);
 if(pressed&(1u<<B_OCTUP))ui.octave=clamp(ui.octave+1,-2,2);
 if(pressed&(1u<<B_GLO)){ui.page=ui.page==2?0:2;ui.confirm=0;}
 if(pressed&(1u<<B_HOME)){ui.page=0;ui.cursor=0;ui.confirm=0;}
 if(pressed&(1u<<B_SEL))ui.filter^=1;
 if(pressed&(1u<<B_EDIT))ui.page=ui.page==1?0:1;
 if(!engine.playing){
  if(pressed&(1u<<B_ENV))undo();
  if(pressed&(1u<<B_LFO))redo();
  if(pressed&(1u<<B_SEQ)){ui.grid=ui.grid==96?48:ui.grid==48?24:96;ui.cursor=ui.cursor/ui.grid*ui.grid;}
  if(pressed&(1u<<B_ARP)){checkpoint();song.loop^=1;}
  if(pressed&(1u<<B_FX)){checkpoint();song.volume[ui.instrument]=song.volume[ui.instrument]?0:127;}
  if(pressed&(1u<<B_REC)){
   if(ui.page==2){
    if(ui.confirm!=ui.slot+1){ui.confirm=ui.slot+1;app_say("REC AGAIN TO LOAD","");}
    else{int n;plat_unlock(f);n=plat_store_load(ui.slot,wire,sizeof wire);f=plat_lock();checkpoint();if(n>0 && sc_decode(&song,wire,n)){ui.dirty=0;ui.cursor=0;app_say("LOADED","");}else app_say("EMPTY OR BAD SLOT","");ui.confirm=0;}
   }else{unsigned bar=song.beats*96,start=ui.cursor/bar*bar;checkpoint();if(!sc_duplicate(&song,start,bar)){undo();app_say("COPY FAILED","");}else app_say("BAR COPIED","");}
  }
  if(pressed&(1u<<B_SAVE)){unsigned n=sc_encode(&song,wire);int rc;sc_stop(&engine);plat_unlock(f);rc=n?plat_store_save(ui.slot,wire,n):-1;f=plat_lock();if(!rc){ui.dirty=0;app_say("SAVED","");}else app_say("SAVE FAILED","");}
 }else if(pressed&((1u<<B_SAVE)|(1u<<B_REC)|(1u<<B_ENV)|(1u<<B_LFO)))app_say("STOP FIRST","");
 ui.erase=(btn>>B_EDIT)&1u;
 if((d=plat_enc(EN_SELECT)))ui.cursor=clamp((int)ui.cursor+d*(int)ui.grid,0,(song.end-1)/ui.grid*ui.grid);
 if((d=plat_enc(EN_PRESET)))ui.instrument=clamp((int)ui.instrument+d,0,14);
 if((d=plat_enc(EN_ALGO)))ui.layer=clamp((int)ui.layer+d,0,7);
 if((d=plat_enc(EN_K1))){if(ui.page==2){ui.slot=clamp((int)ui.slot+d,0,7);ui.confirm=0;}else if(!engine.playing){checkpoint();song.tempo=clamp(song.tempo+d,30,300);}}
 if((d=plat_enc(EN_K2))){if(ui.page==1 && !engine.playing){checkpoint();song.volume[ui.instrument]=clamp(song.volume[ui.instrument]+d,0,127);}else ui.velocity=clamp((int)ui.velocity+d,1,127);}
 if((d=plat_enc(EN_K3))){if(ui.page==1 && !engine.playing){checkpoint();song.layers[ui.layer]=clamp(song.layers[ui.layer]+d,0,127);}else ui.length=clamp((int)ui.length+d*24,24,1536);}
 if((d=plat_enc(EN_K4)) && !engine.playing){unsigned minend=song.count?song.notes[song.count-1].tick+1:1;checkpoint();song.end=clamp(song.end+d*96,minend>song.loop_start?minend:(unsigned)song.loop_start+1,65535);if(ui.cursor>=song.end)ui.cursor=(song.end-1)/ui.grid*ui.grid;}
 ui.keys=keys;ui.buttons=btn;plat_unlock(f);plat_glow(1);plat_leds((engine.playing?1u<<B_PLAY:0)|(ui.dirty?1u<<B_SAVE:0),keys);
}
void app_input_only(void){input();}
static int diatonic(unsigned p){static const uint8_t d[12]={0,0,1,1,2,3,3,4,4,5,5,6};return (int)(p/12)*7+d[p%12];}
/* Original geometric stamps, 15 distinct 7x7 masks, not copied character sprites. */
static const uint8_t icons[15][7]={{127,85,85,127,73,73,127},{28,62,127,127,127,62,28},{64,32,24,12,6,2,1},{8,28,28,62,62,127,8},{1,3,127,127,127,3,1},{120,72,79,9,121,72,120},{34,119,127,85,127,62,28},{65,99,127,85,127,62,28},{62,127,85,127,62,42,28},{3,7,14,28,56,127,127},{28,62,85,127,93,62,28},{2,4,28,62,127,62,28},{127,65,65,65,65,65,127},{127,85,85,85,85,85,127},{8,28,62,127,127,62,28}};
static void stamp(int x,int y,unsigned id,uint16_t color){unsigned a,b;for(a=0;a<7;a++)for(b=0;b<7;b++)if(icons[id][a]&(1u<<b))cv_rect(x+b,y+a,1,1,color);}
static void draw(void){
 char t[64];unsigned i,step=ui.grid,active=engine.playing?engine.tick:ui.cursor,start=active/(step*8)*step*8;int center=41;
 if(ui.last<59)center-=14;else if(ui.last>79)center+=7;
 cv_begin(240,80,RGB(15,23,43));cv_text(8,4,&FONT_B,"STAMP COMPOSER",C_WHITE);
 snprintf(t,sizeof t,"%s %u BPM  %s",engine.playing?"PLAY":"EDIT",song.tempo,song.loop?"LOOP":"ONCE");cv_text(8,26,&FONT_XS,t,C_HI);
 stamp(9,50,ui.instrument,C_HI);cv_text(23,46,&FONT_B,sc_names[ui.instrument],C_WHITE);
 snprintf(t,sizeof t,"L%u  %u/%u  %s",ui.layer+1,song.count,SC_EVENTS,ui.dirty?"*":"");cv_text(8,65,&FONT_XS,t,C_GRAY);cv_blit(0,0);
 cv_begin(240,80,RGB(249,241,211));
 for(i=0;i<5;i++)cv_rect(6,20+i*10,228,1,RGB(145,145,132));
 for(i=0;i<8;i++)cv_rect(14+i*28,5,1,70,RGB(216,207,181));
 for(i=0;i<song.count;i++){ScNote n=song.notes[i];int y;if(n.tick<start || n.tick>=start+step*8 || (ui.filter && n.instrument!=ui.instrument))continue;y=40-(diatonic(n.pitch)-center)*5;if(y<4 || y>68)continue;stamp(11+(n.tick-start)*28/step,y,n.instrument,n.instrument==ui.instrument?RGB(10,100,110):RGB(130,77,51));if(n.pitch%12==1 || n.pitch%12==3 || n.pitch%12==6 || n.pitch%12==8 || n.pitch%12==10)cv_text(4+(n.tick-start)*28/step,y-4,&FONT_XS,"#",C_BLACK);}
 cv_rect(12+(active-start)*28/step,0,9,3,C_HI);if(engine.playing)cv_rect(15+(active-start)*28/step,3+(plat_ms()/100%2)*2,3,3,RGB(230,90,70));cv_blit(0,80);
 cv_begin(240,80,RGB(15,23,43));
 if(ui.page==2){snprintf(t,sizeof t,"FILE SLOT %u / 8",ui.slot+1);cv_text(8,4,&FONT_B,t,C_HI);cv_text(8,25,&FONT_XS,"K1 SLOT  SAVE WRITE",C_WHITE);cv_text(8,42,&FONT_XS,"REC TWICE LOAD  HOME EXIT",C_GRAY);}
 else{snprintf(t,sizeof t,"BAR %u  GRID 1/%u  OCT %+d",ui.cursor/(song.beats*96)+1,384/step,ui.octave);cv_text(8,4,&FONT_XS,t,C_HI);snprintf(t,sizeof t,ui.page==1?"SOUND VOL %u  LAYER VOL %u":"VEL %u  LENGTH %u TICKS",ui.page==1?song.volume[ui.instrument]:ui.velocity,ui.page==1?song.layers[ui.layer]:ui.length);cv_text(8,23,&FONT_XS,t,C_WHITE);snprintf(t,sizeof t,"END %u  %s",song.end,ui.filter?"FILTER":"ALL SOUNDS");cv_text(8,40,&FONT_XS,t,C_GRAY);}
 cv_text(8,62,&FONT_XS,plat_ms()<ui.until?ui.message:"PLACEHOLDER SOUND BANK",C_HI);cv_blit(0,160);
}
void app_ui_init(void){draw();}
void app_frame(void){input();draw();}
int app_expect(const char *s,int *v){
 if(!strcmp(s,"notes"))*v=song.count;else if(!strcmp(s,"playing"))*v=engine.playing;else if(!strcmp(s,"dirty"))*v=ui.dirty;else if(!strcmp(s,"instrument"))*v=ui.instrument;else if(!strcmp(s,"tick"))*v=engine.tick;else if(!strcmp(s,"triggers"))*v=engine.triggers;else if(!strcmp(s,"tempo"))*v=song.tempo;else if(!strcmp(s,"cursor"))*v=ui.cursor;else return 0;return 1;
}
#ifdef PLAT_WEB
/* Export only for the browser host; device transfer will use a separate transport. */
#undef __attribute__
#define SC_WEB __attribute__((used, visibility("default")))
SC_WEB uint8_t *stamp_buffer(void){return wire;}
SC_WEB unsigned stamp_export(void){return sc_encode(&song,wire);}
SC_WEB int stamp_import(unsigned n){
 ScSong candidate;
 if(engine.playing || n>sizeof wire || !sc_decode(&candidate,wire,n))return 0;
 checkpoint();song=candidate;ui.cursor=0;ui.confirm=0;app_say("IMPORTED", "SAVE TO KEEP");return 1;
}
SC_WEB unsigned stamp_count(void){return song.count;}
#endif
