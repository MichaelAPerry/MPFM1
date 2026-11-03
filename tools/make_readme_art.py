"""Compose the README banner from actual host screenshots and source stamps."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
root=Path(__file__).resolve().parents[1]
out=root/'docs/screens';out.mkdir(parents=True,exist_ok=True)
for name in ('welcome','compose','playback','mixer','songbook','chord','playing','editor'):
 im=Image.open(root/f'build/host/{name}.png').convert('RGB')
 im.save(out/f'{name}.png',optimize=True)
font=root/'base/assets/fonts'
def f(size,bold=False):
 return ImageFont.truetype(str(font/('BarlowSemiCondensed-Bold.ttf' if bold else 'BarlowSemiCondensed-Medium.ttf')),size)
im=Image.new('RGB',(1200,600),'#f4efdf');d=ImageDraw.Draw(im)
d.rectangle((0,0,1200,8),fill='#cc3934')
d.text((48,38),'STAMP COMPOSER  /  FM-1',font=f(22,True),fill='#197b7b')
d.text((45,74),'Little stamps. Big ideas.',font=f(59,True),fill='#27303b')
d.text((49,148),'A pocket music playground. Pick a character. Place a note. Make a song.',font=f(23),fill='#686d70')
for x,name,label in [(48,'compose','01  COMPOSE'),(424,'mixer','02  MIX & SHAPE'),(800,'songbook','03  SONG BOOK')]:
 d.rounded_rectangle((x,205,x+350,548),radius=14,fill='#27303b')
 shot=Image.open(root/f'build/host/{name}.png').convert('RGB').resize((288,288),Image.Resampling.NEAREST)
 im.paste(shot,(x+31,224));d.text((x+31,520),label,font=f(16,True),fill='#f4efdf')
d.text((49,566),'15 STAMPS  /  8 LAYERS  /  NATIVE C + WEBASSEMBLY',font=f(17,True),fill='#197b7b')
im.save(root/'docs/hero.png',optimize=True)
