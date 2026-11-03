"""MIT (tool only): build assets/sounds/smp_bank.bin from Super Mario Paint's soundset3.sf2 (not redistributed here, see assets/sounds/NOTICE.md).
Loops only where the SF2 enables them (sampleModes), none here: every instrument plays once, full length. One zone per instrument (the one whose key range covers C4 = 60, else the nearest), resampled to 22050 Hz mono (Blackman-windowed sinc),
level-matched, IMA ADPCM (tools/adpcm.py). Needs numpy. Deterministic: same SF2 -> same bytes."""
import argparse, math, pathlib, struct, sys
import numpy as np
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parent))
import adpcm, inspect_sf2
ORDER='Mario Mushroom Yoshi Star Flower Gameboy Dog Cat Pig Swan Face Plane Boat Car Heart Piranha Coin Shy Boo Luigi Peach Feather Bullet Goomba Bob Spiny Berry 1-UP Moon Egg Gnome'.split() # SMPInstrument enum order (names match the SF2 presets by prefix; SMP swaps Coin/Piranha: program 15 Coin, 16 Piranha Plant)
RATE=22050;TAPS=12;FC=.46;TARGET_RMS=6000;PEAK=30000
def s16(v):return v-65536 if v>=32768 else v
def resample(x,fs,loop):
    """x float array at fs -> 22050 Hz; loop=(ls,le) source frames or None -> (out, (lso,leo) or None)"""
    r=fs/RATE;n=len(x)
    if loop:
        ls,le=loop;L=le-ls;Lo=max(1,round(L/r));lso=round(ls/r);m=lso+Lo
        pos=np.where(np.arange(m)<lso,np.arange(m)*(ls/lso if lso else r),ls+(np.arange(m)-lso)*(L/Lo))
    else:
        m=int(n/r);pos=np.arange(m)*r
    out=np.empty(m);K=math.ceil(TAPS*max(r,1));ar=np.arange(-K+1,K+1)
    for a in range(0,m,8192):
        p=pos[a:a+8192];base=np.floor(p).astype(np.int64)
        idx=base[:,None]+ar[None,:];t=(idx-p[:,None])/max(r,1)
        if loop:idx=np.where(idx>=le,ls+(idx-ls)%L,idx)
        ok=(idx>=0)&(idx<n);v=np.where(ok,x[np.clip(idx,0,n-1)],0.0)
        h=np.sinc(2*FC*t)*(0.42+0.5*np.cos(math.pi*t/TAPS)+0.08*np.cos(2*math.pi*t/TAPS))*(np.abs(t)<TAPS)
        out[a:a+8192]=(v*h).sum(1)/h.sum(1)
    return out,((lso,lso+Lo) if loop else None)
def build(sf2,header_loops=False):
    d=inspect_sf2.inspect(sf2,True);pcm=np.frombuffer(d['smpl'],'<i2').astype(np.float64);S=d['samples'];P={}
    for p in d['presets']:P[p['name']]=p
    insts=[];rep=[]
    for name in ORDER:
        p=next(v for k,v in P.items() if k.lower().startswith(name.lower()));ii=[g['amount'] for z in p['zones'] for g in z['generators'] if g['operator']==41][0]
        zones=d['instruments'][ii]['zones'];glob={g['operator']:g['amount'] for g in zones[0]['generators']} if not any(g['operator']==53 for g in zones[0]['generators']) else {}
        best=None
        for z in zones:
            g={**glob,**{x['operator']:x['amount'] for x in z['generators']}}
            if 53 not in g:continue
            lo,hi=(g[43]&255,g[43]>>8) if 43 in g else (0,127);dist=0 if lo<=60<=hi else min(abs(lo-60),abs(hi-60))
            if best is None or dist<best[0]:best=(dist,g)
        g=best[1];s=S[g[53]];root=g.get(58,s['root']);cents=s['cents']+s16(g.get(52,0))+100*s16(g.get(51,0));scale=g.get(56,100)
        x=pcm[s['start']:s['start']+s['frames']]*10**(-s16(g.get(48,0))/200) # initialAttenuation (centibels)
        lp=(s['loop_start'],s['loop_end']) if (g.get(54,0)&1 or header_loops) and s['loop_end']>s['loop_start']>0 else None # sampleModes 1/3 only: this SF2 sets none, and its header loops (Coin, Piranha, Shy Guy, Boo) jump by thousands at the wrap
        y,lp=resample(x,s['rate'],lp)
        rms=max(math.sqrt(float((y[a:a+2048]**2).mean())) for a in range(0,max(1,len(y)-2047),512));pk=float(np.abs(y).max());k=min(TARGET_RMS/max(rms,1),4.0,PEAK/max(pk,1))  # level-match by the loudest 2048-frame RMS, never past PEAK
        y=np.clip(np.round(y*k),-32768,32767).astype(int).tolist()
        insts.append(dict(pcm=y,root=root,scale=scale,cents=cents,loop=lp))
        rep.append((name,p['name'],g[53],root,cents,scale,len(y),lp,round(k,2),int(pk),int(rms)))
    return adpcm.pack(insts),rep
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('sf2',type=pathlib.Path);ap.add_argument('-o','--out',type=pathlib.Path,default=pathlib.Path(__file__).resolve().parents[1]/'assets/sounds/smp_bank.bin')
    ap.add_argument('--header-loops',action='store_true',help='loop the zones whose sample header has loop points (Coin, Piranha, Shy Guy, Boo; the SF2 does not enable them and they click)');a=ap.parse_args()
    b,rep=build(a.sf2,a.header_loops);a.out.write_bytes(b)
    for r in rep:print('%-8s %-14s smp %3d root %2d cents %4d scale %3d frames %6d loop %-14s gain x%.2f (raw peak %d, rms %d)'%r)
    print(len(b),'bytes ->',a.out)
if __name__=='__main__':main()
