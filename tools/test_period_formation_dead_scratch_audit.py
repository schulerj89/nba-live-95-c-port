"""Poison omitted CPU inputs in original ROM execution from current C roles state."""
import argparse,hashlib,json,sys
from pathlib import Path
def main():
 p=argparse.ArgumentParser()
 for n in ('source','rom','traces','output'):p.add_argument('--'+n,type=Path,required=True)
 a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);sys.path.insert(0,str(a.source.resolve()/'tools'))
 import verify_period_formation as v
 import verify_period_roles_v3 as role
 from period_roles_rom_reference_v2 import original
 assert v.sha(a.rom)==v.ROM_SHA;rom=a.rom.read_bytes();names=v.mapping();rnames=role.mapping();byaddress={addr:(i,width)for i,(_,addr,width)in enumerate(names)}
 aliases=[];dead=[]
 for i,(name,address,width)in enumerate(rnames):
  if address in byaddress:
   j,w=byaddress[address];assert w==width;aliases.append((i,j,name))
  else:dead.append((i,name,address,width))
 assert len(aliases)==211 and len(dead)==12
 reports=[];comparisons=0;pcs=set();steps=0
 for path in sorted(a.traces.glob('*.jsonl')):
  rows=[json.loads(s)for s in path.read_text().splitlines()];start=next((r for r in rows if r['pc']==0x86e1e5),None);end=next((r for r in rows if r['pc']==0x86e1f7 or r['kind']==4),None)
  if not start or not end:continue
  for pattern in range(3):
   words=[0]*223
   for ri,ci,_ in aliases:words[ri]=start['values'][ci]
   for i,name,address,width in dead:words[i]=(0xffff if pattern==0 else (address*257)^0x5a5a if pattern==1 else (0x39eb+i*127))&65535
   expected,seen,count,writes=original(rom,words,rnames);last=expected[-1];pcs.update(seen);steps+=count
   assert last['pc']==end['pc'],(path.name,'boundary')
   for ri,ci,name in aliases:
    assert last['words'][ri]==end['values'][ci],(path.name,pattern,name,last['words'][ri],end['values'][ci]);comparisons+=1
   reports.append(dict(trace=path.name,pattern=pattern,terminal_pc=last['pc']))
 report=dict(passed=True,cases=reports,compared_semantic_values=comparisons,source_instructions=steps,source_pcs=len(pcs),omitted_fields=[name for _,name,_,_ in dead],reference_sha256=v.sha(a.source/'tools/period_roles_rom_reference_v2.py'),scope='actual ROM diagnostic from current C-produced E1E5 state; varying all12 omitted CPU input words cannot change211owned outputs or stop; no natural/timing/CPU-residue claim')
 (a.output/'report.json').write_text(json.dumps(report,indent=2));print({k:value for k,value in report.items()if k!='cases'})
if __name__=='__main__':main()
