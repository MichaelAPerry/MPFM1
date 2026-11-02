"""Compile bundled pixel stamps into RGB565 with explicit transparency; no smoothing."""
from pathlib import Path
from PIL import Image
ROOT=Path(__file__).resolve().parents[1]
NAMES='mario mushroom yoshi star flower gameboy dog cat pig swan face plane boat car heart treble'.split()
def main():
    rows=[]
    for name in NAMES:
        im=Image.open(ROOT/'assets/stamps'/f'{name}.png').convert('RGBA').resize((16,18),Image.Resampling.NEAREST)
        values=[]
        for r,g,b,a in (im.get_flattened_data() if hasattr(im,"get_flattened_data") else im.getdata()):
            value=1 if a<128 else ((r>>3)<<11)|((g>>2)<<5)|(b>>3)
            if value==1 and a>=128:value=0
            values.append(str(value))
        rows.append('{'+','.join(values)+'}')
    (ROOT/'assets/stamps.h').write_text('/* Generated from assets/stamps; see assets/NOTICE.md. */\nstatic const uint16_t stamp_pixels[16][288]={\n'+',\n'.join(rows)+'};\n')
    print('Stamp ROM: 9216 bytes; 15 character stamps and treble clef.')
if __name__=='__main__':main()
