"""MIT: deterministic original placeholder PCM, no Nintendo audio. Optional user WAV bank."""
import math, random, pathlib, wave, argparse, struct
N=4096
NAMES='piano kick zip bell trumpet pulse dog cat pig horn baby guitar block organ bass'.split()
def main():
    ap=argparse.ArgumentParser();ap.add_argument('--wav-dir',type=pathlib.Path);a=ap.parse_args()
    banks=[]; rng=random.Random(100)
    for role,name in enumerate(NAMES):
        if a.wav_dir:
            with wave.open(str(a.wav_dir/(name+'.wav')),'rb') as w:
                if (w.getnchannels(),w.getsampwidth(),w.getframerate())!=(1,2,22050): raise ValueError(name+': requires mono PCM16 22050Hz')
                if w.getnframes()>N: raise ValueError(name+': exceeds 4096 samples; trim explicitly')
                samples=list(struct.unpack('<'+'h'*w.getnframes(),w.readframes(w.getnframes())))
                if len(samples)<2: raise ValueError(name+': too short')
                samples += [0]*(N-len(samples))
        else:
            samples=[]; phase=0
            for i in range(N):
                t=i/22050; freq=261.625565
                if role==1: freq=55+160*math.exp(-t*35)
                elif role in (2,6,7,8,10): freq*=1+0.35*math.sin(t*(25+role*4))*math.exp(-t*9)
                phase+=2*math.pi*freq/22050
                s=math.sin(phase)
                if role in (0,3,11,12,14): s+=0.4*math.sin(phase*(2.01 if role==3 else 2))+0.15*math.sin(phase*4)
                elif role in (4,9,13): s+=0.4*math.sin(phase*3)+0.2*math.sin(phase*5)
                elif role==5: s=sum(math.sin(phase*k)/k for k in (1,3,5,7))
                elif role in (6,8,10): s=0.7*s+0.3*(rng.random()*2-1)
                env=min(1,i/64)*math.exp(-t*(8+role%5*3))*min(1,(N-1-i)/256)
                samples.append(round(max(-1,min(1,s*env/1.6))*28000))
        banks.append(samples)
    out=pathlib.Path(__file__).resolve().parents[1]/'dsp/bank.h'
    text='/* Generated original placeholders (MIT), or explicit user WAV bank. */\n#define SC_SAMPLE_LEN 4096\nstatic const int16_t sc_bank[15][SC_SAMPLE_LEN]={\n'
    text+=',\n'.join('{'+','.join(map(str,s))+'}' for s in banks)+'};\n'
    text+='static const uint32_t sc_increments[49]={'+','.join(str(round(32768*2**((p-60)/12))) for p in range(36,85))+'};\n'
    out.write_text(text)
    print('PCM: 122880 bytes; pitch table: 196 bytes; 15 x 4096 mono PCM16 @ 22050 Hz, root C4; no loops')
if __name__=='__main__': main()
