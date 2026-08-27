"""Native tip BAA2 core and D3C6 wrapper replay through the live C adapter.

Full represented core outputs; wrapper state/math/event writes and preserved
height. Native stack/DP restoration has no C ABI counterpart. Final B649 pose
attachment is not attributed to this proof. Low 8 fractional bits are outside
the port's 24.8 coordinate representation and the pure launch tests cover them.
"""
import argparse,json,subprocess
from pathlib import Path
from verify_ball_acquisition_vectors import memory,row,word

def main():
    p=argparse.ArgumentParser();p.add_argument('--vectors',nargs='+',required=True)
    p.add_argument('--probe',required=True);p.add_argument('--pack',required=True)
    p.add_argument('--normalized',action='store_true');p.add_argument('--export');a=p.parse_args()
    if a.normalized: vs=json.loads(Path(a.vectors[0]).read_text())['witnesses']
    else: vs=[v for f in a.vectors for s in Path(f).read_text().splitlines()
              if (v:=json.loads(s))['kind'] in ('catch_core','tip_bridge')]
    assert vs
    for kind in ('catch_core','tip_bridge'):
        selected=[v for v in vs if v['kind']==kind];assert selected
        if kind=='tip_bridge':
            pcs={x for v in selected for x in v['executed'] if 0x86d3c6<=x<=0x86d43d}
            assert len(pcs)==60,'wrapper fixture lost native branch coverage'
            assert {word(memory({'mem':v['entry']}),0xb2) for v in selected}=={0,5},'missing team-side witness'
        images=[];expected=[]
        for v in selected:
            # Reject missing required native regions instead of zero-filling
            # an uncaptured dependency and accidentally declaring parity.
            for snapshot in ('entry','exit'):
                covered=set()
                for base,data in v[snapshot].items():
                    covered.update(range(int(base,16),int(base,16)+len(data)//2))
                required=list(range(0,256))+list(range(0x700,0xa10))+list(range(0x1800,0x1870))
                required+=list(range(0x34eb,0x3fff))+list(range(0x46eb,0x486b))
                assert set(required)<=covered,'incomplete native capture'
            images.append(memory({'mem':v['entry']}));b=memory({'mem':v['exit']});want=row(b)
            if kind=='tip_bridge':
                want += [word(b,x) for x in (0x932,0x13e9,0x140f,0x148f,0x14a7,0x1477,0x14bf)]
                want += [b[0x1430],b[0x1448],word(b,0x3ef5)>>8,word(b,0x94a)]
                for i in range(10):want += [word(b,0x34eb+i*256+x) for x in (0x60,0x5a,0xc0,0x62)]
            expected.append(want)
        command=[a.probe,a.pack]+(['tip-wrapper'] if kind=='tip_bridge' else [])
        run=subprocess.run(command,input=b''.join(images),capture_output=True,check=True)
        actual=[[int(x,16) for x in s.split()] for s in run.stdout.decode().splitlines() if s and not s.startswith('[')]
        assert len(actual)==len(expected)
        bad=[]
        for n,(want,got) in enumerate(zip(expected,actual)):
            assert len(want)==len(got)
            # +60 is exposed separately from its legacy contact mirror for
            # wrapper launch; every other represented actor field is checked.
            excluded={39+i*13+5 for i in range(10)} if kind=='tip_bridge' else {0}
            differences=[(i,x,y) for i,(x,y) in enumerate(zip(want,got)) if x!=y and i not in excluded]
            if differences:bad.append((n,differences))
        print(kind,len(selected),'native calls;',len(bad),'mismatches')
        for x in bad[:6]:print(x)
        assert not bad
    if a.export:
        Path(a.export).write_text(json.dumps({'sources':a.vectors,'witnesses':vs},separators=(',',':'))+'\n')
if __name__=='__main__':main()
