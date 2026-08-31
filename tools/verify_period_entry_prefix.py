"""Complete WRAM projection from one DCA6 prestate; no native child/after inputs."""
import argparse,hashlib,importlib.util,json,re,struct,subprocess,sys
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[1]
OWNER=Path(r'C:\Users\joshs\Projects\nba-live-95-c-port\.analysis\worktrees\completion-owner')
CONTRACT=OWNER/'tools/verify_period_restart_v2.py'
CONTRACT_SHA='68d22789ecaff106b9b2c773a821a5a3510c3a984dd5d8aeac6e61b03c6f2eca'
FREEZE=OWNER/'build/period-restart-native-freeze-v1.json'
FREEZE_SHA='04e4c13a1b7298b97fd72fac004e73f58cf6f2eb5bcddf0eaf389eeb404f3d2b'
ROM_SHA='2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
BUILD={'include/nba_types.h','include/nba_period_entry_prefix.h','src/nba_period_entry_prefix.c','tools/period_entry_prefix_probe.c','tools/period_entry_prefix_fields.inc','tools/build_period_entry_prefix_probe.ps1'}
def need(ok,message):
 if not ok:raise ValueError(message)
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def unique(pairs):
 out={}
 for k,v in pairs:need(k not in out,'duplicate JSON key');out[k]=v
 return out
def loads(s):return json.loads(s,object_pairs_hook=unique)
def read(p):return loads(Path(p).read_text(encoding='utf-8-sig'))
def integer(x,lo,hi):return type(x)is int and lo<=x<=hi
def word(raw,a):return struct.unpack_from('<H',raw,a)[0]
def mapping():
 fields=re.findall(r'^(ACTOR|GLOBAL)\((\w+),(0x[0-9a-f]+)\)$',(ROOT/'tools/period_entry_prefix_fields.inc').read_text(),re.M)
 need(len(fields)==51,'field schema')
 result=[]
 for kind,name,off in fields:
  if kind=='ACTOR':result.extend((f'actor{i}.{name}',0x34eb+i*256+int(off,16))for i in range(10))
  else:result.append((name,int(off,16)))
 need(len(result)==159,'typed field count');return result
def build(exe):
 m=read(exe.parent/'build-manifest.json')
 need(set(m)=={'schema','compiler_exit','sources','executable'}and integer(m['schema'],1,1)and integer(m['compiler_exit'],0,0),'build schema')
 need(set(m['sources'])==BUILD,'build sources')
 for name,digest in m['sources'].items():need(type(digest)is str and sha(ROOT/name)==digest,'build source identity')
 e=m['executable'];need(set(e)=={'path','sha256'}and Path(e['path']).resolve()==exe and sha(exe)==e['sha256'],'build executable')
 return m
def contract():
 need(sha(CONTRACT)==CONTRACT_SHA and sha(FREEZE)==FREEZE_SHA,'accepted native contract/freeze changed')
 spec=importlib.util.spec_from_file_location('accepted_period_contract',CONTRACT);v=importlib.util.module_from_spec(spec);spec.loader.exec_module(v)
 return v
