"""MIT: IMA ADPCM 4-bit encoder/decoder and the MPFM1 sound-pack (SMPB) writer/reader shared by make_smp_bank.py and make_bank.py.
Block = 4-byte header (s16 predictor, u8 step index, u8 0 = the decoder state BEFORE the block's first sample) + 128 bytes (256 samples, low nibble first).
Pack = 'SMPB', u16 version 1, u16 count, u32 rate, u32 data bytes; count x 24-byte entries
(u32 offset, u32 frames, u32 loop_start, u32 loop_end, s16 loop_pred, u8 loop_idx, u8 flags (1 loop), u8 root key, u8 cents per key, s16 cents); then the blocks.
A looped instrument stores frames = loop_end and the decoder state at loop_start, so wrapping is exact."""
import struct
STEP=[7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,80,88,97,107,118,130,143,157,173,190,209,230,253,279,307,337,371,408,449,494,544,598,658,724,796,876,963,1060,1166,1282,1411,1552,1707,1878,2066,2272,2499,2749,3024,3327,3660,4026,4428,4871,5358,5894,6484,7132,7845,8630,9493,10442,11487,12635,13899,15289,16818,18500,20350,22385,24623,27086,29794,32767]
IDX=[-1,-1,-1,-1,2,4,6,8]
BLOCK=256;BB=4+BLOCK//2
def step(p,i,n):
    s=STEP[i];d=s>>3
    if n&4:d+=s
    if n&2:d+=s>>1
    if n&1:d+=s>>2
    if n&8:d=-d
    return max(-32768,min(32767,p+d)),max(0,min(88,i+IDX[n&7]))
def encode(pcm,loop_start=None):
    """-> (bytes, (pred,idx) before sample loop_start)"""
    p=pcm[0] if len(pcm)>1 else 0;i=next((k for k in range(89) if STEP[k]*2>=abs(pcm[1]-pcm[0])),88) if len(pcm)>1 else 0;out=bytearray();ls=None;n=len(pcm);pcm=list(pcm)+[0]*(-n%BLOCK)
    for b in range(0,len(pcm),BLOCK):
        out+=struct.pack('<hBB',p,i,0);nib=[]
        for k in range(b,b+BLOCK):
            if k==loop_start:ls=(p,i)
            d=pcm[k]-p;c=0
            if d<0:c=8;d=-d
            s=STEP[i]
            if d>=s:c|=4;d-=s
            if d>=s>>1:c|=2;d-=s>>1
            if d>=s>>2:c|=1
            p,i=step(p,i,c);nib.append(c)
        out+=bytes(nib[j]|nib[j+1]<<4 for j in range(0,BLOCK,2))
    return bytes(out),ls
def decode(data,off,frames,start=0,state=None):
    """decode frames [start,frames) of the stream at off; state=(pred,idx) before `start` (default: the block header, start must be block-aligned)"""
    if state is None:
        assert start%BLOCK==0;p,i,_=struct.unpack_from('<hBB',data,off+start//BLOCK*BB);state=(p,i)
    p,i=state;out=[]
    for k in range(start,frames):
        c=data[off+k//BLOCK*BB+4+k%BLOCK//2];c=c>>4 if k&1 else c&15
        p,i=step(p,i,c);out.append(p)
    return out
def pack(insts,rate=22050):
    """insts: dicts pcm (int list), root, scale, cents, loop=(start,end) or None -> bytes"""
    tab=b'';blob=b''
    for t in insts:
        lp=t.get('loop');pcm=t['pcm'][:lp[1]] if lp else t['pcm']
        d,ls=encode(pcm,lp[0] if lp else None)
        tab+=struct.pack('<IIIIhBBBBh',len(blob),len(pcm),lp[0] if lp else 0,lp[1] if lp else 0,ls[0] if lp else 0,ls[1] if lp else 0,1 if lp else 0,t['root'],t.get('scale',100),t.get('cents',0));blob+=d
    return struct.pack('<4sHHII',b'SMPB',1,len(insts),rate,len(blob))+tab+blob
def unpack(b):
    m,v,n,rate,nb=struct.unpack_from('<4sHHII',b);assert m==b'SMPB' and v==1 and len(b)==16+24*n+nb
    ents=[dict(zip('off frames ls le lp li flags root scale cents'.split(),struct.unpack_from('<IIIIhBBBBh',b,16+24*k))) for k in range(n)]
    return ents,b[16+24*n:],rate
