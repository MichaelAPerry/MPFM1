// SPDX-License-Identifier: MIT
let status;
export function attach({node,status:show}) {
 status=show;
 const section=document.createElement('section');section.className='bar';
 const label=document.createElement('label');label.textContent='Import SCF1 (replaces current song; undo available): ';
 const input=document.createElement('input');input.type='file';input.accept='.scf';
 input.addEventListener('change',async()=>{
  const file=input.files[0];if(!file)return;
  if(file.size>3124){status('Song exceeds capacity.');return;}
  const bytes=new Uint8Array(await file.arrayBuffer());node.port.postMessage({type:'app',action:'import',bytes});input.value='';
 });label.append(input);section.append(label);
 const button=document.createElement('button');button.textContent='Export current song';button.onclick=()=>node.port.postMessage({type:'app',action:'export'});section.append(button);
 document.querySelector('main').append(section);
}
export function message(m){
 if(m.action==='result')status(m.ok?'Imported. Press SAVE to keep this song.':'Import refused: stop playback and use a valid SCF1 file.');
 if(m.action==='export'){
  const url=URL.createObjectURL(new Blob([m.bytes],{type:'application/octet-stream'}));const a=document.createElement('a');a.href=url;a.download='stamp-song.scf';a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);
 }
}
