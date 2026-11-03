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
static struct { unsigned cursor,instrument,layer,grid,velocity,length,slot,dirty,page,filter,erase,confirm,edit_used,gest,held,help,hp,ho,hs,nsp,nss,nsn,ps,used,pp,bf,bt,bg,free,lcp,bp,sp,lf,dm; int octave,last,bx,o0; uint32_t keys,buttons,until,hold,ot; uint8_t kp[NKEYS],ki[NKEYS]; char message[32]; } ui;
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
 if(hist_pos<16){history[hist_pos++]=song;}else{memmove(history,history+1,15*sizeof *history);history[15]=song;if(ui.nsp)ui.nsp--;}
 hist_count=hist_pos;ui.dirty=1;if(ui.nsp>=hist_pos)ui.nsp=0;
}
/* a load or new song rebinds the working slot: nsp = its history state, nss/nsn = the slot before/after, so undo/redo carry the slot with the song; ps = the working song's slot (ui.slot also moves while browsing) */
static void bind(void){ui.nsp=hist_pos;ui.nss=ui.ps;ui.nsn=ui.slot;ui.ps=ui.slot;}
static void undo(void){if(hist_pos){if(hist_pos==hist_count)history[hist_pos]=song;if(hist_pos==ui.nsp)ui.slot=ui.ps=ui.nss;song=history[--hist_pos];ui.dirty=1;}}
static void redo(void){if(hist_pos<hist_count){song=history[++hist_pos];if(hist_pos==ui.nsp)ui.slot=ui.ps=ui.nsn;ui.dirty=1;}}
void app_say(const char *a,const char *b){snprintf(ui.message,sizeof ui.message,"%s %s",a?a:"",b?b:"");ui.until=plat_ms()+1600;ann_say(a,b,ANN_NOW);}
static int stopped(void){if(engine.playing)app_say("STOP FIRST","");return !engine.playing;} /* the one refusal for edits during playback */
#define WRAP(v,d,n) (unsigned)((((int)(v)+(d))%(n)+(n))%(n))
#define CKN (song.midi_clock==2?"IN":song.midi_clock?"OUT":"OFF")
static void midi_out(uint32_t p){plat_midi_out(p);}
void app_panic(void){sc_stop(&engine);memset(engine.voices,0,sizeof engine.voices);}
void app_render(int32_t *lr,uint32_t n,uint32_t master){sc_render(&engine,&song,lr,n,master);}
void app_init(void){
 int n;memset(&ui,0,sizeof ui);ui.grid=96;ui.length=96;ui.velocity=100;ui.last=60;ui.bx=12;ui.sp=1;hist_count=hist_pos=0;
 sc_default(&song);sc_engine_init(&engine,8);engine.midi=midi_out;
 n=plat_store_load(OBJ_AUTO,arec,sizeof arec); /* restore the working song; any damage falls back to slot 1 */
 if(n>8 && arec[0]==(AMAGIC&255) && arec[1]==AMAGIC>>8 && arec[2]==1 && arec[3]<8 && sc_decode(&song,arec+8,n-8)){ui.slot=arec[3];ui.dirty=arec[4]&1;snprintf(ui.message,sizeof ui.message,"RESTORED");ui.until=plat_ms()+1600;}
 else{sc_default(&song);n=plat_store_load(OBJ_SONG0,wire,sizeof wire);if(n>0)sc_decode(&song,wire,n);}
 au.sig=au.seen=asig(apack());au.act=au.chk=plat_ms();au.last=au.act-20000u;
}
static int load_demo(void){ /* Twinkle Twinkle (our arrangement): STAR melody + HEART bass, 12 bars; bar types 0..5 in the order of seq, 0 ends a bar's melody (3rd note = half) */
 static const uint8_t mel[6][4]={{72,72,79,79},{81,81,79,0},{77,77,76,76},{74,74,72,0},{79,79,77,77},{76,76,74,0}},bas[6][2]={{48,48},{53,48},{53,48},{43,48},{48,53},{48,43}},seq[12]={0,1,2,3,4,5,4,5,0,1,2,3};
 unsigned b,i,k;if(engine.playing)return 0;checkpoint();sc_default(&song);song.loop=1;song.tempo=100;song.end=4608;
 for(b=0;b<12;b++){k=seq[b];
  for(i=0;i<4 && mel[k][i];i++)sc_add(&song,(ScNote){b*384+i*96,(k&1) && i==2?180:84,mel[k][i],3,100,0});
  for(i=0;i<2;i++)sc_add(&song,(ScNote){b*384+i*192,180,bas[k][i],14,90,0});
 }
 ui.cursor=0;ui.page=0;ui.instrument=3;ui.last=79;ui.confirm=0;app_say("TWINKLE TWINKLE","DEMO LOADED");return 1;
}
#ifdef PLAT_HOST
int app_host_cmd(const char *cmd,const char *a,const char *b,const char *c,const char *d){
 (void)a;(void)b;(void)c;(void)d;
 if(!strcmp(cmd,"demo")){load_demo();return 1;}if(!strcmp(cmd,"autocorrupt")){plat_store_save(OBJ_AUTO,"junk junk junk junk",19);return 1;}return 0;
}
#endif
/* Live (playing): audition, then place/erase at the playhead snapped to the nearest grid step. Past the end: loop start if looping, else the last step. One undo step per run (lcp). The scan index is kept by sc_insert/sc_remove; a note snapped ahead of the playhead plays again when reached. */
#define ATC(p) ((p)->tick==ui.cursor && (!ui.filter || (p)->instrument==ui.instrument)) /* a note at the cursor tick in the view (SEL filter) */
static unsigned at_cursor(void){unsigned i,n=0;for(i=0;i<song.count;i++)n+=ATC(song.notes+i);return n;}
static void note_label(unsigned pitch,unsigned flat,char *out){
 static const char *const names[]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"},*const fl[]={"C","Db","D","Eb","E","F","Gb","G","Ab","A","Bb","B"};
 pitch%=128;
 snprintf(out,8,"%s%d",(flat?fl:names)[pitch%12],(int)(pitch/12)-1);
}
/* EDIT+key target: at tick t the selected sound's first note of that pitch, else the most recently added one (any sound or layer); -1 if none */
static int victim(unsigned t,unsigned pitch){int i,j=-1;for(i=0;i<(int)song.count;i++){ScNote *p=song.notes+i;if(p->tick==t && p->pitch==pitch){j=i;if(p->instrument==ui.instrument)break;}}return j;}
static void erased(ScNote n){char t[32],l[8];note_label(n.pitch,n.layer>>7,l);snprintf(t,sizeof t,"ERASED %s %s",l,sc_names[n.instrument]);app_say(t,"");}
static void live(ScNote n){
 unsigned g=ui.grid,t=(engine.tick+g/2)/g*g;
 if(t>=song.end)t=song.loop?song.loop_start:(song.end-1)/g*g;
 n.tick=t;
 if(ui.erase){int i=victim(t,n.pitch);if(i<0)app_say("NOTHING TO ERASE","");else{n=song.notes[i];if(!ui.lcp++)checkpoint();sc_remove(&engine,&song,n);erased(n);}return;}
 sc_note(&engine,&song,n);
 if(sc_find(&song,n)>=0)return;
 if(song.count>=SC_EVENTS){app_say("SONG FULL","");return;}
 if(!ui.lcp++)checkpoint();
 sc_insert(&engine,&song,n);
}
static void enter(unsigned pitch,unsigned vel){
 int i;ScNote note={(uint16_t)ui.cursor,(uint16_t)ui.length,pitch,ui.instrument,vel,ui.layer|sc_spell(pitch,ui.sp)};if(ui.help)return; /* help open: consumed */
 ui.last=pitch;ui.lf=note.layer>>7;
 if(ui.erase)ui.edit_used=1;
 if(ui.free || ui.page){if(ui.free)note.duration=65535;sc_note(&engine,&song,note);return;} /* FREE: gated, ended by sc_release; off the staff keys only audition */
 if(engine.playing){live(note);return;}
 if(ui.erase){i=victim(ui.cursor,pitch);if(i<0)app_say("NOTHING TO ERASE","");else{note=song.notes[i];checkpoint();sc_remove(&engine,&song,note);erased(note);}return;}
 if((i=sc_find(&song,note))>=0){note=song.notes[i];checkpoint();sc_remove(&engine,&song,note);erased(note);return;} /* the same note again: toggle it off (no audition) */
 if(song.count>=SC_EVENTS){app_say("SONG FULL","");return;}
 checkpoint();if(sc_add(&song,note))sc_note(&engine,&song,note);
}
/* BARS (page 6): 0-based bars bf..bt, target bg, transpose bx; bars() = bars in the song (last may be partial) */
#define BARS ((song.end+song.beats*96u-1)/(song.beats*96u))
static void bars_fix(void){if(ui.bf>=BARS)ui.bf=BARS-1;if(ui.bt>=BARS)ui.bt=BARS-1;if(ui.bt<ui.bf)ui.bt=ui.bf;}
static void bars_knob(int k,int d){
 unsigned maxg=65535/(song.beats*96u)-1;
 if(k==0)ui.bf=clamp((int)ui.bf+d,0,BARS-1);else if(k==1)ui.bt=clamp((int)ui.bt+d,0,BARS-1);else if(k==2)ui.bg=clamp((int)ui.bg+d,0,maxg);else ui.bx=clamp(ui.bx+d,1,12);
 bars_fix();
}
/* op 0 COPY (REC), 1 MOVE (FX), 2 DELETE (SEQ), 3 INSERT (ARP), 4 TRANSPOSE UP (OCT+), 5 DOWN (OCT-) by K4; copy/move/transpose follow the SEL filter, delete/insert take every sound */
static void bars_op(int op){
 static const char *const ok[5]={"COPIED","MOVED","BARS DELETED","BARS INSERTED","TRANSPOSED"},*const bad[5]={"BAD RANGE","SONG FULL","OVER 65535 TICKS","OUT OF C2 TO C6","NOTHING TO DO"};
 unsigned bar=song.beats*96u,a=ui.bf*bar,len=(ui.bt-ui.bf+1)*bar,t=ui.bg*bar,dirty=ui.dirty;int inst=ui.filter?(int)ui.instrument:-1,r;
 if(!stopped())return;
 checkpoint();
 r=op==0?sc_copy(&song,a,len,t,inst):op==1?sc_move(&song,a,len,t,inst):op==2?sc_delete_bars(&song,a,len):op==3?sc_insert_bars(&song,a,len):sc_transpose(&song,a,len,op==4?ui.bx:-ui.bx,inst,ui.sp);
 if(r<0){undo();hist_count=hist_pos;ui.dirty=dirty;app_say(bad[-r-1],"");}else{bars_fix();app_say(ok[op>4?4:op],"");}
}
/* SOUND (page 3), MIDI (4) and GROOVE (5) knobs: k = K1..K4 */
static void shape(int k,int d){
 unsigned u=ui.instrument;int v,lo=0,hi=127;
 if(ui.page==6){bars_knob(k,d);return;}
 if(ui.page==5){v=k==0?song.swing:k==1?song.human:k==2?song.sgrid:song.hmode;hi=k==0?75:k==1?100:k==2?1:2;}
 else if(ui.page==3){v=k==0?song.tune[u]:k==1?song.tone[u]:k==2?song.decay[u]:song.pan[u];if(k==0)lo=-12,hi=12;if(k==3)lo=-64,hi=63;}
 else{v=k==0?song.midi_ch[u]:k==1?song.midi_mode:k==2?song.midi_clock:song.midi_in;hi=k==1||k==2?2:16;}
 d=clamp(v+d,lo,hi);if(d==v)return;checkpoint();
 if(ui.page==5){if(k==0)song.swing=d;else if(k==1)song.human=d;else if(k==2)song.sgrid=d;else song.hmode=d;}
 else if(ui.page==3){if(k==0)song.tune[u]=d;else if(k==1)song.tone[u]=d;else if(k==2)song.decay[u]=d;else song.pan[u]=d;}
 else if(k==0)song.midi_ch[u]=d;else if(k==1)song.midi_mode=d;else if(k==2)song.midi_clock=d;else song.midi_in=d;
}
/* EDIT held + K2 (what 0: velocity) / K3 (1: length) on the staff: the selected sound's notes at the cursor, as erase picks them */
static int note_edit(int what,int d){
 unsigned i,n=0;
 if(!ui.erase || ui.page || engine.playing)return 0;
 for(i=0;i<song.count;i++)if(song.notes[i].tick==ui.cursor && song.notes[i].instrument==ui.instrument && (song.notes[i].layer&127)==ui.layer){
  ScNote *p=song.notes+i;int v=what?clamp(p->duration+d*(int)LSTEP,(int)LSTEP,1536)/(int)LSTEP*(int)LSTEP:clamp(p->velocity+d,1,127);
  if(!n++ && !ui.gest){checkpoint();ui.gest=1;}
  if(what)p->duration=v;else p->velocity=v;
  snprintf(ui.message,sizeof ui.message,what?"NOTE LEN %d T":"NOTE VEL %d",v);ui.until=plat_ms()+1600;
 }
 if(!n)app_say("NO NOTE HERE","");
 return 1;
}
static const char *const SPN[3]={"SHARP","AUTO","FLAT"};
/* STAFF, EDIT+OCT- / OCT+ (after the 150 ms chord window): step the spelling mode toward flat (SHARP, AUTO, FLAT) / sharp; stopped: respell the selected sound's black-key notes at the cursor (one undo step; note_edit's match rule) */
static void spell(int d){
 unsigned i,n=0;
 ui.sp=clamp((int)ui.sp-d,0,2);app_say("SPELLING",SPN[ui.sp]);
 if(engine.playing)return;
 for(i=0;i<song.count;i++){ScNote *p=song.notes+i;
  if(p->tick==ui.cursor && p->instrument==ui.instrument && (p->layer&127)==ui.layer && sc_black(p->pitch) && (p->layer&SC_FLAT)!=sc_spell(p->pitch,ui.sp)){if(!n++)checkpoint();p->layer^=SC_FLAT;if(p->pitch==ui.last)ui.lf=p->layer>>7;}}
 if(n)snprintf(ui.message,sizeof ui.message,"%s %u NOTE%s",SPN[ui.sp],n,n>1?"S":"");
}
static void pick(int d){int p=clamp((ui.dm?8:(int)ui.slot)+d,0,8);ui.dm=p==8;if(!ui.dm)ui.slot=p;ui.confirm=0;} /* song book: 8 slots, then DEMO (slot stays the last real slot) */
/* STAFF, EDIT+FX (stopped): clear every note at the cursor tick (the selected sound's only with the SEL filter on), one undo step */
static void clear_cursor(void){
 unsigned i,j,n=at_cursor();char t[32];
 if(!n){app_say("NOTHING TO ERASE","");return;}
 checkpoint();for(i=j=0;i<song.count;i++)if(!ATC(song.notes+i))song.notes[j++]=song.notes[i];song.count=j;
 snprintf(t,sizeof t,"CLEARED %u NOTE%s",n,n>1?"S":"");app_say(t,"");
}
/* STAFF, EDIT+SELECT: d detents to the previous (<0) / next tick that has a note in the view, whatever the grid */
static void jump(int d){
 for(;d;d+=d>0?-1:1){unsigned i,b=0;int f=0;
  for(i=0;i<song.count;i++){unsigned t=song.notes[i].tick;if((!ui.filter || song.notes[i].instrument==ui.instrument) && (d>0?t>ui.cursor && (!f||t<b):t<ui.cursor && (!f||t>b))){b=t;f=1;}}
  if(!f){app_say("NO MORE NOTES","");return;}
  ui.cursor=b;}
}
/* Help: this page's rows, then the shared rows; 12 fit a screen, more scroll (SELECT / K1) as a second page */
static const char *const help[7][10]={
 {"KEYS  PLACE/REMOVE (LIVE: PLACE)","SELECT  CURSOR  EDIT+ NEXT NOTE","K1 TEMPO  K2 VEL  K3 LEN","K4 SONG END","SEQ GRID  ARP LOOP  REC COPY BAR","FX MUTE SOUND  SCL VIEW FILTER","OCT- / OCT+  KEYBOARD OCTAVE","EDIT+KEY ERASE  EDIT+FX CLEAR","EDIT+K2 / K3  NOTE VEL / LEN","EDIT+OCT- / OCT+  FLAT / SHARP"},
 {"K1 BEATS PER BAR  K4 SONG END","K2 SOUND LEVEL  K3 LAYER LEVEL","FX MUTE LAYER  SCL SOLO LAYER","MUTE / SOLO ARE LIVE, NOT SAVED","KNOBS: STOPPED ONLY","KEYS AUDITION  OCT- / OCT+ OCTAVE"},
 {"K1 / SELECT  SLOT 1-8, THEN DEMO","REC  ARM, REC AGAIN LOADS SLOT","EMPTY SLOT  REC REC NEW SONG","SAVE  WRITE SLOT (NOT DEMO)","K2 VEL  K3 LEN  (NEW NOTES)","FX MUTE SOUND  SCL VIEW FILTER","KEYS AUDITION  OCT- / OCT+ OCTAVE"},
 {"K1 TUNE  K2 TONE","K3 DECAY  K4 PAN","STOPPED ONLY, SAVED WITH SONG","FX MUTE SOUND  SCL VIEW FILTER","KEYS AUDITION  OCT- / OCT+ OCTAVE"},
 {"K1 CHANNEL OFF / 1-16","K2 MODE OFF / ON / MIDI ONLY","K3 CLOCK OFF / OUT / IN","K4 MIDI IN OMNI / CH 1-16","FX MUTE SOUND  SCL VIEW FILTER","KEYS AUDITION  OCT- / OCT+ OCTAVE"},
 {"K1 SWING 0-75  K2 HUMAN 0-100","K3 SWING ON 8TH / 16TH","K4 HUMAN TIME+VEL/TIME/VEL","STOPPED ONLY, SAVED WITH SONG","FX MUTE SOUND  SCL VIEW FILTER","KEYS AUDITION  OCT- / OCT+ OCTAVE"},
 {"K1 FROM  K2 TO  K3 TARGET","K4 TRANSPOSE AMOUNT 1-12","SELECT  FROM BAR","REC COPY  FX MOVE","SEQ DELETE  ARP INSERT","OCT- DOWN  OCT+ UP BY K4","SCL  ONLY SELECTED SOUND","(COPY MOVE TRANSPOSE)","STOPPED ONLY","KEYS  AUDITION ONLY"}};
