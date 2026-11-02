// SPDX-License-Identifier: MIT
// Executes the actual preview WASM module in Node, not PI32 firmware emulation.
import fs from 'node:fs';import assert from 'node:assert/strict';
const imports=new Proxy({}, {get:()=>()=>0});
const {instance}=await WebAssembly.instantiate(fs.readFileSync(process.argv[2]),{env:imports,wasi_snapshot_preview1:imports});
const e=instance.exports;e._initialize?.();e.web_boot();e.web_master(4096);
const step=(ms=40)=>{for(let i=0;i<Math.ceil(ms*44.1/128);i++)e.web_render(128);};
const tap=bit=>{e.web_buttons(1<<bit);step();e.web_buttons(0);step();};
e.web_keys(1<<7);step();e.web_keys(0);step();assert.equal(e.stamp_count(),1);
let n=e.stamp_export();assert.equal(n,228);const saved=new Uint8Array(e.memory.buffer,e.stamp_buffer(),n).slice();
const input=fs.readFileSync(process.argv[3]);new Uint8Array(e.memory.buffer,e.stamp_buffer(),input.length).set(input);assert.equal(e.stamp_import(input.length),1);assert.equal(e.stamp_count(),11);
tap(10); // PLAY
new Uint8Array(e.memory.buffer,e.stamp_buffer(),saved.length).set(saved);assert.equal(e.stamp_import(saved.length),0);
let peak=0;for(let i=0;i<100;i++){e.web_render(128);for(const s of new Float32Array(e.memory.buffer,e.web_out_l(),128))peak=Math.max(peak,Math.abs(s));}assert(peak>0.001);
tap(10);tap(7); // STOP then SAVE
assert(e.web_store_writes()>0);e.web_boot();assert.equal(e.stamp_count(),11);
new Uint8Array(e.memory.buffer,e.stamp_buffer(),saved.length).set(saved);assert.equal(e.stamp_import(saved.length),1);assert.equal(e.stamp_count(),1);
console.log('WASM: key editing, import/export, playback audio, import refusal during playback, persistence/reboot passed; peak',peak);


assert.equal(e.stamp_select(30),1);assert.equal(e.stamp_instrument(),30);
assert.equal(e.stamp_select(31),0);assert.equal(e.stamp_instrument(),30);
assert.equal(e.stamp_demo(),1);assert.equal(e.stamp_count(),66);assert.equal(e.stamp_dirty(),1);
e.stamp_transport();assert.equal(e.stamp_playing(),1);assert.equal(e.stamp_demo(),0);
e.stamp_transport();assert.equal(e.stamp_playing(),0);
tap(2); // ENV: undo demo
assert.equal(e.stamp_count(),1);
console.log('WASM: palette bounds, 66-note Twinkle demo, demo undo, transport and playback protection passed');
