"""Read RIFF SoundFont metadata without copying audio into the project."""
import json, pathlib, struct, sys

def inspect(path):
    data=path.read_bytes(); chunks={}
    def walk(start,end):
        while start+8<=end:
            tag=data[start:start+4].decode('ascii');size=struct.unpack_from('<I',data,start+4)[0];p=start+8
            if p+size>end: raise ValueError('truncated RIFF')
            if tag in ('RIFF','LIST'):walk(p+4,p+size)
            else:chunks[tag]=data[p:p+size]
            start=p+size+(size&1)
    walk(0,len(data));samples=[]
    for pos in range(0,len(chunks['shdr'])-46,46):
        name,start,end,lo,hi,rate,key,cents,link,kind=struct.unpack_from('<20sIIIIIBbHH',chunks['shdr'],pos)
        samples.append(dict(name=name.rstrip(b'\0').decode('latin1'),frames=end-start,pcm_bytes=(end-start)*2,rate=rate,seconds=(end-start)/rate,root=key,cents=cents,loop_start=lo-start,loop_end=hi-start,link=link,type=kind))
    presets=[]
    for pos in range(0,len(chunks['phdr'])-38,38):
        name,program,bank,bag,*_=struct.unpack_from('<20sHHHIII',chunks['phdr'],pos)
        presets.append(dict(name=name.rstrip(b'\0').decode('latin1'),program=program,bank=bank,bag=bag))
    # Preserve all zone generators, including key ranges (43), sample IDs (53),
    # loop modes (54), tuning (51/52), root overrides (58), envelope (33..38).
    def records(tag,fmt):
        b=chunks[tag]; sz=struct.calcsize(fmt)
        return [struct.unpack_from(fmt,b,p) for p in range(0,len(b),sz)]
    inst=records('inst','<20sH'); ibag=records('ibag','<HH'); igen=records('igen','<HH')
    pbag=records('pbag','<HH'); pgen=records('pgen','<HH'); ph=records('phdr','<20sHHHIII')
    def zones(begin,end,bags,gens):
        return [dict(generators=[dict(operator=op,amount=amount) for op,amount in gens[bags[z][0]:bags[z+1][0]]]) for z in range(begin,end)]
    instruments=[dict(name=r[0].rstrip(b'\0').decode('latin1'),zones=zones(r[1],inst[i+1][1],ibag,igen)) for i,r in enumerate(inst[:-1])]
    for i,preset in enumerate(presets):
        preset['zones']=zones(ph[i][3],ph[i+1][3],pbag,pgen)
        ids=set()
        for zone in preset['zones']:
            for g in zone['generators']:
                if g['operator']==41:
                    for iz in instruments[g['amount']]['zones']:
                        for ig in iz['generators']:
                            if ig['operator']==53:ids.add(ig['amount'])
        preset['sample_ids']=sorted(ids)
        preset['unique_pcm_bytes']=sum(samples[x]['pcm_bytes'] for x in ids)
    return dict(instruments=instruments,file_bytes=len(data),pcm_pool_bytes=len(chunks['smpl']),bit_depth=16,samples=samples,presets=presets,note='Sample headers only; zone generators can override root/loop/envelope. No redistribution permission inferred.')
if __name__=='__main__':print(json.dumps(inspect(pathlib.Path(sys.argv[1])),indent=2))
