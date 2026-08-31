"""Typed period-domain BC07 v2 differential; native cadence path plus separate ROM proof."""
import argparse,hashlib,json,re,struct,subprocess
from pathlib import Path
import verify_period_restart_v2 as capture_contract
ROOT=Path(__file__).resolve().parents[1]
ROM_SHA=capture_contract.ROM_SHA
CAPTURE_CONTRACT_SHA='68d22789ecaff106b9b2c773a821a5a3510c3a984dd5d8aeac6e61b03c6f2eca'
BUILD={'include/nba_period_roles.h','src/nba_period_roles.c','include/nba_period_roles_v2.h','src/nba_period_roles_v2.c','tools/period_roles_probe_v2.c','tools/period_roles_probe_fields_v2.inc','tools/build_period_roles_probe_v2.ps1'}
check=capture_contract.check
integer=capture_contract.integer
loads=capture_contract.loads
typed=capture_contract.typed
sha=capture_contract.sha
word=capture_contract.word

def mapping():
    fields=re.findall(r'^FIELD\(([^,]+),(0x[0-9a-f]+),([12])\)$',(ROOT/'tools/period_roles_probe_fields_v2.inc').read_text(),re.M)
    check(len(fields)==223,'v2 projection closure');return [(n,int(o,16),int(w))for n,o,w in fields]

def project(memory):return [memory[a]if w==1 else word(memory,a)for _,a,w in mapping()]

def check_build(exe):
    m=loads((exe.parent/'build-manifest.json').read_text(encoding='utf-8-sig'))
    check(set(m)=={'schema','compiler_exit','sources','executable'}and integer(m['schema'],1,1)and integer(m['compiler_exit'],0,0)and set(m['sources'])==BUILD,'role build contract')
    for name,value in m['sources'].items():check(set(value)=={'path','sha256'}and sha(value['path'])==value['sha256']==sha(ROOT/name),'role build source identity')
    e=m['executable'];check(set(e)=={'path','sha256'}and Path(e['path']).resolve()==exe.resolve()and sha(exe)==e['sha256'],'role executable identity')
    return m

def source(rom):
    raw=rom.read_bytes();check(sha(rom)==ROM_SHA,'original ROM')
    body=(ROOT/'src/nba_period_roles.c').read_text().split('direction_map[16]={',1)[1].split('};',1)[0]
    table=bytes(int(v)for v in body.split(','));check(len(table)==16 and table==raw[0x2f09a:0x2f0aa],'original F09Adirection map')
    check(raw[0x39c7b:0x39c8f]==struct.pack('<10H',*[0x34eb+256*i for i in range(10)]),'original879C7B actor pointers')
    check(raw[0x361e5:0x361f7]==bytes.fromhex('a9eb46859e2207bc85a96b47859e2207bc85'),'original paired caller')
    intervals={}
    for lo,hi in ((0x85b95c,0x85c0f5),(0x85f347,0x85f3ba),(0x86e1e5,0x86e1f6),(0x80cee7,0x80cefc)):
        off=((lo>>16)&127)*32768+(lo&32767);intervals[f'{lo:06x}-{hi:06x}']=hashlib.sha256(raw[off:off+hi-lo+1]).hexdigest()
    return intervals

def binary_input(words):return struct.pack('<'+('H'*(len(words)+2)),0x5252,2,*words)

def run_probe(exe,out,name,data):
    inp=out/(name+'.input');trace=out/(name+'.jsonl');inp.write_bytes(data)
    r=subprocess.run([str(exe.resolve()),str(inp)],capture_output=True,text=True)
    check(type(r.returncode)is int and r.returncode==0 and type(r.stdout)is str and type(r.stderr)is str and r.stderr=='','role probe status/stderr')
    trace.write_text(r.stdout);rows=[loads(line)for line in r.stdout.splitlines()]
    for row in rows:
        check(set(row)=={'kind','pc','completed_calls','record_pointer','words'}and integer(row['kind'],1,4)and integer(row['record_pointer'],0,65535)and integer(row['pc'],0,0xffffff)and integer(row['completed_calls'],0,2),'role C boundary schema')
        check(type(row['words'])is list and len(row['words'])==len(mapping())and all(integer(x,0,65535)for x in row['words']),'role C typed word schema')
    return rows

def verify_case(p,rom,exe,out):
    manifest,rows=capture_contract.read_native(p,rom)
    before=next(r for r in rows if r['tag']=='roles.before');after=next(r for r in rows if r['tag']=='roles.after')
    check(before['pc']==0x86e1e5 and after['pc']==0x86e1f7 and after['index']==before['index']+1,'paired native role boundaries')
    for row in (before,after):check(row['d']==0 and not(row['ps']&0x38),'role native M/X/decimal/direct-page domain')
    names=mapping();initial=project(before['memory'])
    # Only the before-state is sent to C. The expected-after state is never an
    # argument to binary_input or the probe and has no child-return adapter.
    got=run_probe(exe,out,p.name,binary_input(initial))
    typed([{k:r[k]for k in ('kind','pc','completed_calls','record_pointer')}for r in got],[{'kind':1,'pc':0x86e1ee,'completed_calls':1,'record_pointer':0},{'kind':2,'pc':0x86e1f7,'completed_calls':2,'record_pointer':0}])
    expected=project(after['memory'])
    differences=[(n,g,w)for (n,_,_),g,w in zip(names,got[-1]['words'],expected)if g!=w]
    check(not differences,'role parent fields: '+str(differences[:15]))
    return {'capture':str(p),'manifest_sha256':sha(p/'manifest.json'),'words_compared':len(names),'source_boundary':0x86e1f7,'scope':'native two early returns, extended typed fields preserved; extension tested separately by controlled ROM','initial_cadence':word(before['memory'],0x9d2),'final_cadence':word(after['memory'],0x9d2),'delta':word(before['memory'],0xc6),'rebuild':word(before['memory'],0x9d6),'camera':word(before['memory'],0x93a)}

def main(a):
    a.rom=a.rom.resolve();a.exe=a.exe.resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
    check(sha(capture_contract.__file__)==CAPTURE_CONTRACT_SHA,'accepted native capture contract identity')
    check_build(a.exe);intervals=source(a.rom)
    cases=[verify_case(p.resolve(),a.rom,a.exe,out)for p in a.native]
    report={'passed':True,'cases':cases,'source_intervals':intervals,'verifier_sha256':sha(__file__),'capture_contract_sha256':sha(capture_contract.__file__),'source_sha256':sha(ROOT/'src/nba_period_roles_v2.c'),'build_manifest_sha256':sha(a.exe.parent/'build-manifest.json')}
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print('PASS',len(cases),'paired captures');return report
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--native',type=Path,nargs='+',required=True);p.add_argument('--rom',type=Path,required=True);p.add_argument('--exe',type=Path,required=True);p.add_argument('--output',type=Path,required=True);main(p.parse_args())
