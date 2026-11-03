"""Strict SMP text subset to SCF1 (all 31 SMP instruments). No audio or character art is bundled."""
import argparse, pathlib, re, struct, zlib
ROLES='MARIO MUSHROOM YOSHI STAR FLOWER GAMEBOY DOG CAT PIG SWAN FACE PLANE BOAT CAR HEART PIRANHA COIN SHYGUY BOO LUIGI PEACH FEATHER BULLETBILL GOOMBA BOBOMB SPINY FRUIT ONEUP MOON EGG GNOME'.split() # SMPInstrument enum order = our sound ids (by name: the SF2 programs of COIN and PIRANHA are swapped, not the order)

def convert(text,acknowledge=True):  # acknowledge: kept for old callers, no longer required
    lines=[s.strip() for s in text.splitlines() if s.strip()]
    if not lines or not lines[0].startswith('TEMPO:'): raise ValueError('Requires SMP text, not legacy MPC or an arrangement')
    props={}
    for token in lines[0].split(','):
        key,sep,value=token.strip().partition(':')
        if not sep or key not in ('TEMPO','EXT','TIME','SOUNDSET') or key in props: raise ValueError('Unknown/duplicate property: '+key)
        props[key]=value.strip()
    if set(props)!=set(('TEMPO','EXT','TIME','SOUNDSET')):raise ValueError('Missing SMP header property')
    tempo=float(props['TEMPO'])
    if not 30<=tempo<=300 or tempo!=int(tempo):raise ValueError('Requires integer tempo 30..300; no rounding performed')
    if int(props['EXT'])!=0:raise ValueError('Note extensions unsupported')
    if props['TIME'] not in ('3/4','4/4','3','4'):raise ValueError('Only 3/4 and 4/4 supported')
    beats=int(props['TIME'][0]);notes=[];seen=set();last=-1
    for lineno,line in enumerate(lines[1:],2):
        parts=[s.strip() for s in line.split(',')];m=re.fullmatch(r'(\d+):(\d+)',parts[0])
        if not m:raise ValueError(f'Line {lineno}: invalid position')
        bar,beat=map(int,m.groups());tick=((bar-1)*beats+beat)*96
        if bar<1 or not 0<=beat<beats or tick in seen:raise ValueError(f'Line {lineno}: invalid/duplicate position')
        seen.add(tick);last=max(last,tick)
        if not re.fullmatch(r'VOL:\s*\d+',parts[-1]):raise ValueError(f'Line {lineno}: requires VOL')
        vel=int(parts[-1].split(':')[1]);
        if not 0<=vel<=127:raise ValueError('Volume out of range')
        for token in parts[1:-1]:
            m=re.fullmatch(r'([A-Z]+) ([A-G])([2-6])([b#BX]?)',token)
            if not m:raise ValueError(f'Line {lineno}: unsupported note/mute modifier: {token}')
            inst,letter,octave,acc=m.groups()
            if inst not in ROLES:raise ValueError('Unsupported instrument: '+inst)
            pitch=(int(octave)+1)*12+dict(C=0,D=2,E=4,F=5,G=7,A=9,B=11)[letter]+{'':0,'b':-1,'#':1,'B':-2,'X':2}[acc]
            if not 36<=pitch<=84:raise ValueError('Pitch outside C2..C6')
            flat=acc in 'bB' and pitch%12 in (1,3,6,8,10)  # flat spelling only where the pitch is a black key; Cb, Ebb, #/X keep natural/sharp spelling
            notes.append([tick,0,pitch,ROLES.index(inst),vel,128 if flat else 0])
    if len(notes)>384:raise ValueError('More than 384 events; nothing written')
    end=((max(0,last)//(beats*96))+1)*beats*96
    if end>65535:raise ValueError('Song exceeds 65535 ticks')
    notes.sort(key=lambda n:n[0])
    # SMP EXT=0 stops every previous pitch of the same instrument at its next occupied line.
    for note in notes:
        following=next((n[0] for n in notes if n[0]>note[0] and n[3]==note[3]),end)
        note[1]=following-note[0]
    v2=any(n[5] or n[3]>14 for n in notes)  # the flat flag (layer bit 7) or a sound above 14 needs SCF v2 (31 sounds: volume 127 at 16, layers 127 at 47, tone 127 at 86), else v1
    head=bytearray(216 if v2 else 48);struct.pack_into('<4sHHHHHBB',head,0,b'SCF1',2 if v2 else 1,len(notes),int(tempo),end,0,0,beats)
    if v2:head[16:55]=bytes([127])*39;head[86:117]=bytes([127])*31
    else:head[16:39]=bytes([127])*23
    payload=bytes(head)+b''.join(struct.pack('<HHBBBB',*n) for n in notes)
    return payload+struct.pack('<I',zlib.crc32(payload))

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('input',type=pathlib.Path);ap.add_argument('output',type=pathlib.Path);ap.add_argument('--placeholder-bank',action='store_true',help='accepted and ignored (old scripts)');a=ap.parse_args()
    try: data=convert(a.input.read_text(),a.placeholder_bank)
    except (ValueError,OverflowError) as e:ap.exit(2,str(e)+'\n')
    a.output.write_bytes(data);print(f'{len(data)} bytes; musical events converted. Note: one sample per instrument and simplified envelopes, so not identical to SMP/MPC playback.')
if __name__=='__main__':main()
