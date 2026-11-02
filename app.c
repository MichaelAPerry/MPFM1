/* SPDX-License-Identifier: MIT. Native fm1-base application; no desktop UI runtime. */
#include "dsp/stamp.h"
#include "dsp/libc_shim.h"
static ScSong song;
static ScEngine engine;
static PLAT_POOL ScSong history[17];
static unsigned hist_count,hist_pos;
static uint8_t wire[SC_BYTES];
/* Autosave (OBJ_AUTO): 8-byte header (u16 magic, version, slot, flags bit0 dirty, 3 spare) + the working song's SCF bytes */
#define AMAGIC 0x4150u
static PLAT_POOL uint8_t arec[8+SC_BYTES];
static struct { uint32_t sig,seen,act,chk,last; unsigned n; } au; /* sig: last written; seen: last polled; act: last change/key; n: autosaves */
static const char *gname(unsigned g){return g==96?"1/4":g==48?"1/8":g==24?"1/16":g==32?"1/8T":g==16?"1/16T":0;}
#define LSTEP (ui.grid%24?16:24) /* K3 length step follows the grid: 24 ticks, 16 on triplet grids */
static struct { unsigned cursor,instrument,layer,grid,velocity,length,slot,dirty,page,filter,erase,confirm,edit_used,gest,held,help,bf,bt,bg; int octave,last,bx; uint32_t keys,buttons,until,hold; char message[32]; } ui;
static unsigned apack(void){unsigned n=sc_encode(&song,arec+8);arec[0]=AMAGIC&255;arec[1]=AMAGIC>>8;arec[2]=1;arec[3]=ui.slot;arec[4]=ui.dirty&1;return n?n+8:0;}
static uint32_t asig(unsigned n){uint32_t h=2166136261u;while(n)h=(h^arec[--n])*16777619u;return h;}
static void asave(void){unsigned n=apack();au.last=plat_ms();if(n && !plat_store_save(OBJ_AUTO,arec,n)){au.sig=asig(n);au.n++;}} /* a failed write leaves the old record; retried after the rate limit */
/* every 128 ms while stopped: a changed song, slot or dirty flag restarts the idle time; written after 3 s idle and 20 s since the last write */
static void autosave(void){
 uint32_t t=plat_ms(),s;unsigned n;
 if(engine.playing||ui.held){au.act=t;return;}
 if(t-au.chk<128)return;
 au.chk=t;n=apack();s=asig(n);
 if(s!=au.seen){au.seen=s;au.act=t;}
 if(n && s!=au.sig && t-au.act>=3000 && t-au.last>=20000)asave();
}
static void checkpoint(void) {
 if(hist_pos<16){history[hist_pos++]=song;}else{memmove(history,history+1,15*sizeof *history);history[15]=song;}
 hist_count=hist_pos;ui.dirty=1;
}
static void undo(void){if(hist_pos){if(hist_pos==hist_count)history[hist_pos]=song;song=history[--hist_pos];ui.dirty=1;}}
static void redo(void){if(hist_pos<hist_count){song=history[++hist_pos];ui.dirty=1;}}
void app_say(const char *a,const char *b){snprintf(ui.message,sizeof ui.message,"%s %s",a?a:"",b?b:"");ui.until=plat_ms()+1600;ann_say(a,b,ANN_NOW);}
#define CKN (song.midi_clock==2?"IN":song.midi_clock?"OUT":"OFF")
static void midi_out(uint32_t p){plat_midi_out(p);}
void app_panic(void){sc_stop(&engine);memset(engine.voices,0,sizeof engine.voices);}
void app_render(int32_t *lr,uint32_t n,uint32_t master){sc_render(&engine,&song,lr,n,master);}
void app_init(void){
 int n;memset(&ui,0,sizeof ui);ui.grid=96;ui.length=96;ui.velocity=100;ui.last=60;ui.bx=12;hist_count=hist_pos=0;
 sc_default(&song);sc_engine_init(&engine,8);engine.midi=midi_out;
 n=plat_store_load(OBJ_AUTO,arec,sizeof arec); /* restore the working song; any damage falls back to slot 1 */
 if(n>8 && arec[0]==(AMAGIC&255) && arec[1]==AMAGIC>>8 && arec[2]==1 && arec[3]<8 && sc_decode(&song,arec+8,n-8)){ui.slot=arec[3];ui.dirty=arec[4]&1;snprintf(ui.message,sizeof ui.message,"RESTORED");ui.until=plat_ms()+1600;}
 else{sc_default(&song);n=plat_store_load(OBJ_SONG0,wire,sizeof wire);if(n>0)sc_decode(&song,wire,n);}
 au.sig=au.seen=asig(apack());au.act=au.chk=plat_ms();au.last=au.act-20000u;
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
 if(!strcmp(cmd,"demo")){load_demo();return 1;}if(!strcmp(cmd,"autocorrupt")){plat_store_save(OBJ_AUTO,"junk junk junk junk",19);return 1;}return 0;
}
#endif
static void enter(unsigned pitch,unsigned vel){
 unsigned i;ScNote note={(uint16_t)ui.cursor,(uint16_t)ui.length,pitch,ui.instrument,vel,ui.layer};ui.last=pitch;
 if(engine.playing){app_say("STOP TO EDIT","");return;}
 if(ui.erase){ui.edit_used=1;for(i=0;i<song.count;i++)if(song.notes[i].tick==ui.cursor && song.notes[i].pitch==pitch && song.notes[i].instrument==ui.instrument && song.notes[i].layer==ui.layer){checkpoint();memmove(song.notes+i,song.notes+i+1,(song.count-i-1)*sizeof(ScNote));song.count--;return;}return;}
 if(song.count>=SC_EVENTS){app_say("SONG FULL","");return;}
 checkpoint();if(sc_add(&song,note))sc_note(&engine,&song,note);
}
/* BARS (page 6): 0-based bars bf..bt, target bg, transpose bx; bars() = bars in the song (last may be partial) */
#define BARS ((song.end+song.beats*96u-1)/(song.beats*96u))
static void bars_fix(void){if(ui.bf>=BARS)ui.bf=BARS-1;if(ui.bt>=BARS)ui.bt=BARS-1;if(ui.bt<ui.bf)ui.bt=ui.bf;}
static void bars_knob(int k,int d){
 unsigned maxg=65535/(song.beats*96u)-1;
 if(k==0)ui.bf=clamp((int)ui.bf+d,0,BARS-1);else if(k==1)ui.bt=clamp((int)ui.bt+d,0,BARS-1);else if(k==2)ui.bg=clamp((int)ui.bg+d,0,maxg);else ui.bx=clamp(ui.bx+d,-12,12);
 bars_fix();
}
/* op 0 COPY (REC), 1 MOVE (FX), 2 DELETE (SEQ), 3 INSERT (ARP), 4 TRANSPOSE (OCT+); copy/move/transpose follow the SEL filter, delete/insert take every sound */
static void bars_op(int op){
 static const char *const ok[5]={"COPIED","MOVED","BARS DELETED","BARS INSERTED","TRANSPOSED"},*const bad[5]={"BAD RANGE","SONG FULL","OVER 65535 TICKS","OUT OF C2 TO C6","NOTHING TO DO"};
 unsigned bar=song.beats*96u,a=ui.bf*bar,len=(ui.bt-ui.bf+1)*bar,t=ui.bg*bar,dirty=ui.dirty;int inst=ui.filter?(int)ui.instrument:-1,r;
 if(engine.playing){app_say("STOP FIRST","");return;}
 checkpoint();
 r=op==0?sc_copy(&song,a,len,t,inst):op==1?sc_move(&song,a,len,t,inst):op==2?sc_delete_bars(&song,a,len):op==3?sc_insert_bars(&song,a,len):sc_transpose(&song,a,len,ui.bx,inst);
 if(r<0){undo();hist_count=hist_pos;ui.dirty=dirty;app_say(bad[-r-1],"");}else{bars_fix();app_say(ok[op],"");}
}
/* SOUND (page 3), MIDI (4) and GROOVE (5: K1 swing, K2 human) knobs: k = K1..K4 */
static void shape(int k,int d){
 unsigned u=ui.instrument;int v,lo=0,hi=127;
 if(ui.page==6){bars_knob(k,d);return;}
 if(ui.page==5){if(k>1)return;v=k?song.human:song.swing;hi=k?100:75;}
 else if(ui.page==3){v=k==0?song.tune[u]:k==1?song.tone[u]:k==2?song.decay[u]:song.pan[u];if(k==0)lo=-12,hi=12;if(k==3)lo=-64,hi=63;}
 else{if(k>2)return;v=k==0?song.midi_ch[u]:k==1?song.midi_mode:song.midi_clock;hi=k?2:16;}
 d=clamp(v+d,lo,hi);if(d==v)return;checkpoint();
 if(ui.page==5){if(k)song.human=d;else song.swing=d;}
 else if(ui.page==3){if(k==0)song.tune[u]=d;else if(k==1)song.tone[u]=d;else if(k==2)song.decay[u]=d;else song.pan[u]=d;}
 else if(k==0)song.midi_ch[u]=d;else if(k==1)song.midi_mode=d;else song.midi_clock=d;
}
/* EDIT held + K2 (what 0: velocity) / K3 (1: length) on the staff: the selected sound's notes at the cursor, as erase picks them */
static int note_edit(int what,int d){
 unsigned i,n=0;
 if(!ui.erase || ui.page || engine.playing)return 0;
 ui.edit_used=1;
 for(i=0;i<song.count;i++)if(song.notes[i].tick==ui.cursor && song.notes[i].instrument==ui.instrument && song.notes[i].layer==ui.layer){
  ScNote *p=song.notes+i;int v=what?clamp(p->duration+d*(int)LSTEP,(int)LSTEP,1536)/(int)LSTEP*(int)LSTEP:clamp(p->velocity+d,1,127);
  if(!n++ && !ui.gest){checkpoint();ui.gest=1;}
  if(what)p->duration=v;else p->velocity=v;
  snprintf(ui.message,sizeof ui.message,what?"NOTE LEN %d T":"NOTE VEL %d",v);ui.until=plat_ms()+1600;
 }
 if(!n)app_say("NO NOTE HERE","");
 return 1;
}
static void input(void){
 uint32_t keys=plat_keys(),btn=plat_buttons(),pressed=btn&~ui.buttons,f=plat_lock();unsigned k;int d;
 ui.erase=(btn>>B_EDIT)&1u;
 if(pressed&(1u<<B_EDIT))ui.edit_used=ui.gest=0;
 {uint32_t pkt;int port;while(plat_midi_in(&pkt,&port)){unsigned rt=pkt>>8&0xFFu,st=rt&0xF0u,p=pkt>>16&0x7Fu,v=pkt>>24&0x7Fu;if(song.midi_clock==2){if(rt==0xF8u)sc_ext_clock(&engine,&song,plat_ms()*1000u);else if(rt==0xFAu||rt==0xFBu)sc_ext_start(&engine,&song,rt&1);else if(rt==0xFCu)sc_stop(&engine);} /* clock in: read here (main loop, under the lock) and stamped on read, in ms (the 24-interval mean averages the quantizing) */
  if(st==0x90u && v && p>=36 && p<=84)enter(p,v);}}
 for(k=0;k<NKEYS;k++)if((keys&~ui.keys)&(1u<<k)){int pitch=53+(int)k+12*ui.octave;if(pitch>=36 && pitch<=84)enter(pitch,ui.velocity);else app_say("C2 TO C6","");}
 if(pressed&(1u<<B_PLAY)){if(engine.playing)sc_stop(&engine);else if(ui.erase){ui.edit_used=1;sc_start_at(&engine,&song,ui.cursor/(song.beats*96u)*(song.beats*96u));}else sc_start(&engine);} /* EDIT+PLAY: from the cursor's bar */
 if(ui.page==6){ /* BARS: REC FX SEQ ARP OCT+ are the ops (OCT- unused) */
  static const uint8_t ob[5]={B_REC,B_FX,B_SEQ,B_ARP,B_OCTUP};unsigned i;
  for(i=0;i<5;i++)if(pressed>>ob[i]&1)bars_op(i);
  pressed&=~((1u<<B_REC)|(1u<<B_FX)|(1u<<B_SEQ)|(1u<<B_ARP)|(1u<<B_OCTUP)|(1u<<B_OCTDN));
 }
 if(pressed&(1u<<B_OCTDN))ui.octave=clamp(ui.octave-1,-2,2);
 if(pressed&(1u<<B_OCTUP))ui.octave=clamp(ui.octave+1,-2,2);
 if(pressed&(1u<<B_GLO)){ui.page=ui.page==2?0:2;ui.confirm=0;}
 if(pressed&(1u<<B_HOME))ui.hold=plat_ms();
 if(btn&(1u<<B_HOME))ui.help=plat_ms()-ui.hold>=600; /* held 600 ms: help overlay; a tap acts on release */
 else if(ui.buttons&(1u<<B_HOME)){if(!ui.help){ui.page=0;ui.cursor=0;ui.confirm=0;}ui.help=0;}
 if(ui.page==1){ /* MIX: FX mutes, SEL solos the selected layer (live, any time, not saved) */
  unsigned m=1u<<ui.layer;
  if(pressed&(1u<<B_FX)){engine.mute^=m;snprintf(ui.message,sizeof ui.message,"LAYER %u %s",ui.layer+1,engine.mute&m?"MUTED":"UNMUTED");ui.until=plat_ms()+1600;}
  if(pressed&(1u<<B_SEL)){engine.solo^=m;snprintf(ui.message,sizeof ui.message,"LAYER %u %s",ui.layer+1,engine.solo&m?"SOLO":"UNSOLO");ui.until=plat_ms()+1600;}
 }else if(pressed&(1u<<B_SEL))ui.filter^=1;
 if((ui.buttons&(1u<<B_EDIT)) && !(btn&(1u<<B_EDIT)) && !ui.edit_used)ui.page="\1\3\0\4\5\6\0"[ui.page];
 if(!engine.playing){
  if(pressed&(1u<<B_ENV))undo();
  if(pressed&(1u<<B_LFO))redo();
  if(pressed&(1u<<B_SEQ)){ui.grid=ui.grid==96?48:ui.grid==48?24:ui.grid==24?32:ui.grid==32?16:96;ui.cursor=ui.cursor/ui.grid*ui.grid;}
  if(pressed&(1u<<B_ARP)){checkpoint();song.loop^=1;}
  if(pressed&(1u<<B_FX) && ui.page!=1){checkpoint();song.volume[ui.instrument]=song.volume[ui.instrument]?0:127;}
  if(pressed&(1u<<B_REC)){
   if(ui.page==2){
    if(ui.confirm!=ui.slot+1){ui.confirm=ui.slot+1;app_say("REC AGAIN TO LOAD","");}
    else{int n;plat_unlock(f);n=plat_store_load(ui.slot,wire,sizeof wire);f=plat_lock();checkpoint();if(n>0 && sc_decode(&song,wire,n)){ui.dirty=0;ui.cursor=0;app_say("LOADED","");}else app_say("EMPTY OR BAD SLOT","");ui.confirm=0;}
   }else{unsigned bar=song.beats*96,start=ui.cursor/bar*bar;checkpoint();if(!sc_duplicate(&song,start,bar)){undo();app_say("COPY FAILED","");}else app_say("BAR COPIED","");}
  }
  if(pressed&(1u<<B_SAVE)){if(ui.held){app_say("BACKUP BUSY","");}else{unsigned n=sc_encode(&song,wire);int rc;sc_stop(&engine);plat_unlock(f);rc=n?plat_store_save(ui.slot,wire,n):-1;if(!rc){ui.dirty=0;asave();}f=plat_lock();if(!rc){app_say("SAVED","");}else app_say("SAVE FAILED","");}}
 }else if(pressed&((1u<<B_SAVE)|(1u<<B_REC)|(1u<<B_ENV)|(1u<<B_LFO)))app_say("STOP FIRST","");
 ui.erase=(btn>>B_EDIT)&1u;
 if((d=plat_enc(EN_SELECT)))ui.cursor=clamp((int)ui.cursor+d*(int)ui.grid,0,(song.end-1)/ui.grid*ui.grid);
 if((d=plat_enc(EN_PRESET)))ui.instrument=clamp((int)ui.instrument+d,0,14);
 if((d=plat_enc(EN_ALGO)))ui.layer=clamp((int)ui.layer+d,0,7);
 if(ui.page>2){int i;for(i=0;i<4;i++)if((d=plat_enc(EN_K1+i)) && !engine.playing)shape(i,d);}else{
 if((d=plat_enc(EN_K1))){if(ui.page==2){ui.slot=clamp((int)ui.slot+d,0,7);ui.confirm=0;}else if(!engine.playing){if(ui.page==1){int b=clamp(song.beats+d,2,12);if(b!=song.beats){checkpoint();song.beats=b;}}else{checkpoint();song.tempo=clamp(song.tempo+d,30,300);}}}
 if((d=plat_enc(EN_K2))){if(note_edit(0,d)){}else if(ui.page==1 && !engine.playing){checkpoint();song.volume[ui.instrument]=clamp(song.volume[ui.instrument]+d,0,127);}else ui.velocity=clamp((int)ui.velocity+d,1,127);}
 if((d=plat_enc(EN_K3))){if(note_edit(1,d)){}else if(ui.page==1 && !engine.playing){checkpoint();song.layers[ui.layer]=clamp(song.layers[ui.layer]+d,0,127);}else ui.length=clamp((int)ui.length+d*(int)LSTEP,(int)LSTEP,1536)/LSTEP*LSTEP;}
 if((d=plat_enc(EN_K4)) && !engine.playing){unsigned minend=song.count?song.notes[song.count-1].tick+1:1;checkpoint();song.end=clamp(song.end+d*96,minend>song.loop_start?minend:(unsigned)song.loop_start+1,65535);if(ui.cursor>=song.end)ui.cursor=(song.end-1)/ui.grid*ui.grid;}}
 if(ui.cursor>=song.end)ui.cursor=(song.end-1)/ui.grid*ui.grid;
 if(keys!=ui.keys||btn!=ui.buttons)au.act=plat_ms();
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
static void pan_text(int p,char *t,unsigned n){if(p<0)snprintf(t,n,"L%d",-p);else if(p>0)snprintf(t,n,"R%d",p);else snprintf(t,n,"C");}
static void tiles_shape(void){
 char t[16];unsigned u=ui.instrument;
 if(ui.page==3){
  snprintf(t,sizeof t,"%+d",song.tune[u]);tile(4,2,55,"K1 TUNE",t);
  snprintf(t,sizeof t,"%u",song.tone[u]);tile(63,2,55,"K2 TONE",t);
  snprintf(t,sizeof t,"%u",song.decay[u]);tile(122,2,55,"K3 DECAY",t);
  pan_text(song.pan[u],t,sizeof t);tile(181,2,55,"K4 PAN",t);
 }else if(ui.page==5){
  snprintf(t,sizeof t,"%u%%",song.swing);tile(4,2,55,"K1 SWING",t);
  snprintf(t,sizeof t,"%u%%",song.human);tile(63,2,55,"K2 HUMAN",t);
  tile(122,2,55,"K3","-");tile(181,2,55,"K4","-");
 }else if(ui.page==6){
  snprintf(t,sizeof t,"%u",ui.bf+1);tile(4,2,55,"K1 FROM",t);
  snprintf(t,sizeof t,"%u",ui.bt+1);tile(63,2,55,"K2 TO",t);
  snprintf(t,sizeof t,"%u",ui.bg+1);tile(122,2,55,"K3 TARGET",t);
  snprintf(t,sizeof t,"%+d",ui.bx);tile(181,2,55,"K4 SEMI",t);
 }else{
  if(song.midi_ch[u])snprintf(t,sizeof t,"%u",song.midi_ch[u]);else snprintf(t,sizeof t,"OFF");tile(4,2,55,"K1 CH",t);
  tile(63,2,55,"K2 MODE",song.midi_mode==2?"ONLY":song.midi_mode?"ON":"OFF");
  tile(122,2,55,"K3 CLOCK",CKN);tile(181,2,55,"K4","-");
 }
}
static const char *const help[7][12]={
 {"KEYS  PLACE / AUDITION","SELECT  MOVE CURSOR","PRESET SOUND   ALGO LAYER","K1 TEMPO  K2 VEL  K3 LEN  K4 END","PLAY  START / STOP","EDIT+PLAY  PLAY FROM CURSOR BAR","EDIT+KEY  ERASE","EDIT+K2/K3  VEL / LEN AT CURSOR","SEQ GRID   ARP LOOP   REC COPY BAR","FX MUTE SOUND   SEL SOLO VIEW","ENV UNDO   LFO REDO   OCT-/+","EDIT NEXT PAGE   GLO SONG BOOK"},
 {"K1 BEATS PER BAR","K2 SOUND LEVEL","K3 LAYER LEVEL","K4 SONG END","ALGO  PICK LAYER","PRESET  PICK SOUND","FX  MUTE LAYER","SEL  SOLO LAYER","MUTE / SOLO ARE LIVE, NOT SAVED","EDIT NEXT PAGE   HOME STAFF"},
 {"K1 SAVE SLOT","SAVE  WRITE SLOT","REC  ARM LOAD, REC AGAIN LOADS","GLO / HOME  BACK TO STAFF"},
 {"K1 TUNE","K2 TONE","K3 DECAY","K4 PAN","PRESET  PICK SOUND","EDIT NEXT PAGE   HOME STAFF"},
 {"K1 CHANNEL OFF / 1-16","K2 MODE OFF / ON / MIDI ONLY","K3 CLOCK OFF / OUT / IN","PRESET  PICK SOUND","EDIT NEXT PAGE   HOME STAFF"},
 {"K1 SWING 0-75","LATE OFF-BEAT 8THS AND 16THS","K2 HUMAN 0-100","RANDOM TIMING AND VELOCITY","SET WHILE STOPPED, SAVED WITH SONG","EDIT NEXT PAGE   HOME STAFF"},
 {"K1 FROM  K2 TO  (BARS)","K3 TARGET BAR  K4 TRANSPOSE","REC COPY TO TARGET (KEEPS NOTES)","FX MOVE TO TARGET","SEQ DELETE BARS  ARP INSERT BARS","OCT+ TRANSPOSE BY K4","SEL: ONLY SELECTED SOUND","(COPY MOVE TRANSPOSE)","ENV UNDO   STOPPED ONLY","EDIT NEXT PAGE   HOME STAFF"}};
static const char *const hname[7]={"STAFF","MIX","SONG BOOK","SOUND","MIDI","GROOVE","BARS"};
static void draw_help(void){ /* full screen in three 80-row bands; a tap on HOME does not show it */
 unsigned b,i;
 for(b=0;b<3;b++){
  int o=-80*(int)b;cv_begin(240,80,INK);
  cv_rect(0,o,4,32,ACCENT);cv_text(10,o+4,&FONT_B,hname[ui.page],C_WHITE);cv_text(150,o+8,&FONT_T,"HELP  RELEASE HOME",RGB(162,182,188));
  for(i=0;i<12 && help[ui.page][i];i++)cv_text(10,o+30+(int)i*17,&FONT_T,help[ui.page][i],C_WHITE);
  cv_blit(0,80*b);
 }
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
 snprintf(t,sizeof t,"%03u",song.midi_clock==2 && engine.xinc?(unsigned)(engine.xinc/96+.5f):song.tempo);cv_text(140,3,&FONT_B,t,C_WHITE); /* IN: measured tempo */
 cv_text(168,8,&FONT_T,song.midi_clock==2?"EXT":"BPM",RGB(162,182,188));
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
 snprintf(t,sizeof t,"LAYER %u%s",ui.layer+1,engine.solo>>ui.layer&1?" SOLO":engine.mute>>ui.layer&1?" M":engine.solo?" OFF":"");cv_text(164,49,&FONT_T,t,engine.mute>>ui.layer&1 || (engine.solo && !(engine.solo>>ui.layer&1))?ACCENT:TEAL);
 snprintf(t,sizeof t,"%s / %s",character_names[selected],sc_names[selected]);
 cv_text(8,56,&FONT_XS,t,INK);
 cv_blit(0,0);
 if(ui.page==3){
  static const char *const nm[4]={"TUNE","TONE","DECAY","PAN"};
  cv_begin(240,112,PAPER);
  cv_text(10,4,&FONT_B,"SOUND SHAPE",INK);
  for(i=0;i<4;i++){
   int y=26+(int)i*21,v=i==0?song.tune[selected]:i==1?song.tone[selected]:i==2?song.decay[selected]:song.pan[selected];
   int lo=i==0?-12:i==3?-64:0,hi=i==0?12:i==3?63:127;
   snprintf(t,sizeof t,"K%u  %s",i+1,nm[i]);cv_text(10,y+2,&FONT_XS,t,TEAL);
   cv_rect(86,y+3,104,9,RGB(225,218,197));cv_rect(86,y+3,(v-lo)*104/(hi-lo),9,i==3?TEAL:ACCENT);
   if(lo<0)cv_rect(137,y,2,15,INK);
   if(i==0)snprintf(t,sizeof t,"%+d",v);else if(i==3)pan_text(v,t,sizeof t);else snprintf(t,sizeof t,"%d",v);
   cv_text(197,y,&FONT_B,t,INK);
  }
  cv_blit(0,72);
 }else if(ui.page==6){
  unsigned nb=BARS,bar=song.beats*96u,n;
  cv_begin(240,112,PAPER);
  cv_text(10,4,&FONT_B,"BARS",INK);
  snprintf(t,sizeof t,"%u-%u > %u%s",ui.bf+1,ui.bt+1,ui.bg+1,ui.filter?"  SOUND":"");cv_text(80,7,&FONT_XS,t,TEAL);
  for(i=0;i<nb;i++){int x=10+(int)(i*220/nb),x2=10+(int)((i+1)*220/nb);cv_rect(x,28,x2-x-1,16,i>=ui.bf && i<=ui.bt?ACCENT:RGB(233,225,202));} /* overview: one cell per bar, range filled, target underlined */
  for(n=0;n<song.count;n++)cv_rect(10+(int)(song.notes[n].tick/bar*220/nb)+2,34,3,4,INK);
  if(ui.bg<nb)cv_rect(10+(int)(ui.bg*220/nb),46,220/(int)nb-1,3,TEAL);
  cv_text(10,56,&FONT_T,"REC COPY  FX MOVE  SEQ DEL",INK);cv_text(10,72,&FONT_T,"ARP INSERT  OCT+ TRANSPOSE",INK);
  snprintf(t,sizeof t,"%u BARS  %u NOTES",nb,song.count);cv_text(10,92,&FONT_T,t,MUTED);
  cv_blit(0,72);
 }else if(ui.page==5){
  cv_begin(240,112,PAPER);
  cv_text(10,4,&FONT_B,"GROOVE",INK);
  for(i=0;i<2;i++){
   int y=30+(int)i*34,v=i?song.human:song.swing,hi=i?100:75;
   snprintf(t,sizeof t,"K%u  %s",i+1,i?"HUMAN":"SWING");cv_text(10,y+2,&FONT_XS,t,TEAL);
   cv_rect(86,y+3,104,9,RGB(225,218,197));cv_rect(86,y+3,v*104/hi,9,i?TEAL:ACCENT);
   snprintf(t,sizeof t,"%d",v);cv_text(197,y,&FONT_B,t,INK);
  }
  cv_text(10,98,&FONT_T,"OFF-BEAT LATE / RANDOM TIME + VEL",MUTED);
  cv_blit(0,72);
 }else if(ui.page==4){
  cv_begin(240,112,PAPER);
  cv_text(10,4,&FONT_B,"MIDI OUT",INK);
  for(i=0;i<15;i++){
   int x=8+(int)(i%5)*46,y=26+(int)(i/5)*22;
   cv_rect(x,y,44,20,i==selected?TEAL:RGB(233,225,202));
   stamp(x+2,y+1,i,1);
   if(song.midi_ch[i])snprintf(t,sizeof t,"%u",song.midi_ch[i]);else snprintf(t,sizeof t,"--");
   cv_text(x+24,y+5,&FONT_T,t,i==selected?C_WHITE:INK);
  }
  snprintf(t,sizeof t,"MODE %s   CLOCK %s",song.midi_mode==2?"MIDI ONLY":song.midi_mode?"ON":"OFF",CKN);
  cv_text(10,96,&FONT_T,t,TEAL);
  cv_blit(0,72);
 }else if(ui.page==2){
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
  snprintf(t,sizeof t,"K1  BEATS %u/4",song.beats);cv_text(140,7,&FONT_XS,t,TEAL);
  cv_text(10,29,&FONT_XS,"K2  SOUND LEVEL",TEAL);
  cv_rect(10,48,180,9,RGB(225,218,197));cv_rect(10,48,song.volume[selected]*180/127,9,ACCENT);
  snprintf(t,sizeof t,"%u",song.volume[selected]);cv_text(197,42,&FONT_B,t,INK);
  cv_text(10,67,&FONT_XS,"K3  LAYER LEVEL",TEAL);
  cv_rect(10,86,180,9,RGB(225,218,197));cv_rect(10,86,song.layers[ui.layer]*180/127,9,TEAL);
  snprintf(t,sizeof t,"%u",song.layers[ui.layer]);cv_text(197,80,&FONT_B,t,INK);
  for(i=0;i<8;i++){ /* layers: ALGO picks; M muted, S solo (live) */
   int x=10+(int)i*28,mu=engine.mute>>i&1,so=engine.solo>>i&1;
   cv_rect(x,98,26,12,i==ui.layer?TEAL:RGB(233,225,202));
   snprintf(t,sizeof t,"%u%s",i+1,so?"S":mu?"M":"");cv_text(x+3,98,&FONT_T,t,i==ui.layer?C_WHITE:so?ACCENT:mu?MUTED:INK);
  }
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
  if(engine.playing){ /* hop: one arc per grid step, tall when a note starts at the next step (landing), else a small bounce */
   unsigned p=active%step,h=2;for(i=0;i<song.count;i++)if(song.notes[i].tick/step==active/step+1){h=8;break;}
   stamp(cursor_x,11-(int)(h*4*p*(step-p)/(step*step)),selected,1);}
  else cv_rect(cursor_x+4,19,5,4,ACCENT);
  if(!song.count)cv_text(40,85,&FONT_XS,"PLAY A KEY TO PLACE A NOTE",TEAL);
  cv_blit(0,72);
 }
 /* Four encoder cards, then one concise contextual hint. */
 cv_begin(240,56,INK);
 if(ui.page>2)tiles_shape();else{
 snprintf(t,sizeof t,ui.page==1?"%u/4":ui.page==2?"%02u":"%u",ui.page==1?song.beats:ui.page==2?ui.slot+1:song.tempo);tile(4,2,55,ui.page==2?"K1 SLOT":ui.page==1?"K1 BEATS":"K1 TEMPO",t);
 snprintf(t,sizeof t,"%u",ui.page==1?song.volume[selected]:ui.velocity);tile(63,2,55,ui.page==1?"K2 SOUND":"K2 VEL",t);
 snprintf(t,sizeof t,ui.page==1?"%u":"1/%u",ui.page==1?song.layers[ui.layer]:384/ui.length);if(ui.page!=1 && (ui.length>384 || 384%ui.length!=0))snprintf(t,sizeof t,"%u T",ui.length);if(ui.page!=1 && gname(ui.length))snprintf(t,sizeof t,"%s",gname(ui.length));tile(122,2,55,ui.page==1?"K3 LAYER":"K3 LEN",t);
 snprintf(t,sizeof t,"%u BAR",(song.end+song.beats*96-1)/(song.beats*96));tile(181,2,55,"K4 END",t);}
 if(plat_ms()<ui.until)cv_text(5,44,&FONT_T,ui.message,RGB(250,212,104));
 else{snprintf(t,sizeof t,ui.page==3?"EDIT NEXT / PRESET SOUND":ui.page==4?"EDIT NEXT / K1 CH / HOME STAFF":ui.page==6?"REC CPY FX MOV SEQ DEL ARP INS OCT+":ui.page==5?"EDIT NEXT / K1 SWING / K2 HUMAN":ui.page==2?"SAVE WRITE / REC LOAD / HOME EXIT":ui.page==1?"FX MUTE / SEL SOLO / ALGO LAYER":"%s GRID / %u NOTES / %s",gname(step),song.count,ui.dirty?"UNSAVED":"SAVED");cv_text(5,44,&FONT_T,t,RGB(162,182,188));}
 cv_blit(0,184);
 if(ui.help)draw_help();
}
/* APP_REDRAW: draw() repaints all three bands every frame, so a mirror that lost a part is fixed by the next frame: nothing to do */
void app_redraw(void){}
void app_ui_init(void){draw();}
void app_frame(void){
 input();autosave();draw();
 if(ann_on() && ann_where()){
  char t[160];snprintf(t,sizeof t,"Stamp Composer. %s. %s. %u notes. Tempo %u. Layer %u. %s.",
   ui.page==6?"Bars":ui.page==5?"Groove":ui.page==4?"MIDI":ui.page==3?"Sound":ui.page==2?"Song book":ui.page==1?"Mixer":"Staff",character_names[ui.instrument],song.count,song.tempo,ui.layer+1,engine.playing?"Playing":"Stopped");
  ann_say(t,0,ANN_NOW);
 }
 ann_tick(plat_ms());
}
int app_expect(const char *s,int *v){
 if(!strcmp(s,"page"))*v=ui.page;else if(!strcmp(s,"autosaves"))*v=au.n;else if(!strcmp(s,"slot"))*v=ui.slot;else if(!strcmp(s,"notes"))*v=song.count;else if(!strcmp(s,"playing"))*v=engine.playing;else if(!strcmp(s,"dirty"))*v=ui.dirty;else if(!strcmp(s,"instrument"))*v=ui.instrument;else if(!strcmp(s,"tick"))*v=engine.tick;else if(!strcmp(s,"triggers"))*v=engine.triggers;else if(!strcmp(s,"tempo"))*v=song.tempo;else if(!strcmp(s,"cursor"))*v=ui.cursor;else if(!strcmp(s,"grid"))*v=ui.grid;else if(!strcmp(s,"beats"))*v=song.beats;else if(!strcmp(s,"length"))*v=ui.length;
 else if(!strcmp(s,"tune"))*v=song.tune[ui.instrument];else if(!strcmp(s,"tone"))*v=song.tone[ui.instrument];else if(!strcmp(s,"decay"))*v=song.decay[ui.instrument];else if(!strcmp(s,"pan"))*v=song.pan[ui.instrument];
 else if(!strcmp(s,"midi_ch"))*v=song.midi_ch[ui.instrument];else if(!strcmp(s,"midi_mode"))*v=song.midi_mode;else if(!strcmp(s,"midi_clock"))*v=song.midi_clock;else if(!strcmp(s,"tick8"))*v=engine.tick/8;else if(!strcmp(s,"ext_bpm"))*v=(int)(engine.xinc/96+.5f);
 else if(!strcmp(s,"note_vel")||!strcmp(s,"note_len")){unsigned i;for(i=0;i<song.count;i++)if(song.notes[i].tick==ui.cursor && song.notes[i].instrument==ui.instrument && song.notes[i].layer==ui.layer)break;if(i==song.count)return 0;*v=s[5]=='v'?song.notes[i].velocity:song.notes[i].duration;}
 else if(!strcmp(s,"swing"))*v=song.swing;else if(!strcmp(s,"human"))*v=song.human;else if(!strcmp(s,"mute"))*v=engine.mute;else if(!strcmp(s,"solo"))*v=engine.solo;else if(!strcmp(s,"layer"))*v=ui.layer;else if(!strcmp(s,"help"))*v=ui.help;
 else if(!strcmp(s,"end"))*v=song.end;else if(!strcmp(s,"loop_start"))*v=song.loop_start;else if(!strcmp(s,"valid"))*v=sc_valid(&song);else if(!strcmp(s,"bf"))*v=ui.bf+1;else if(!strcmp(s,"bt"))*v=ui.bt+1;else if(!strcmp(s,"bg"))*v=ui.bg+1;else if(!strcmp(s,"bx"))*v=ui.bx;
 else if(!strcmp(s,"first_tick")||!strcmp(s,"first_pitch")||!strcmp(s,"last_tick")||!strcmp(s,"last_pitch")){if(!song.count)return 0;*v=s[0]=='f'?(s[6]=='t'?song.notes[0].tick:song.notes[0].pitch):(s[5]=='t'?song.notes[song.count-1].tick:song.notes[song.count-1].pitch);}
 else return 0;
 return 1;
}
/* Backup over USB (plat.h). Flush is a no-op by choice: saving stays explicit (SAVE), so a backup holds what is saved and never overwrites a slot with unsaved edits. */
int app_backup_playing(void){return engine.playing;}
void app_backup_flush(void){}
void app_backup_hold(int on){ui.held=on;}
void app_backup_restored(void){
 int n;sc_default(&song);n=plat_store_load(ui.slot,wire,sizeof wire);if(n>0)sc_decode(&song,wire,n);
 hist_count=hist_pos=0;ui.dirty=ui.confirm=0;ui.cursor=0;au.last=plat_ms()-20000u; /* the old working copy must not resurrect: rewritten after 3 s idle */
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

