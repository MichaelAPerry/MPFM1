import sys,pathlib,unittest,math,os,struct
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'tools'))
import adpcm
ROOT=pathlib.Path(__file__).resolve().parents[1]
SF2=pathlib.Path(os.environ.get('SMP_SF2','')) if os.environ.get('SMP_SF2') else None
class AdpcmTests(unittest.TestCase):
 sig=[round(9000*math.sin(i*.05+.3*math.sin(i*.003))*math.exp(-i/4000)+1500*math.sin(i*.31)*math.exp(-i/2500)) for i in range(3000)]
 def test_roundtrip_error(self):
  d,_=adpcm.encode(self.sig);y=adpcm.decode(d,0,len(self.sig))
  err=sum((a-b)**2 for a,b in zip(self.sig,y));pw=sum(a*a for a in self.sig);self.assertGreater(10*math.log10(pw/max(err,1)),30)
  self.assertEqual(len(d),(len(self.sig)+255)//256*132)
 def test_block_restart_and_loop_state(self):
  d,ls=adpcm.encode(self.sig,1000);y=adpcm.decode(d,0,3000)
  for b in range(1,12):self.assertEqual(adpcm.decode(d,0,3000,b*256),y[b*256:])
  self.assertEqual(adpcm.decode(d,0,3000,1000,ls),y[1000:])
 def test_pack(self):
  b=adpcm.pack([dict(pcm=self.sig,root=60),dict(pcm=self.sig,root=48,scale=120,cents=-10,loop=(500,2000))]);e,data,rate=adpcm.unpack(b)
  self.assertEqual((rate,len(e)),(22050,2));self.assertEqual((e[1]['frames'],e[1]['ls'],e[1]['le'],e[1]['flags'],e[1]['scale'],e[1]['cents']),(2000,500,2000,1,120,-10))
  y=adpcm.decode(data,e[1]['off'],2000);self.assertEqual(adpcm.decode(data,e[1]['off'],2000,500,(e[1]['lp'],e[1]['li'])),y[500:])
  self.assertEqual(adpcm.pack([dict(pcm=self.sig,root=60)]),adpcm.pack([dict(pcm=self.sig,root=60)]))
 def test_committed_pack(self):
  e,data,rate=adpcm.unpack((ROOT/'assets/sounds/smp_bank.bin').read_bytes());self.assertEqual((rate,len(e)),(22050,31));self.assertLess(len(data),260000)
  for t in e:self.assertTrue(t['frames']>=2 and 24<=t['root']<=96)
 @unittest.skipUnless(SF2 and SF2.exists(),'set SMP_SF2 to soundset3.sf2')
 def test_converter_deterministic(self):
  import make_smp_bank
  a,_=make_smp_bank.build(SF2);b,_=make_smp_bank.build(SF2);self.assertEqual(a,b);self.assertEqual(adpcm.unpack(a)[0][0]['root'],62)
if __name__=='__main__':unittest.main()
