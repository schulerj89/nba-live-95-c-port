"""Bounded collision/draw sort and counter data; never CPU/video timing."""
import argparse,copy,hashlib,json,struct,subprocess
from pathlib import Path
from verify_period_restart_v2 import read_native,loads
ROOT=Path(__file__).resolve().parents[1]
def sha(path):
    with Path(path).open('rb')as f:return hashlib.file_digest(f,'sha256').hexdigest()
def process(result,raw):
    if type(result.returncode)is not int or result.returncode!=0:raise ValueError('process status')
    if result.stdout!=b''or result.stderr!=b'':raise ValueError('unexpected process text')
    if type(raw)is not bytes or len(raw)!=131072:raise ValueError('raw output type/length')
def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in('probes','captures','rom','output'):p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args()
    for name in vars(a):setattr(a,name,getattr(a,name).resolve())
    a.output.mkdir(parents=True,exist_ok=False)
    m=loads((a.probes/'render-build-manifest.json').read_text(encoding='utf-8-sig'))
    assert type(m['compiler_exit'])is int and m['compiler_exit']==0
    assert sha(a.probes/'build-manifest.json')==m['base_manifest_sha256']
    base=loads((a.probes/'build-manifest.json').read_text(encoding='utf-8-sig'))
    for name,v in {**base['sources'],**m['sources']}.items():assert sha(v['path'])==v['sha256']==sha(ROOT/name)
    exe=a.probes/'period_render_tail_probe.exe';assert Path(m['executable']['path'])==exe and sha(exe)==m['executable']['sha256']
    cases=[];first=None
    for period in range(4):
        d=a.captures/f'period-{period}-ready1-children-v{3 if period==3 else 2}'
        manifest,rows=read_native(d,a.rom)
        assert sum(r['tag']=='roles.after'for r in rows)==sum(r['tag']=='formation.return'for r in rows)==1
        before,after=rows[-2:];assert before['tag']=='roles.after'and after['tag']=='formation.return'
        assert after['index']==before['index']+1 and before['sp']==after['sp']
        command=[str(exe),str(d/before['raw']),str(a.output/f'{period}.bin')]
        r=subprocess.run(command,cwd=ROOT,capture_output=True);raw=Path(command[2]).read_bytes();process(r,raw)
        for start,length in[(0x34d3,24),(0x34eb,3072),(0x7e44,24),(0x84a,4)]:assert raw[start:start+length]==after['memory'][start:start+length]
        cases.append(dict(period=period,command=command,observed_clocks=[before['frame'],before['court'],after['frame'],after['court']]))
        if first is None:first=(r,raw,command)
    r,raw,command=first;checks=[]
    def reject(name,fn):
        try:fn()
        except(ValueError,TypeError):checks.append(name)
        else:raise AssertionError('accepted '+name)
    for name,key,value in [('bool exit','returncode',False),('float exit','returncode',0.0),('error exit','returncode',1),('stdout','stdout',b'x'),('stderr','stderr',b'x')]:
        bad=copy.copy(r);setattr(bad,key,value);reject(name,lambda:process(bad,raw))
    for name,value in [('short raw',raw[:-1]),('long raw',raw+b'x'),('wrong raw type',list(raw))]:reject(name,lambda:process(r,value))
    entry=Path(command[1]).read_bytes();guards=[]
    for address,value in[(0x34d1,1),(0x34d1,0x4000),(0x34d1,0xffff),(0x7e44,0),(0x7e44,0x34ec),(0x7e44,0x40eb),(0x7e44,struct.unpack_from('<H',entry,0x7e46)[0]),(0x34d3,0),(0x34e9,1)]:
        bad=bytearray(entry);struct.pack_into('<H',bad,address,value);inp=a.output/f'guard{len(guards)}.input';out=a.output/f'guard{len(guards)}.output';inp.write_bytes(bad)
        r=subprocess.run([str(exe),str(inp),str(out)],cwd=ROOT,capture_output=True)
        assert type(r.returncode)is int and r.returncode==3 and r.stdout==r.stderr==b''and not out.exists();guards.append(dict(address=address,value=value))
    report=dict(passed=True,cases=cases,compared_bytes=12496,protocol_rejections=checks,domain_refusals=guards,scope='Owned data only. Native frame crossings recorded, not reproduced. No CPU/DP/interrupt/video/DMA or production acceptance.')
    (a.output/'report.json').write_text(json.dumps(report,indent=2));print('PASS 12496 native bytes; 8 protocol and 9 source-domain refusals')
if __name__=='__main__':main()