static const char *const hall[6]={"PLAY START/STOP  EDIT+ FROM BAR","PRESETS SOUND  ALGORITHM LAYER","ENV UNDO  LFO REDO  SAVE WRITE","GLO SONGS  EDIT NEXT PAGE","HOME STAFF  HOLD: THIS HELP","OCT- + OCT+  FREE PLAY"};
static unsigned hn(void){unsigned n=0;while(n<10 && help[ui.page][n])n++;return n;} /* this page's own rows */
static void input(void){
 uint32_t keys=plat_keys(),btn=plat_buttons(),pressed=btn&~ui.buttons,f=plat_lock();unsigned k;int d,cl=0,en[7];
 en[0]=plat_enc(EN_SELECT);en[1]=plat_enc(EN_PRESET);en[2]=plat_enc(EN_ALGO);for(k=0;k<4;k++)en[3+k]=plat_enc(EN_K1+k);
 if(ui.help){ /* sticky help: all input is consumed; a button, key or turn closes it (SELECT/K1 scroll a 2-page list); HOME bounce ignored for 250 ms */
  int sc=hn()+6>12 && (en[0]||en[3]);
  if(sc)ui.hp^=1;
  if((pressed&~(plat_ms()-ui.ho<250?1u<<B_HOME:0)) || (keys&~ui.keys) || en[1]||en[2]||en[4]||en[5]||en[6]||(!sc&&(en[0]||en[3])))cl=1;
  pressed=0;memset(en,0,sizeof en);ui.edit_used=1;
 }
 ui.erase=(btn>>B_EDIT)&1u;if(!engine.playing)ui.lcp=0;
 if(pressed&(1u<<B_EDIT))ui.edit_used=ui.gest=0;
 if(ui.erase && ((pressed&~(1u<<B_EDIT)) || (keys&~ui.keys) || en[0]||en[1]||en[2]||en[3]||en[4]||en[5]||en[6]))ui.edit_used=1; /* any other input while EDIT is held: not a page tap */
 {uint32_t pkt;int port;while(plat_midi_in(&pkt,&port)){unsigned rt=pkt>>8&0xFFu,st=rt&0xF0u,p=pkt>>16&0x7Fu,v=pkt>>24&0x7Fu;if(song.midi_clock==2){if(rt==0xF8u)sc_ext_clock(&engine,&song,plat_ms()*1000u);else if(rt==0xFAu||rt==0xFBu)sc_ext_start(&engine,&song,rt&1);else if(rt==0xFCu)sc_stop(&engine);} /* clock in: read here (main loop, under the lock) and stamped on read, in ms (the 24-interval mean averages the quantizing) */
  if(rt<0xF0u && song.midi_in && (rt&15u)!=song.midi_in-1u)continue; /* channel filter: notes only, realtime passes */
  if(st==0x90u && v && p>=36 && p<=84)enter(p,v);else if(ui.free && (st==0x80u || st==0x90u))sc_release(&engine,&song,p,ui.instrument);}}
 for(k=0;k<NKEYS;k++){int pitch=53+(int)k+12*ui.octave;
  if((keys&~ui.keys)&(1u<<k)){if(pitch>=36 && pitch<=84){enter(pitch,ui.velocity);ui.kp[k]=ui.free?pitch:0;ui.ki[k]=ui.instrument;}else app_say("C2 TO C6","");}
  if((ui.keys&~keys)&(1u<<k) && ui.kp[k]){sc_release(&engine,&song,ui.kp[k],ui.ki[k]);ui.kp[k]=0;}} /* FREE: gate = key held */
 if(pressed&(1u<<B_PLAY)){if(engine.playing)sc_stop(&engine);else if(ui.erase)sc_start_at(&engine,&song,ui.cursor/(song.beats*96u)*(song.beats*96u));else sc_start(&engine);} /* EDIT+PLAY: from the cursor's bar */
 if(ui.page==6){ /* BARS: REC FX SEQ ARP are the ops (OCT- OCT+ below) */
  static const uint8_t ob[4]={B_REC,B_FX,B_SEQ,B_ARP};unsigned i;
  for(i=0;i<4;i++)if(pressed>>ob[i]&1)bars_op(i);
  pressed&=~((1u<<B_REC)|(1u<<B_FX)|(1u<<B_SEQ)|(1u<<B_ARP));
 }
 {uint32_t oc=(1u<<B_OCTDN)|(1u<<B_OCTUP); /* OCT- + OCT+ together (second within 150 ms): FREE PLAY toggles, the first press's shift is undone. BARS (transpose down/up) and STAFF with EDIT held (spelling toward flat/sharp): a single press waits out the 150 ms, then acts */
  if(pressed&oc){if((btn&oc)==oc && ((pressed&oc)==oc || plat_ms()-ui.ot<=150)){if((pressed&oc)!=oc)ui.octave=ui.o0;ui.bp=0;ui.free^=1;pressed&=~oc;app_say("FREE PLAY",ui.free?"ON":"OFF");}else{ui.o0=ui.octave;ui.ot=plat_ms();if(ui.page==6 || (!ui.page && ui.erase)){ui.bp=pressed&oc;pressed&=~oc;}}}
  if(ui.bp && plat_ms()-ui.ot>150){if(ui.page==6)bars_op(ui.bp>>B_OCTUP&1?4:5);else spell(ui.bp>>B_OCTUP&1?1:-1);ui.bp=0;}}
 if(pressed&(1u<<B_OCTDN))ui.octave=clamp(ui.octave-1,-2,2);
 if(pressed&(1u<<B_OCTUP))ui.octave=clamp(ui.octave+1,-2,2);
 if(pressed&(1u<<B_GLO)){ui.page=ui.page==2?0:2;ui.confirm=0;}
 if(cl)ui.help=0;
 if(pressed&(1u<<B_HOME)){ui.hold=plat_ms();ui.hs=1;}
 if(!(btn&(1u<<B_HOME))){if(ui.hs){ui.page=0;ui.cursor=0;ui.confirm=0;}ui.hs=0;} /* a tap acts on release */
 else if(ui.hs && plat_ms()-ui.hold>=600){ui.help=1;ui.hp=0;ui.hs=0;ui.ho=plat_ms();} /* held 600 ms: sticky help overlay */
 if(ui.page==1){ /* MIX: FX mutes, SEL solos the selected layer (live, any time, not saved) */
  unsigned m=1u<<ui.layer;
  if(pressed&(1u<<B_FX)){engine.mute^=m;snprintf(ui.message,sizeof ui.message,"LAYER %u %s",ui.layer+1,engine.mute&m?"MUTED":"UNMUTED");ui.until=plat_ms()+1600;}
  if(pressed&(1u<<B_SEL)){engine.solo^=m;snprintf(ui.message,sizeof ui.message,"LAYER %u %s",ui.layer+1,engine.solo&m?"SOLO":"UNSOLO");ui.until=plat_ms()+1600;}
 }else{ /* elsewhere (not BARS: FX is MOVE there): FX mutes the selected sound (live, not saved, volume untouched), SEL = view filter */
  if(pressed&(1u<<B_SEL))ui.filter^=1;
  if(pressed&(1u<<B_FX)){if(ui.erase && !ui.page){if(stopped())clear_cursor();}else{engine.smute^=1u<<ui.instrument;app_say(sc_names[ui.instrument],engine.smute>>ui.instrument&1?"MUTED":"UNMUTED");}}
 }
 if((ui.buttons&(1u<<B_EDIT)) && !(btn&(1u<<B_EDIT)) && !ui.edit_used)ui.page="\1\3\0\4\5\6\0"[ui.page];
 if(!engine.playing){
  if(pressed&(1u<<B_ENV))undo();
  if(pressed&(1u<<B_LFO))redo();
  if(ui.page==0){ /* grid, loop and bar copy belong to the staff */
   if(pressed&(1u<<B_SEQ)){ui.grid=ui.grid==96?48:ui.grid==48?24:ui.grid==24?32:ui.grid==32?16:96;ui.cursor=ui.cursor/ui.grid*ui.grid;}
   if(pressed&(1u<<B_ARP)){checkpoint();song.loop^=1;}
  }
  if(pressed&(1u<<B_REC)){
   if(ui.page==2){
    if(ui.confirm!=(ui.dm?9:ui.slot+1)){ui.confirm=ui.dm?9:ui.slot+1;app_say(!ui.dm && !(ui.used>>ui.slot&1)?"REC AGAIN: NEW SONG":"REC AGAIN TO LOAD","");}
    else if(ui.dm){load_demo();ui.confirm=0;}
    else if(!(ui.used>>ui.slot&1)){char t[24];snprintf(t,sizeof t,"NEW SONG IN SLOT %u",ui.slot+1);checkpoint();bind();sc_default(&song);ui.dirty=ui.cursor=ui.confirm=0;app_say(t,"");} /* empty slot: a blank song bound to it, one undo step */
    else{int n;plat_unlock(f);n=plat_store_load(ui.slot,wire,sizeof wire);f=plat_lock();checkpoint();if(n>0 && sc_decode(&song,wire,n)){bind();ui.dirty=0;ui.cursor=0;app_say("LOADED","");}else app_say("EMPTY OR BAD SLOT","");ui.confirm=0;}
   }else if(ui.page==0){unsigned bar=song.beats*96,start=ui.cursor/bar*bar;checkpoint();if(!sc_duplicate(&song,start,bar)){undo();app_say("COPY FAILED","");}else app_say("BAR COPIED","");}
  }
  if(pressed&(1u<<B_SAVE)){if(ui.held){app_say("BACKUP BUSY","");}else if(ui.dm)app_say("PICK A SLOT","");else{unsigned n=sc_encode(&song,wire);int rc;sc_stop(&engine);plat_unlock(f);rc=n?plat_store_save(ui.slot,wire,n):-1;if(!rc){ui.dirty=0;ui.ps=ui.slot;ui.used|=1u<<ui.slot;asave();}f=plat_lock();if(!rc){app_say("SAVED","");}else app_say("SAVE FAILED","");}}
 }else if(pressed&((1u<<B_SAVE)|(1u<<B_ENV)|(1u<<B_LFO)|(ui.page==0?(1u<<B_REC)|(1u<<B_SEQ)|(1u<<B_ARP):ui.page==2?1u<<B_REC:0)))app_say("STOP FIRST","");
 if(en[0]){if(!ui.page){if(ui.erase)jump(en[0]);else ui.cursor=clamp((int)ui.cursor+en[0]*(int)ui.grid,0,(song.end-1)/ui.grid*ui.grid);}else if(ui.page==2)pick(en[0]);else if(ui.page==6)bars_knob(0,en[0]);} /* SELECT: cursor / FROM bar / slot */
 if(en[1])ui.instrument=WRAP(ui.instrument,en[1],SC_SOUNDS);
 if(en[2])ui.layer=WRAP(ui.layer,en[2],8);
 if(ui.erase && ui.page);else if(ui.page>2){for(k=0;k<4;k++)if(en[3+k] && stopped())shape(k,en[3+k]);}else{ /* EDIT+knob off the staff: nothing */
 if((d=en[3])){if(ui.page==2)pick(d);else if(stopped()){if(ui.page==1){int b=clamp(song.beats+d,2,12);if(b!=song.beats){checkpoint();song.beats=b;}}else{checkpoint();song.tempo=clamp(song.tempo+d,30,300);}}}
 if((d=en[4])){if(note_edit(0,d)){}else if(ui.page==1){if(stopped()){checkpoint();song.volume[ui.instrument]=clamp(song.volume[ui.instrument]+d,0,127);}}else ui.velocity=clamp((int)ui.velocity+d,1,127);}
 if((d=en[5])){if(note_edit(1,d)){}else if(ui.page==1){if(stopped()){checkpoint();song.layers[ui.layer]=clamp(song.layers[ui.layer]+d,0,127);}}else ui.length=clamp((int)ui.length+d*(int)LSTEP,(int)LSTEP,1536)/LSTEP*LSTEP;}
 if((d=en[6]) && ui.page<2 && stopped()){unsigned minend=song.count?song.notes[song.count-1].tick+1:1;checkpoint();song.end=clamp(song.end+d*96,minend>song.loop_start?minend:(unsigned)song.loop_start+1,65535);if(ui.cursor>=song.end)ui.cursor=(song.end-1)/ui.grid*ui.grid;}}
 if(ui.cursor>=song.end)ui.cursor=(song.end-1)/ui.grid*ui.grid;
 if(keys!=ui.keys||btn!=ui.buttons)au.act=plat_ms();
 if(ui.page!=2){ui.dm=0;ui.ps=ui.slot;}
 ui.keys=keys;ui.buttons=btn;plat_unlock(f);plat_glow(1);plat_leds((engine.playing?1u<<B_PLAY:0)|(ui.dirty?1u<<B_SAVE:0),keys);
 if(ui.page==2 && ui.pp!=2){ui.used=0;for(k=0;k<8;k++)ui.used|=(plat_store_head(k,wire,1)>=0)<<k;} /* which slots hold a song, read on entering the song book */
 ui.pp=ui.page;
}
void app_input_only(void){input();}
static int diatonic(unsigned p){static const uint8_t d[12]={0,0,1,1,2,3,3,4,4,5,5,6};return (int)(p/12)*7+d[p%12];}
/* Palette and notes share the exact pixel assets used by the browser. */
#include "assets/stamps.h"
static const char *const character_names[SC_SOUNDS]={"MARIO","MUSHROOM","YOSHI","STAR","FLOWER","GAME BOY","DOG","CAT","PIG","SWAN","FACE","PLANE","BOAT","CAR","HEART","PIRANHA","COIN","SHY GUY","BOO","LUIGI","PEACH","FEATHER","BULLET BILL","GOOMBA","BOB-OMB","SPINY","FRUIT","1-UP","MOON","EGG","GNOME"};
#define CLEF 31 /* stamp ROM: ids 0..30 = sounds, 31 = treble clef */
#define INK RGB(39,45,59)
#define PAPER RGB(255,249,229)
#define ACCENT RGB(226,69,62)
#define TEAL RGB(27,121,122)
#define MUTED RGB(157,153,137)
static void stamp(int x,int y,unsigned id,unsigned scale,unsigned mute){
 unsigned a,b;
 for(a=0;a<18;a++)for(b=0;b<16;b++){
  uint16_t c=stamp_pixels[id][a*16+b];
  if(c!=1 && !(mute && (a+b&1)))cv_rect(x+(int)(b*scale),y+(int)(a*scale),scale,scale,c);
 }
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
  tile(122,2,55,"K3 GRID",song.sgrid?"16TH":"8TH");tile(181,2,55,"K4 HUMAN",song.hmode==1?"TIME":song.hmode?"VEL":"T+V");
 }else if(ui.page==6){
  snprintf(t,sizeof t,"%u",ui.bf+1);tile(4,2,55,"K1 FROM",t);
  snprintf(t,sizeof t,"%u",ui.bt+1);tile(63,2,55,"K2 TO",t);
  snprintf(t,sizeof t,"%u",ui.bg+1);tile(122,2,55,"K3 TARGET",t);
  snprintf(t,sizeof t,"%d",ui.bx);tile(181,2,55,"K4 SEMI",t);
 }else{
  if(song.midi_ch[u])snprintf(t,sizeof t,"%u",song.midi_ch[u]);else snprintf(t,sizeof t,"OFF");tile(4,2,55,"K1 CH",t);
  tile(63,2,55,"K2 MODE",song.midi_mode==2?"ONLY":song.midi_mode?"ON":"OFF");
  tile(122,2,55,"K3 CLOCK",CKN);if(song.midi_in)snprintf(t,sizeof t,"%u",song.midi_in);else snprintf(t,sizeof t,"OMNI");tile(181,2,55,"K4 IN",t);
 }
}
static const char *const hname[7]={"STAFF","MIX","SONG BOOK","SOUND","MIDI","GROOVE","BARS"};
static void draw_help(void){ /* full screen in three 80-row bands; sticky, closed by the next input */
 unsigned b,i,n=hn(),m=n+6;char t[24];
 snprintf(t,sizeof t,m>12?"%u/2  SELECT: %s":"ANY BUTTON CLOSES",ui.hp+1,ui.hp?"BACK":"MORE");
 for(b=0;b<3;b++){
  int o=-80*(int)b;cv_begin(240,80,INK);
  cv_rect(0,o,4,32,ACCENT);cv_text(10,o+4,&FONT_B,hname[ui.page],C_WHITE);cv_text(m>12?128:150,o+8,&FONT_T,t,RGB(162,182,188));
  for(i=0;i<12 && ui.hp*12+i<m;i++){unsigned r=ui.hp*12+i;cv_text(10,o+30+(int)i*17,&FONT_T,r<n?help[ui.page][r]:hall[r-n],C_WHITE);}
  cv_blit(0,80*b);
 }
}
static void draw(void){
 char t[64],note[8];unsigned i,step=ui.grid,emp=!ui.dm && !(ui.used>>ui.slot&1),active=engine.playing?engine.tick:ui.cursor;
 unsigned start=active/(step*6)*step*6,selected=ui.instrument;
 int center=41,focus=diatonic(ui.last+ui.lf),shift=0;
 while(focus<center-8){center-=7;shift--;}
 while(focus>center+9){center+=7;shift++;}
 note_label(ui.last,ui.lf,note);
 /* Header and five-item instrument ribbon: PRESET moves the window. */
 cv_begin(240,72,INK);
 cv_rect(0,0,4,24,ACCENT);
 cv_text(10,3,&FONT_B,"STAMP",C_WHITE);
 snprintf(t,sizeof t,"%03u",song.midi_clock==2 && engine.xinc?(unsigned)(engine.xinc/96+.5f):song.tempo);cv_text(140,3,&FONT_B,t,C_WHITE); /* IN: measured tempo */
 cv_text(168,8,&FONT_T,song.midi_clock==2?"EXT":"BPM",RGB(162,182,188));
 cv_rect(197,4,37,17,ui.free?ACCENT:engine.playing?TEAL:RGB(71,78,88));
 cv_text(201,5,&FONT_XS,ui.free?"FREE":engine.playing?"PLAY":"EDIT",C_WHITE);
 cv_rect(0,25,240,47,PAPER);
 for(i=0;i<5;i++){
  int id=(int)selected+(int)i-2,x=6+(int)i*31;
  if(id<0)id+=SC_SOUNDS;
  if(id>=SC_SOUNDS)id-=SC_SOUNDS;
  if(i==2){cv_rect(x,28,28,24,RGB(250,212,104));cv_rect(x,52,28,2,ACCENT);}
  stamp(x+6,31,(unsigned)id,1,engine.smute>>id&1);
  if(engine.smute>>id&1)cv_text(x+1,29,&FONT_T,"M",ACCENT);
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
  cv_text(10,56,&FONT_T,"REC COPY  FX MOVE  SEQ DEL",INK);cv_text(10,72,&FONT_T,"ARP INSERT  OCT- DOWN  OCT+ UP",INK);
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
  snprintf(t,sizeof t,"K3 SWING %s   K4 HUMAN %s",song.sgrid?"16TH":"8TH",song.hmode==1?"TIME":song.hmode?"VEL":"TIME+VEL");cv_text(10,98,&FONT_T,t,MUTED);
  cv_blit(0,72);
 }else if(ui.page==4){
  cv_begin(240,112,PAPER);
  cv_text(10,4,&FONT_B,"MIDI OUT",INK);
  for(i=0;i<SC_SOUNDS;i++){ /* all 31 on one page: 8 x 4 cells, stamp + channel */
   int x=4+(int)(i%8)*29,y=22+(int)(i/8)*19;
   cv_rect(x,y,28,18,i==selected?TEAL:RGB(233,225,202));
   stamp(x,y,i,1,engine.smute>>i&1);
   if(song.midi_ch[i])snprintf(t,sizeof t,"%u",song.midi_ch[i]);else snprintf(t,sizeof t,"--");
   cv_text(x+17,y+3,&FONT_T,t,i==selected?C_WHITE:INK);
  }
  {char w[8];if(song.midi_in)snprintf(w,sizeof w,"%u",song.midi_in);else snprintf(w,sizeof w,"OMNI");
  snprintf(t,sizeof t,"MODE %s  CLOCK %s  IN %s",song.midi_mode==2?"ONLY":song.midi_mode?"ON":"OFF",CKN,w);}
  cv_text(10,99,&FONT_T,t,TEAL);
  cv_blit(0,72);
 }else if(ui.page==2){
  cv_begin(240,112,PAPER);
  cv_text(10,4,&FONT_B,"SONG BOOK",INK);
  for(i=0;i<9;i++){ /* 8 slots, then the DEMO wide cell */
   int x=i<8?10+(i%4)*57:10,y=24+(i/4)*26,w=i<8?49:223,on=i<8?!ui.dm && i==ui.slot:ui.dm;
   cv_rect(x,y,w,22,on?TEAL:RGB(233,225,202));
   if(i<8)snprintf(t,sizeof t,"%02u",i+1);else snprintf(t,sizeof t,"DEMO  TWINKLE TWINKLE");
   cv_text(i<8?x+14:x+8,y+(i<8?3:4),&FONT_B,t,on?C_WHITE:i<8 && !(ui.used>>i&1)?MUTED:INK);
   if(i<8 && on && !(ui.used>>i&1))cv_rect(x+2,y+2,w-4,18,TEAL),cv_text(x+9,y+6,&FONT_T,"EMPTY",C_WHITE); /* the selected empty slot says so instead of its number */
  }
  cv_text(10,99,&FONT_T,ui.confirm?(emp?"NEW SONG ARMED - REC AGAIN":"LOAD ARMED - REC AGAIN"):ui.dm?"TWINKLE TWINKLE (MARIO PAINT DEMO)":emp?"EMPTY SLOT - REC REC: NEW SONG":"K1 SELECTS A SLOT OR THE DEMO",TEAL);
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
  stamp(5,42,CLEF,1,0);
  for(i=0;i<6;i++){
   int x=39+(int)i*32;unsigned tick=start+i*step;
   if(tick%(song.beats*96)==0)cv_rect(x-5,29,1,67,RGB(157,153,137));
   snprintf(t,sizeof t,"%u",tick/step+1);cv_text(x+3,99,&FONT_T,t,MUTED);
  }
  for(i=0;i<song.count;i++){
   ScNote n=song.notes[i];int y,x,k,fl=n.layer>>7;
   if(n.tick<start || n.tick>=start+step*6 || (ui.filter && n.instrument!=selected))continue;
   y=55-(diatonic(n.pitch+fl)-center)*4;x=39+(n.tick-start)*32/step;
   if(y<27 || y>91){cv_text(x+4,y<27?16:88,&FONT_T,y<27?"+":"-",MUTED);continue;}
   if(y>=79)for(k=79;k<=y;k+=8)cv_rect(x-3,k,22,1,MUTED);
   if(y<=31)for(k=31;k>=y;k-=8)cv_rect(x-3,k,22,1,MUTED);
   if(!engine.playing && ATC(&n))cv_rect(x-2,y-10,20,20,TEAL); /* the cursor's notes get a teal box: what EDIT+key / EDIT+FX erase */
   stamp(x,y-9,n.instrument,1,engine.smute>>n.instrument&1);
   if(fl){static const uint8_t fg[8]={8,8,8,8,14,9,9,14};int r,c;for(r=0;r<8;r++)for(c=0;c<4;c++)if(fg[r]>>(3-c)&1)cv_rect(x-7+c,y-9+r,1,1,INK);} /* flat sign: 4x8 pixels, bowl on the note's line */
   else if(sc_black(n.pitch))cv_text(x-7,y-8,&FONT_T,"#",INK);
  }
  cv_rect(cursor_x-3,28,2,66,ACCENT);
  if(engine.playing){ /* hop: one arc per grid step, tall when a note starts at the next step (landing), else a small bounce */
   unsigned p=active%step,h=2;for(i=0;i<song.count;i++)if(song.notes[i].tick/step==active/step+1){h=8;break;}
   stamp(cursor_x,11-(int)(h*4*p*(step-p)/(step*step)),selected,1,engine.smute>>selected&1);}
  else cv_rect(cursor_x+4,19,5,4,ACCENT);
  if(ui.free)cv_text(40,85,&FONT_XS,"FREE PLAY",ACCENT);else if(!song.count)cv_text(40,85,&FONT_XS,"PLAY A KEY TO PLACE A NOTE",TEAL);
  cv_blit(0,72);
 }
 /* Four encoder cards, then one concise contextual hint. */
 cv_begin(240,56,INK);
 if(ui.page>2)tiles_shape();else{
 snprintf(t,sizeof t,ui.page==1?"%u/4":ui.page==2?"%02u":"%u",ui.page==1?song.beats:ui.page==2?ui.slot+1:song.tempo);if(ui.dm)snprintf(t,sizeof t,"DEMO");tile(4,2,55,ui.page==2?"K1 SLOT":ui.page==1?"K1 BEATS":"K1 TEMPO",t);
 snprintf(t,sizeof t,"%u",ui.page==1?song.volume[selected]:ui.velocity);tile(63,2,55,ui.page==1?"K2 SOUND":"K2 VEL",t);
 snprintf(t,sizeof t,ui.page==1?"%u":"1/%u",ui.page==1?song.layers[ui.layer]:384/ui.length);if(ui.page!=1 && (ui.length>384 || 384%ui.length!=0))snprintf(t,sizeof t,"%u T",ui.length);if(ui.page!=1 && gname(ui.length))snprintf(t,sizeof t,"%s",gname(ui.length));tile(122,2,55,ui.page==1?"K3 LAYER":"K3 LEN",t);
 if(ui.page==2)tile(181,2,55,"K4","-");else{unsigned b=(song.end+95)/96,r=b%song.beats; /* END in bars.beats, so every quarter step shows: 4B+2 = 4 bars and 2 beats */
 if(r)snprintf(t,sizeof t,"%uB+%u",b/song.beats,r);else snprintf(t,sizeof t,"%u BAR",b/song.beats);tile(181,2,55,"K4 END",t);}}
 if(plat_ms()<ui.until)cv_text(5,44,&FONT_T,ui.message,RGB(250,212,104));
 else{snprintf(t,sizeof t,ui.page==3?"EDIT NEXT / FX MUTE SOUND":ui.page==4?"EDIT NEXT / K4 IN CH / FX MUTE":ui.page==6?"REC CPY FX MOV SEQ DEL ARP INS OCT-/+":ui.page==5?"EDIT NEXT / K3 GRID / K4 MODE":ui.page==2?emp?"REC REC: NEW SONG / SAVE WRITES":"SAVE WRITE / REC LOAD / HOME EXIT":ui.page==1?"FX MUTE / SEL SOLO / ALGO LAYER":ui.free?"FREE PLAY / OCT- + OCT+ EXITS":"%s GRID / %s / %u NOTES / %s",gname(step),SPN[ui.sp],song.count,ui.dirty?"UNSAVED":"SAVED");if(!ui.page && !ui.free && !engine.playing && (i=at_cursor()))snprintf(t,sizeof t,"EDIT+KEY ERASE / %u HERE / %s",i,ui.dirty?"UNSAVED":"SAVED");cv_text(5,44,&FONT_T,t,RGB(162,182,188));}
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
 if(!strcmp(s,"page"))*v=ui.page;else if(!strcmp(s,"autosaves"))*v=au.n;else if(!strcmp(s,"slot"))*v=ui.dm?8:ui.slot;else if(!strcmp(s,"notes"))*v=song.count;else if(!strcmp(s,"playing"))*v=engine.playing;else if(!strcmp(s,"dirty"))*v=ui.dirty;else if(!strcmp(s,"instrument"))*v=ui.instrument;else if(!strcmp(s,"tick"))*v=engine.tick;else if(!strcmp(s,"triggers"))*v=engine.triggers;else if(!strcmp(s,"tempo"))*v=song.tempo;else if(!strcmp(s,"cursor"))*v=ui.cursor;else if(!strcmp(s,"grid"))*v=ui.grid;else if(!strcmp(s,"beats"))*v=song.beats;else if(!strcmp(s,"length"))*v=ui.length;
 else if(!strcmp(s,"tune"))*v=song.tune[ui.instrument];else if(!strcmp(s,"tone"))*v=song.tone[ui.instrument];else if(!strcmp(s,"decay"))*v=song.decay[ui.instrument];else if(!strcmp(s,"pan"))*v=song.pan[ui.instrument];
 else if(!strcmp(s,"midi_ch"))*v=song.midi_ch[ui.instrument];else if(!strcmp(s,"midi_mode"))*v=song.midi_mode;else if(!strcmp(s,"midi_clock"))*v=song.midi_clock;else if(!strcmp(s,"tick8"))*v=engine.tick/8;else if(!strcmp(s,"ext_bpm"))*v=(int)(engine.xinc/96+.5f);
 else if(!strcmp(s,"note_vel")||!strcmp(s,"note_len")||!strcmp(s,"note_flat")){unsigned i;for(i=0;i<song.count;i++)if(song.notes[i].tick==ui.cursor && song.notes[i].instrument==ui.instrument && (song.notes[i].layer&127)==ui.layer)break;if(i==song.count)return 0;*v=s[5]=='v'?song.notes[i].velocity:s[5]=='l'?song.notes[i].duration:song.notes[i].layer>>7;}
 else if(!strcmp(s,"swing"))*v=song.swing;else if(!strcmp(s,"sgrid"))*v=song.sgrid;else if(!strcmp(s,"hmode"))*v=song.hmode;else if(!strcmp(s,"midi_in"))*v=song.midi_in;else if(!strcmp(s,"smute"))*v=engine.smute;else if(!strcmp(s,"volume"))*v=song.volume[ui.instrument];else if(!strcmp(s,"filter"))*v=ui.filter;else if(!strcmp(s,"loop"))*v=song.loop;else if(!strcmp(s,"at_cursor"))*v=at_cursor();else if(!strcmp(s,"velocity"))*v=ui.velocity;else if(!strcmp(s,"human"))*v=song.human;else if(!strcmp(s,"mute"))*v=engine.mute;else if(!strcmp(s,"solo"))*v=engine.solo;else if(!strcmp(s,"layer"))*v=ui.layer;else if(!strcmp(s,"help"))*v=ui.help?ui.hp+1:0;else if(!strcmp(s,"used"))*v=ui.used;else if(!strcmp(s,"spell"))*v=ui.sp;else if(!strcmp(s,"last_flat"))*v=ui.lf;else if(!strcmp(s,"label")){char t[8];note_label(ui.last,ui.lf,t);*v=t[0]<<8|t[1];} /* label: the header name's first two characters */else if(!strcmp(s,"free"))*v=ui.free;else if(!strcmp(s,"octave"))*v=ui.octave;
 else if(!strcmp(s,"stamps"))*v=sizeof stamp_pixels/sizeof *stamp_pixels;else if(!strcmp(s,"clef"))*v=CLEF;
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
 hist_count=hist_pos=ui.nsp=ui.pp=0;ui.ps=ui.slot;ui.dirty=ui.confirm=0;ui.cursor=0;au.last=plat_ms()-20000u; /* the old working copy must not resurrect: rewritten after 3 s idle */
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
SC_WEB int stamp_select(unsigned id){if(id>=SC_SOUNDS)return 0;ui.instrument=id;return 1;}
SC_WEB int stamp_demo(void){return load_demo();}
SC_WEB void stamp_transport(void){if(engine.playing)sc_stop(&engine);else sc_start(&engine);}
#endif

