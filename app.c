/* SPDX-License-Identifier: MIT. Native fm1-base application; no desktop UI runtime. */
#include "dsp/stamp.h"
#include "dsp/libc_shim.h"
static ScSong song;
static ScEngine engine;
static PLAT_POOL ScSong history[17];
static unsigned hist_count,hist_pos;
static uint8_t wire[SC_BYTES];
static struct { unsigned cursor,instrument,layer,grid,velocity,length,slot,dirty,page,filter,erase,confirm,edit_used; int octave,last; uint32_t keys,buttons,until; char message[32]; } ui;
static void checkpoint(void) {
 if(hist_pos<16){history[hist_pos++]=song;}else{memmove(history,history+1,15*sizeof *history);history[15]=song;}
 hist_count=hist_pos;ui.dirty=1;
}
static void undo(void){if(hist_pos){if(hist_pos==hist_count)history[hist_pos]=song;song=history[--hist_pos];ui.dirty=1;}}
static void redo(void){if(hist_pos<hist_count){song=history[++hist_pos];ui.dirty=1;}}
void app_say(const char *a,const char *b){snprintf(ui.message,sizeof ui.message,"%s %s",a?a:"",b?b:"");ui.until=plat_ms()+1600;ann_say(a,b,ANN_NOW);}
void app_panic(void){sc_stop(&engine);memset(engine.voices,0,sizeof engine.voices);}
void app_render(int32_t *lr,uint32_t n,uint32_t master){sc_render(&engine,&song,lr,n,master);}
void app_init(void){
 int n;memset(&ui,0,sizeof ui);ui.grid=96;ui.length=96;ui.velocity=100;ui.last=60;hist_count=hist_pos=0;
 sc_default(&song);sc_engine_init(&engine,8);
 n=plat_store_load(OBJ_SONG0,wire,sizeof wire);if(n>0)sc_decode(&song,wire,n);
}
static int load_demo(void){
 static const uint8_t melody[16]={60,64,67,72,71,67,64,62,65,69,72,76,74,72,67,60};
 unsigned i;if(engine.playing)return 0;checkpoint();sc_default(&song);song.loop=1;
 for(i=0;i<16;i++){
  sc_add(&song,(ScNote){i*96,72,melody[i],i<8?0:3,110,0});
  if(i%2==0){sc_add(&song,(ScNote){i*96,144,i<8?48:53,14,95,1});sc_add(&song,(ScNote){i*96,48,48,1,95,2});}
 }
 ui.cursor=0;ui.page=0;ui.instrument=0;ui.last=67;ui.confirm=0;app_say("PLAYGROUND", "DEMO LOADED");return 1;
}
#ifdef PLAT_HOST
int app_host_cmd(const char *cmd,const char *a,const char *b,const char *c,const char *d){
 (void)a;(void)b;(void)c;(void)d;
 if(!strcmp(cmd,"demo")){load_demo();return 1;}return 0;
}
#endif
static void enter(unsigned pitch){
 unsigned i;ScNote note={(uint16_t)ui.cursor,(uint16_t)ui.length,pitch,ui.instrument,ui.velocity,ui.layer};ui.last=pitch;
 if(engine.playing){app_say("STOP TO EDIT","");return;}
 if(ui.erase){ui.edit_used=1;for(i=0;i<song.count;i++)if(song.notes[i].tick==ui.cursor && song.notes[i].pitch==pitch && song.notes[i].instrument==ui.instrument && song.notes[i].layer==ui.layer){checkpoint();memmove(song.notes+i,song.notes+i+1,(song.count-i-1)*sizeof(ScNote));song.count--;return;}return;}
 if(song.count>=SC_EVENTS){app_say("SONG FULL","");return;}
 checkpoint();if(sc_add(&song,note))sc_note(&engine,&song,note);
}
static void input(void){
 uint32_t keys=plat_keys(),btn=plat_buttons(),pressed=btn&~ui.buttons,f=plat_lock();unsigned k;int d;
 ui.erase=(btn>>B_EDIT)&1u;
 if(pressed&(1u<<B_EDIT))ui.edit_used=0;
 for(k=0;k<NKEYS;k++)if((keys&~ui.keys)&(1u<<k)){int pitch=53+(int)k+12*ui.octave;if(pitch>=36 && pitch<=84)enter(pitch);else app_say("C2 TO C6","");}
 if(pressed&(1u<<B_PLAY)){if(engine.playing)sc_stop(&engine);else sc_start(&engine);}
 if(pressed&(1u<<B_OCTDN))ui.octave=clamp(ui.octave-1,-2,2);
 if(pressed&(1u<<B_OCTUP))ui.octave=clamp(ui.octave+1,-2,2);
 if(pressed&(1u<<B_GLO)){ui.page=ui.page==2?0:2;ui.confirm=0;}
 if(pressed&(1u<<B_HOME)){ui.page=0;ui.cursor=0;ui.confirm=0;}
 if(pressed&(1u<<B_SEL))ui.filter^=1;
 if((ui.buttons&(1u<<B_EDIT)) && !(btn&(1u<<B_EDIT)) && !ui.edit_used)ui.page=ui.page==1?0:1;
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
 if(ui.cursor>=song.end)ui.cursor=(song.end-1)/ui.grid*ui.grid;
 ui.keys=keys;ui.buttons=btn;plat_unlock(f);plat_glow(1);plat_leds((engine.playing?1u<<B_PLAY:0)|(ui.dirty?1u<<B_SAVE:0),keys);
}
void app_input_only(void){input();}
static int diatonic(unsigned p){static const uint8_t d[12]={0,0,1,1,2,3,3,4,4,5,5,6};return (int)(p/12)*7+d[p%12];}
/* Palette and notes share the exact pixel assets used by the browser. */
#include "assets/stamps.h"
static const char *const character_names[15]={"MARIO","MUSHROOM","YOSHI","STAR","FLOWER","GAME BOY","DOG","CAT","PIG","SWAN","FACE","PLANE","BOAT","CAR","HEART"};
#define INK RGB(39,45,59)
#define PAPER RGB(255,249,229)
#define ACCENT RGB(226,69,62)
#define TEAL RGB(27,121,122)
#define MUTED RGB(157,153,137)
static void stamp(int x,int y,unsigned id,unsigned scale){
 unsigned a,b;
 for(a=0;a<18;a++)for(b=0;b<16;b++){
  uint16_t c=stamp_pixels[id][a*16+b];
  if(c!=1)cv_rect(x+(int)(b*scale),y+(int)(a*scale),scale,scale,c);
 }
}
static void note_label(unsigned pitch,char *out){
 static const char *const names[]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
 pitch%=128;
 snprintf(out,8,"%s%d",names[pitch%12],(int)(pitch/12)-1);
}
static void tile(int x,int y,int w,const char *label,const char *value){
 cv_rect(x,y,w,43,RGB(47,56,70));
 cv_text(x+5,y+3,&FONT_T,label,RGB(162,182,188));
 cv_text(x+5,y+17,&FONT_B,value,C_WHITE);
}
static void draw(void){
 char t[64],note[8];unsigned i,step=ui.grid,active=engine.playing?engine.tick:ui.cursor;
 unsigned start=active/(step*6)*step*6,selected=ui.instrument;
 int center=41,focus=diatonic(ui.last),shift=0;
 while(focus<center-8){center-=7;shift--;}
 while(focus>center+9){center+=7;shift++;}
 note_label(ui.last,note);
 /* Header and five-item instrument ribbon: PRESET moves the window. */
 cv_begin(240,72,INK);
 cv_rect(0,0,4,24,ACCENT);
 cv_text(10,3,&FONT_B,"STAMP",C_WHITE);
 snprintf(t,sizeof t,"%03u",song.tempo);cv_text(140,3,&FONT_B,t,C_WHITE);
 cv_text(168,8,&FONT_T,"BPM",RGB(162,182,188));
 cv_rect(197,4,37,17,engine.playing?TEAL:RGB(71,78,88));
 cv_text(201,5,&FONT_XS,engine.playing?"PLAY":"EDIT",C_WHITE);
 cv_rect(0,25,240,47,PAPER);
 for(i=0;i<5;i++){
  int id=(int)selected+(int)i-2,x=6+(int)i*31;
  if(id<0)id+=15;
  if(id>=15)id-=15;
  if(i==2){cv_rect(x,28,28,24,RGB(250,212,104));cv_rect(x,52,28,2,ACCENT);}
  stamp(x+6,31,(unsigned)id,1);
 }
 cv_text(164,31,&FONT_B,note,INK);
 snprintf(t,sizeof t,"LAYER %u",ui.layer+1);cv_text(164,49,&FONT_T,t,TEAL);
 snprintf(t,sizeof t,"%s / %s",character_names[selected],sc_names[selected]);
 cv_text(8,56,&FONT_XS,t,INK);
 cv_blit(0,0);
 if(ui.page==2){
  cv_begin(240,112,PAPER);
  cv_text(10,4,&FONT_B,"SONG BOOK",INK);
  for(i=0;i<8;i++){
   int x=10+(i%4)*57,y=29+(i/4)*37;
   cv_rect(x,y,49,29,i==ui.slot?TEAL:RGB(233,225,202));
   snprintf(t,sizeof t,"%02u",i+1);cv_text(x+14,y+4,&FONT_B,t,i==ui.slot?C_WHITE:INK);
  }
  cv_text(10,98,&FONT_T,ui.confirm?"LOAD ARMED - REC AGAIN":"K1 SELECTS A SAVE SLOT",TEAL);
  cv_blit(0,72);
 }else if(ui.page==1){
  cv_begin(240,112,PAPER);
  cv_text(10,4,&FONT_B,"MIX & SHAPE",INK);
  cv_text(10,29,&FONT_XS,"K2  SOUND LEVEL",TEAL);
  cv_rect(10,48,180,9,RGB(225,218,197));cv_rect(10,48,song.volume[selected]*180/127,9,ACCENT);
  snprintf(t,sizeof t,"%u",song.volume[selected]);cv_text(197,42,&FONT_B,t,INK);
  cv_text(10,67,&FONT_XS,"K3  LAYER LEVEL",TEAL);
  cv_rect(10,86,180,9,RGB(225,218,197));cv_rect(10,86,song.layers[ui.layer]*180/127,9,TEAL);
  snprintf(t,sizeof t,"%u",song.layers[ui.layer]);cv_text(197,80,&FONT_B,t,INK);
  cv_blit(0,72);
 }else{
  int cursor_x=39+(int)((active-start)*32/step);
  cv_begin(240,112,PAPER);
  snprintf(t,sizeof t,"BAR %02u  %s",active/(song.beats*96)+1,song.loop?"LOOP":"ONCE");
  cv_text(7,0,&FONT_T,t,TEAL);
  snprintf(t,sizeof t,"%s%+d OCT",ui.filter?"SOLO VIEW ":"",shift);cv_text(133,0,&FONT_T,t,MUTED);
  cv_rect(cursor_x-1,18,22,85,RGB(250,225,184));
  for(i=0;i<5;i++)cv_rect(25,39+i*8,208,1,RGB(177,170,148));
  stamp(5,42,15,1);
  for(i=0;i<6;i++){
   int x=39+(int)i*32;unsigned tick=start+i*step;
   if(tick%(song.beats*96)==0)cv_rect(x-5,29,1,67,RGB(157,153,137));
   snprintf(t,sizeof t,"%u",tick/step+1);cv_text(x+3,99,&FONT_T,t,MUTED);
  }
  for(i=0;i<song.count;i++){
   ScNote n=song.notes[i];int y,x,k;
   if(n.tick<start || n.tick>=start+step*6 || (ui.filter && n.instrument!=selected))continue;
   y=55-(diatonic(n.pitch)-center)*4;x=39+(n.tick-start)*32/step;
   if(y<27 || y>91){cv_text(x+4,y<27?16:88,&FONT_T,y<27?"+":"-",MUTED);continue;}
   if(y>=79)for(k=79;k<=y;k+=8)cv_rect(x-3,k,22,1,MUTED);
   if(y<=31)for(k=31;k>=y;k-=8)cv_rect(x-3,k,22,1,MUTED);
   stamp(x,y-9,n.instrument,1);
   if(n.pitch%12==1 || n.pitch%12==3 || n.pitch%12==6 || n.pitch%12==8 || n.pitch%12==10)cv_text(x-7,y-8,&FONT_T,"#",INK);
  }
  cv_rect(cursor_x-3,28,2,66,ACCENT);
  if(engine.playing)stamp(cursor_x,12-(int)(plat_ms()/120%2)*3,0,1);
  else cv_rect(cursor_x+4,19,5,4,ACCENT);
  if(!song.count)cv_text(40,85,&FONT_XS,"PLAY A KEY TO PLACE A NOTE",TEAL);
  cv_blit(0,72);
 }
 /* Four encoder cards, then one concise contextual hint. */
 cv_begin(240,56,INK);
 snprintf(t,sizeof t,ui.page==2?"%02u":"%u",ui.page==2?ui.slot+1:song.tempo);tile(4,2,55,ui.page==2?"K1 SLOT":"K1 TEMPO",t);
 snprintf(t,sizeof t,"%u",ui.page==1?song.volume[selected]:ui.velocity);tile(63,2,55,ui.page==1?"K2 SOUND":"K2 VEL",t);
 snprintf(t,sizeof t,ui.page==1?"%u":"1/%u",ui.page==1?song.layers[ui.layer]:384/ui.length);if(ui.page!=1 && (ui.length>384 || 384%ui.length!=0))snprintf(t,sizeof t,"%u T",ui.length);tile(122,2,55,ui.page==1?"K3 LAYER":"K3 LEN",t);
 snprintf(t,sizeof t,"%u BAR",(song.end+song.beats*96-1)/(song.beats*96));tile(181,2,55,"K4 END",t);
 if(plat_ms()<ui.until)cv_text(5,44,&FONT_T,ui.message,RGB(250,212,104));
 else{snprintf(t,sizeof t,ui.page==2?"SAVE WRITE / REC LOAD / HOME EXIT":ui.page==1?"EDIT BACK / FX MUTE / HOME STAFF":"1/%u GRID / %u NOTES / %s",384/step,song.count,ui.dirty?"UNSAVED":"SAVED");cv_text(5,44,&FONT_T,t,RGB(162,182,188));}
 cv_blit(0,184);
}
void app_ui_init(void){draw();}
void app_frame(void){
 input();draw();
 if(ann_on() && ann_where()){
  char t[160];snprintf(t,sizeof t,"Stamp Composer. %s. %s. %u notes. Tempo %u. Layer %u. %s.",
   ui.page==2?"Song book":ui.page==1?"Mixer":"Staff",character_names[ui.instrument],song.count,song.tempo,ui.layer+1,engine.playing?"Playing":"Stopped");
  ann_say(t,0,ANN_NOW);
 }
 ann_tick(plat_ms());
}
int app_expect(const char *s,int *v){
 if(!strcmp(s,"page"))*v=ui.page;else if(!strcmp(s,"notes"))*v=song.count;else if(!strcmp(s,"playing"))*v=engine.playing;else if(!strcmp(s,"dirty"))*v=ui.dirty;else if(!strcmp(s,"instrument"))*v=ui.instrument;else if(!strcmp(s,"tick"))*v=engine.tick;else if(!strcmp(s,"triggers"))*v=engine.triggers;else if(!strcmp(s,"tempo"))*v=song.tempo;else if(!strcmp(s,"cursor"))*v=ui.cursor;else return 0;return 1;
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
SC_WEB unsigned stamp_playing(void){return engine.playing;}
SC_WEB unsigned stamp_instrument(void){return ui.instrument;}
SC_WEB unsigned stamp_dirty(void){return ui.dirty;}
SC_WEB unsigned stamp_position(void){return engine.playing?engine.tick:ui.cursor;}
SC_WEB int stamp_select(unsigned id){if(id>=15)return 0;ui.instrument=id;return 1;}
SC_WEB int stamp_demo(void){return load_demo();}
SC_WEB void stamp_transport(void){if(engine.playing)sc_stop(&engine);else sc_start(&engine);}
#endif

