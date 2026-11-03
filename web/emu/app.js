// SPDX-License-Identifier: MIT
const names=['Mario','Mushroom','Yoshi','Star','Flower','Game Boy','Dog','Cat','Pig','Swan','Face','Plane','Boat','Car','Heart','Piranha','Coin','Shy Guy','Boo','Luigi','Peach','Feather','Bullet Bill','Goomba','Bob-omb','Spiny','Fruit','1-Up','Moon','Egg','Gnome'];
const files=['mario','mushroom','yoshi','star','flower','gameboy','dog','cat','pig','swan','face','plane','boat','car','heart','piranha','coin','shyguy','boo','luigi','peach','feather','bulletbill','goomba','bobomb','spiny','fruit','oneup','moon','egg','gnome'];
const roles=['Piano','Kick','Zip','Bell','Trumpet','Pulse','Dog','Cat','Pig','Horn','Baby','Guitar','Block','Organ','Bass','Piranha','Coin','Shy Guy','Boo','Luigi','Peach','Feather','Bullet','Goomba','Bob-omb','Spiny','Fruit','1-Up','Moon','Egg','Gnome'];
const $=id=>document.getElementById(id);
let status,port,latest={count:0,dirty:false,playing:false,instrument:0},armedUntil=0;
// Visible before audio starts, disabled until the native module is ready.
export function mount(){
 const palette=$('stamp-palette');if(palette.children.length)return;
 files.forEach((file,id)=>{
  const b=document.createElement('button');b.type='button';b.disabled=true;b.title=`${names[id]} / ${roles[id]}`;b.setAttribute('aria-label',b.title);b.setAttribute('aria-pressed',id===0?'true':'false');
  const img=document.createElement('img');img.src=`stamps/${file}.png`;img.alt='';b.append(img);b.onclick=()=>port.postMessage({type:'app',action:'select',id});palette.append(b);
 });
}
mount();
export function attach({node,status:show}) {
 status=show;port=node.port;
 $('demo').onclick=()=>{
  if(latest.playing){status('Stop playback before loading the demo.');return;}
  if(latest.count && Date.now()>armedUntil){armedUntil=Date.now()+5000;$('demo').textContent='Replace current song? Click again';setTimeout(()=>{if(Date.now()>=armedUntil)$('demo').textContent='Load Twinkle demo';},5100);return;}
  armedUntil=0;port.postMessage({type:'app',action:'demo'});$('demo').textContent='Load Twinkle demo';
 };
 $('transport').onclick=()=>port.postMessage({type:'app',action:'transport'});
 $('import-song').onchange=async()=>{
  const input=$('import-song'),file=input.files[0];if(!file)return;
  if(latest.playing){status('Stop playback before importing.');input.value='';return;}
  if(file.size>3292){status('This song exceeds the 384-note capacity.');input.value='';return;}
  if(latest.count && !window.confirm('Replace the current song? ENV can undo the import.')){input.value='';return;}
  try{const bytes=new Uint8Array(await file.arrayBuffer());port.postMessage({type:'app',action:'import',bytes});}catch{status('Could not read that song file.');}input.value='';
 };
 $('export-song').onclick=()=>port.postMessage({type:'app',action:'export'});
}
export function ready(){for(const el of document.querySelectorAll('.palette button,#demo,#transport,#import-song,#export-song'))el.disabled=false;}
export function frame(m){if(m.stamp)update(m.stamp);}
function update(s){
 latest=s;
 [...$('stamp-palette').children].forEach((b,id)=>b.setAttribute('aria-pressed',id===s.instrument?'true':'false'));
 $('selected-name').textContent=`${names[s.instrument]} / ${roles[s.instrument]}`;
 $('song-count').textContent=`${s.count} / 384 notes`;
 $('song-save').textContent=s.dirty?'Unsaved changes':'Saved';
 $('transport').textContent=s.playing?'Stop playback':'Play song';
 $('transport').setAttribute('aria-pressed',String(!!s.playing));
}
export function message(m){
 if(m.action==='result')status(m.ok?(m.demo?'Twinkle Twinkle loaded. Press Play song.':'Imported. Press SAVE to keep your song.'):'Could not load: stop playback and use a valid SCF1 song.');
 if(m.action==='export'){
  const url=URL.createObjectURL(new Blob([m.bytes],{type:'application/octet-stream'})),a=document.createElement('a');a.href=url;a.download='stamp-song.scf';a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);status('Song exported.');
 }
}
