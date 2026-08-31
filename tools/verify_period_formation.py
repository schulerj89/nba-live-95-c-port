"""Single typed DD97 input, real child composition, native checkpoint projection."""
import argparse,hashlib,json,re,subprocess
from pathlib import Path
import verify_period_restart_v2 as native_contract
ROOT=Path(__file__).resolve().parents[1]
ROM_SHA=native_contract.ROM_SHA
PACK_SHA='951f82331c4bb6ce8f381da519ee8bfdf517bf8c13f2cd6f20cfa9c34d5ed4df'
DEPENDENCY_MANIFEST_SHA='5d8867bdecd62bfb50d2dd0495fa9c5c7e007f3adbac043ac1e30a0263eab609'
NATIVE_CONTRACT_SHA='68d22789ecaff106b9b2c773a821a5a3510c3a984dd5d8aeac6e61b03c6f2eca'
check=native_contract.check;loads=native_contract.loads;integer=native_contract.integer;sha=native_contract.sha;typed=native_contract.typed

def mapping():
 rows=re.findall(r'^FIELD\(([^,]+),(0x[0-9a-f]+),([124])\)$',(ROOT/'tools/period_formation_fields.inc').read_text(),re.M)
 check(len(rows)==1029,'canonical field closure');result=[(n,int(a,16),int(w))for n,a,w in rows];seen=set()
 for n,a,w in result:
  check(not seen.intersection(range(a,a+w)),'duplicate canonical ownership '+n);seen.update(range(a,a+w))
 check(sum(w for _,_,w in result)==2106,'typed input extent');return result

def project(raw):return [int.from_bytes(raw[a:a+w],'little')for _,a,w in mapping()]
def binary(values):return b'PFC1'+b''.join(v.to_bytes(w,'little')for v,(_,_,w)in zip(values,mapping()))

def check_build(exe):
 m=loads((exe.parent/'build-manifest.json').read_text(encoding='utf-8-sig'))
 check(set(m)=={'schema','compiler_exit','sources','executable'}and integer(m['schema'],1,1)and integer(m['compiler_exit'],0,0),'build envelope')
 deps=ROOT/'.analysis/period-formation-dependencies-v1';check(sha(deps/'manifest.json')==DEPENDENCY_MANIFEST_SHA,'pinned dependency manifest identity');dependency=loads((deps/'manifest.json').read_text())
 expected={ROOT/n for n in ['tools/period_formation_probe.c','src/nba_period_formation.c','src/nba_period_restart_v2.c','src/nba_period_roles.c','src/nba_period_roles_v2.c','include/nba_period_formation.h','include/nba_period_restart_v2.h','include/nba_period_roles.h','include/nba_period_roles_v2.h','tools/period_formation_fields.inc','tools/build_period_formation_probe.ps1']}|{deps/'manifest.json'}
 check(set(dependency)=={'commit','render_freeze_sha256','files'}and dependency['commit']=='979c042'and len(dependency['files'])==30,'dependency closure')
 for n,item in dependency['files'].items():
  check(set(item)=={'path','snapshot','sha256'},'dependency identity envelope');p=deps/Path(item['snapshot']).name;check(p.name==Path(n).name and sha(p)==item['sha256'],'private dependency identity');expected.add(p)
 check(set(m['sources'])=={str(p.resolve())for p in expected},'exact build source/header closure')
 for p,item in m['sources'].items():check(set(item)=={'sha256','bytes'}and integer(item['bytes'],0,2**31-1)and Path(p).stat().st_size==item['bytes']and sha(p)==item['sha256'],'build source bytes')
 e=m['executable'];check(set(e)=={'path','sha256'}and Path(e['path']).resolve()==exe.resolve()and sha(exe)==e['sha256'],'fresh executable identity');return m

def expected_trace(period,tip):
 rows=[]
 for i in range(5):rows.extend([(0x86dfcb,i),(0x86dfcf,i),(0x86dfd8,i+5),(0x86dfdc,i+5)])
 rows.extend((pc,65535)for pc in (0x86e056,0x86e0ac,0x86e0b0,0x86e0b4,0x86e0b8))
 if 0<period<4:
  actor=(tip^(0 if period==3 else 5))+2;rows.extend([(0x86e102,actor),(0x86e106,65535),(0x86e183,actor),(0x86e1a4,actor)])
 else:rows.append((0x86e1ac,65535))
 rows.extend((pc,65535)for pc in (0x86e1e5,0x86e1f7,0x86e207))
 return [dict(kind=2 if pc==0x86e207 else 1,pc=pc,actor=actor,refusal=0,role_kind=0,role_pc=0,role_pointer=0,role_calls=0)for pc,actor in rows]