def attest(capture,rom):
 v=contract();m,rows=v.read_native(capture,rom)
 frozen=read(FREEZE)['files']
 for p in [capture/'manifest.json',capture/'boundaries.jsonl',*[capture/r['raw']for r in rows]]:
  key=p.relative_to(OWNER).as_posix();record=frozen[key]
  need(set(record)=={'path','bytes','sha256'}and type(record['bytes'])is int and Path(record['path']).resolve()==p and p.stat().st_size==record['bytes']and sha(p)==record['sha256'],'frozen native identity')
 need(rows[0]['frame']==4390 and rows[0]['court']==0,'court clock anchor')
 for r in rows:need(r['frame']-r['court']==4390,'relative clock')
 prefix=[next(r for r in rows if r['tag']==tag)for tag in('formation.entry','clock.select','clock.ready','formation.table')]
 first=prefix[0];period=word(first['memory'],0x926);quarter=word(first['memory'],0x17b1)
 need(period==m['period_seed']+1 and quarter==3,'captured period/quarter domain')
 for r in prefix:
  need(r['d']==0 and r['ps']&0x38==0,'binary16/DP0 prefix domain')
  need(r['frame']==first['frame']and r['court']==first['court']and r['sp']==first['sp']==0x1ff9,'prefix clock/stack continuity')
  need(r['memory'][0x1ffa:0x1ffd]==bytes.fromhex('a98c87'),'original8CA6 return frame')
  for a in(0x926,0x17b1,0x9ba,0x9b0,0x9b2):need(word(r['memory'],a)==word(first['memory'],a),'carried prefix input')
 # CPU status is a separate source contract. Data routines do not fake CPU time.
 irq=first['ps']&4
 expected=[(0x3eeb,0x3eeb,0,irq|2),
  (word(prefix[2]['memory'],0x928),quarter*2 if period>=4 else 0x3eeb,0,irq|(0x80 if word(prefix[2]['memory'],0x928)&0x8000 else 2 if word(prefix[2]['memory'],0x928)==0 else 0)),
  (0x34d3,0,0x34eb,irq|2|(1 if period>=2 else 0))]
 for r,registers in zip(prefix[1:],expected):need(tuple(r[k]for k in('a','x','y','ps'))==registers,'source boundary registers/status')
 return m,prefix
def probe(exe,rom,before,compact=False):
 args=[str(exe),str(rom),str(before)]+(['--typed']if compact else [])
 run=subprocess.run(args,capture_output=True,text=True,timeout=60)
 need(type(run.returncode)is int and run.returncode==0 and type(run.stdout)is str and type(run.stderr)is str and run.stderr=='','probe process protocol')
 lines=run.stdout.splitlines();need(len(lines)==(1 if compact else 3),'probe row count')
 rows=[loads(s)for s in lines]
 for i,r in enumerate(rows):
  need(type(r)is dict and set(r)=={'pc','result','words'},'probe fields')
  need(integer(r['pc'],0,0xffffff)and r['pc']==(0x86dd97 if compact else (0x86dd2d,0x86dd47,0x86dd97)[i]),'probe boundary')
  need(integer(r['result'],0,1),'probe result')
  need(type(r['words'])is list and len(r['words'])==(159 if compact else 65536)and all(integer(w,0,65535)for w in r['words']),'probe word type/length')
 return rows,run
def verify(capture,rom,exe,out):
 need(sha(rom)==ROM_SHA,'original ROM identity');build(exe);m,native=attest(capture,rom)
 rows,run=probe(exe,rom,capture/native[0]['raw'])
 if out is not None:(out/'probe-stdout.txt').write_text(run.stdout);(out/'probe-stderr.txt').write_text(run.stderr)
 for got,want in zip(rows,native[1:]):
  need(got['result']==1,'prefix rejected native domain')
  wanted=list(struct.unpack('<65536H',want['memory']))
  if got['words']!=wanted:
   differences=[dict(address=2*i,actual=a,expected=b)for i,(a,b)in enumerate(zip(got['words'],wanted))if a!=b]
   raise ValueError('WRAM mismatch '+json.dumps(differences[:20]))
 return dict(passed=True,manifest_sha256=sha(capture/'manifest.json'),entry=native[0]['index'],exit=native[-1]['index'],period=m['period_seed']+1,compared_words=196608,boundaries=3,frame=native[0]['frame'],court=native[0]['court'],before_only=True)
def main():
 p=argparse.ArgumentParser()
 for n in('rom','probe','output'):p.add_argument('--'+n,type=Path,required=True)
 a=p.parse_args();a.output.mkdir(exist_ok=False,parents=True);exe=a.probe.resolve();rom=a.rom.resolve();cases=[]
 for seed in range(4):
  capture=OWNER/'build/period-restart-attribution-v1'/f'period-{seed}-ready1-children-v{3 if seed==3 else 2}'
  out=a.output/f'period-{seed}';out.mkdir();cases.append(verify(capture,rom,exe,out))
 report=dict(passed=True,verifier_sha256=sha(__file__),module_sha256=sha(ROOT/'src/nba_period_entry_prefix.c'),probe_sha256=sha(exe),rom_sha256=sha(rom),native_contract_sha256=CONTRACT_SHA,native_freeze_sha256=FREEZE_SHA,cases=cases,compared_words=sum(c['compared_words']for c in cases))
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
