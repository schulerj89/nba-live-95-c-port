"""Fresh standalone appearance/support comparison against original captures."""
import argparse,hashlib,json,struct,subprocess
from pathlib import Path
from verify_period_restart_v2 import read_native,loads
from period_support_source_domain import validate_source_domain
ROOT=Path(__file__).resolve().parents[1]
def sha(path):
    with Path(path).open('rb')as f:return hashlib.file_digest(f,'sha256').hexdigest()
def word(raw,address):return struct.unpack_from('<H',raw,address)[0]
def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in('probes','captures','pack','rom','output'):p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args()
    for name in('probes','captures','pack','rom','output'):setattr(a,name,getattr(a,name).resolve())
    a.output.mkdir(parents=True,exist_ok=False)
    m=loads((a.probes/'build-manifest.json').read_text(encoding='utf-8-sig'))
    assert type(m['compiler_exit'])is int and m['compiler_exit']==0
    assert set(m['executables'])=={'period_appearance','period_support'}
    for name,v in m['sources'].items():assert sha(v['path'])==v['sha256']==sha(ROOT/name)
    for name,v in m['executables'].items():assert Path(v['path'])==a.probes/(name+'_probe.exe')and sha(v['path'])==v['sha256']
    assert sha(a.pack)=='951f82331c4bb6ce8f381da519ee8bfdf517bf8c13f2cd6f20cfa9c34d5ed4df'
    assert sha(a.rom)=='2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
    diagnostic=f"[ASSETS] Loaded asset pack: '{a.pack}' (89438786 bytes, 263 assets)\n".encode()
    cases=[];appearance_words=0;support_bytes=0;rom=a.rom.read_bytes()
    def run(command):
        r=subprocess.run(command,cwd=ROOT,capture_output=True)
        assert type(r.returncode)is int and r.returncode==0
        assert r.stderr.replace(b'\r\n',b'\n')==diagnostic
        return r
    for period in range(4):
        d=a.captures/f'period-{period}-ready1-children-v{3 if period==3 else 2}'
        manifest,rows=read_native(d,a.rom)
        for before,after in zip(rows,rows[1:]):
            if before['tag']not in('appearance.first.before','appearance.second.before'):continue
            actor=(word(before['memory'],0x96)-0x34eb)//256
            command=[str(a.probes/'period_appearance_probe.exe'),str(a.pack),str(d/before['raw']),str(actor)]
            result=run(command);data=loads(result.stdout.decode())
            assert type(data)is dict and set(data)=={'rng','owner_pointer','actor'}
            assert type(data['actor'])is list and len(data['actor'])==128
            assert all(type(v)is int and 0<=v<=65535 for v in[data['rng'],data['owner_pointer'],*data['actor']])
            raw=after['memory'];expected=dict(rng=word(raw,0x7f6),owner_pointer=word(raw,0x940),actor=list(struct.unpack_from('<128H',raw,0x34eb+actor*256)))
            assert data==expected
            (a.output/f'appearance-{period}-{actor}.stdout').write_bytes(result.stdout)
            appearance_words+=130;cases.append(dict(mode='appearance',period=period,actor=actor,command=command))
        for mode,start,end in [('assignment','assignment.before','assignment.after'),('sort','assignment.after','cancel.before'),('attachment','possession.after','inbound.after')]:
            if period==3 and mode=='attachment':continue
            before=next(r for r in rows if r['tag']==start);after=next(r for r in rows if r['tag']==end)
            validate_source_domain(before['memory'],mode,rom)
            target=a.output/f'{mode}-{period}.bin'
            command=[str(a.probes/'period_support_probe.exe'),str(a.pack),str(d/before['raw']),mode,str(target)]
            result=run(command);assert result.stdout==b'';actual=target.read_bytes();assert len(actual)==131072
            regions=[(0x34d3,24),(0x34eb,2816)]
            if mode=='assignment':regions.extend([(0x3435,60),(0x9da,20),(0x4734,5),(0x47b4,5)])
            if mode=='attachment':regions.extend([(0x900,0x10a),(0x46eb,0x240)])
            for start,size in regions:assert actual[start:start+size]==after['memory'][start:start+size]
            support_bytes+=sum(size for start,size in regions);cases.append(dict(mode=mode,period=period,command=command))
    assert len(cases)==51 and appearance_words==5200 and support_bytes==34126
    report=dict(passed=True,cases=cases,appearance_words=appearance_words,support_bytes=support_bytes,build_manifest_sha256=sha(a.probes/'build-manifest.json'),scope='Fresh standalone CPU-period data projections only; no CPU/DP/phase/human or production integration acceptance')
    (a.output/'report.json').write_text(json.dumps(report,indent=2));print('PASS 40 appearance calls / 5200 words; 11 support calls / 34126 bytes')
if __name__=='__main__':main()