def parse(result,pack):
 check(type(result.returncode)is int and result.returncode==0 and type(result.stdout)is str and type(result.stderr)is str,'probe status/stream types')
 diagnostic=f"[ASSETS] Loaded asset pack: '{pack}' (89438786 bytes, 263 assets)\n"
 check(result.stderr==diagnostic,'asset loader diagnostic')
 rows=[loads(line)for line in result.stdout.splitlines()];names=mapping()
 for row in rows:
  check(type(row)is dict and set(row)=={'kind','pc','actor','refusal','role_kind','role_pc','role_pointer','role_calls','values'},'C boundary schema')
  for k,lo,hi in [('kind',1,4),('pc',0,0xffffff),('actor',0,65535),('refusal',0,8),('role_kind',0,4),('role_pc',0,0xffffff),('role_pointer',0,65535),('role_calls',0,2)]:check(integer(row[k],lo,hi),'C metadata '+k)
  check(type(row['values'])is list and len(row['values'])==len(names)and all(integer(v,0,2**(8*w)-1)for v,(_,_,w)in zip(row['values'],names)),'C canonical field domains')
 check(bool(rows),'missing all C boundaries');return rows

def run(exe,pack,out,name,values):
 inp=out/(name+'.input');trace=out/(name+'.jsonl');inp.write_bytes(binary(values))
 result=subprocess.run([str(exe.resolve()),str(pack.resolve()),str(inp.resolve())],capture_output=True,text=True)
 trace.write_text(result.stdout);(out/(name+'.stderr')).write_text(result.stderr)
 return parse(result,pack.resolve())

def case(native,rom,exe,pack,out):
 manifest,rows=native_contract.read_native(native,rom);before=next(r for r in rows if r['tag']=='formation.table');raw=before['memory']
 check(before['pc']==0x86dd97 and before['d']==0 and not(before['ps']&0x38),'source DD97 M/X/D/DP domain')
 values=project(raw);trace=run(exe,pack,out,native.name,values);expected=expected_trace(int.from_bytes(raw[0x926:0x928],'little'),int.from_bytes(raw[0x932:0x934],'little'))
 typed([{k:v for k,v in r.items()if k!='values'}for r in trace],expected)
 comparisons=0;cursor=0;names=mapping()
 for r in trace:
  at=next(i for i in range(cursor,len(rows))if rows[i]['pc']==r['pc']);cursor=at+1;want=project(rows[at]['memory'])
  # DP9A is parent cursor/private role pair scratch; child APIs explicitly
  # exclude CPU residue. Never count it as native gameplay parity here.
  differences=[(n,g,e)for(n,_,_),g,e in zip(names,r['values'],want)if n!='parent.list_cursor'and g!=e]
  check(not differences,hex(r['pc'])+' '+str(differences[:20]));comparisons+=len(names)-1
 return dict(capture=str(native),entry_sha256=hashlib.sha256(raw).hexdigest(),manifest_sha256=sha(native/'manifest.json'),boundaries=len(trace),typed_comparisons=comparisons)

def main(a):
 for k,v in vars(a).items():
  if isinstance(v,Path):setattr(a,k,v.resolve())
 a.output.mkdir(parents=True,exist_ok=False);check(sha(a.rom)==ROM_SHA and sha(a.pack)==PACK_SHA,'ROM/pack identity');check(sha(native_contract.__file__)==NATIVE_CONTRACT_SHA,'native source contract identity');check_build(a.exe)
 cases=[case(p.resolve(),a.rom,a.exe,a.pack,a.output)for p in a.native]
 report=dict(passed=True,cases=cases,boundaries=sum(c['boundaries']for c in cases),typed_comparisons=sum(c['typed_comparisons']for c in cases),scope='single typed DD97 input; real child composition; no native after inputs or CPU/DP/timing claim',verifier_sha256=sha(__file__))
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print('PASS',report['boundaries'],'boundaries;',report['typed_comparisons'],'typed fields');return report
if __name__=='__main__':
 p=argparse.ArgumentParser()
 for n in ('rom','pack','exe','output'):p.add_argument('--'+n,type=Path,required=True)
 p.add_argument('--native',type=Path,nargs='+',required=True);main(p.parse_args())
