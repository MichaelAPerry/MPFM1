// SPDX-License-Identifier: MIT
// Runs between audio quanta, never concurrently with the native render callback.
globalThis.fm1App={message(fm1,m){
 const ex=fm1.ex;
 if(m.action==='select'){ex.stamp_select(m.id);
 }else if(m.action==='demo'){fm1.port.postMessage({type:'app',action:'result',ok:ex.stamp_demo(),demo:true});
 }else if(m.action==='transport'){ex.stamp_transport();
 }else if(m.action==='import'){
  const b=new Uint8Array(m.bytes);let ok=0;
  if(b.length<=3204){new Uint8Array(ex.memory.buffer,ex.stamp_buffer(),b.length).set(b);ok=ex.stamp_import(b.length);}
  fm1.port.postMessage({type:'app',action:'result',ok});
 }else if(m.action==='export'){
  const n=ex.stamp_export();fm1.port.postMessage({type:'app',action:'export',bytes:new Uint8Array(ex.memory.buffer,ex.stamp_buffer(),n).slice()});
 }
}};

