import sys,pathlib,unittest,struct,zlib
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'tools'))
from import_smp import convert
HEADER='TEMPO: 120.000000, EXT: 0, TIME: 4/4, SOUNDSET: soundset3.sf2\n'
class ImportTests(unittest.TestCase):
 def test_chords_accidentals_and_retrigger(self):
  b=convert(HEADER+'1:0,MARIO C4#,MARIO E4b,VOL: 87\n1:2,MARIO G4,VOL: 100',True)
  self.assertEqual(struct.unpack_from('<HHBBBB',b,48),(0,192,61,0,87,0));self.assertEqual(b[60],63)
  self.assertEqual(struct.unpack_from('<I',b,len(b)-4)[0],zlib.crc32(b[:-4]))
 def test_refusals(self):
  for text in [HEADER.replace('EXT: 0','EXT: 1'),HEADER.replace('120.000000','120.5'),HEADER.replace('4/4','6/8'),HEADER+'1:0,COIN C4,VOL: 100',HEADER+'1:0,MARIO C41,VOL: 100',HEADER+'1:0,MARIO C6#,VOL: 100',HEADER+'1:0,MARIO C4,VOL: 128',HEADER+'1:4,MARIO C4,VOL: 100',HEADER+'1:0,MARIO C4,VOL: 100\n1:0,DOG C4,VOL: 100']:
   with self.subTest(text=text),self.assertRaises(ValueError):convert(text,True)
 def test_acknowledgement(self):
  with self.assertRaises(ValueError):convert(HEADER)
 def test_capacity(self):
  with self.assertRaises(ValueError):convert(HEADER+'1:0,'+'MARIO C4,'*385+'VOL: 100',True)
if __name__=='__main__':unittest.main()
