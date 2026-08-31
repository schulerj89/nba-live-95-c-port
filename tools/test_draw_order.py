"""Original-ROM edge/domain and persistent-state tests, no native after-seeding."""
import argparse,itertools,json,random,struct,subprocess
from pathlib import Path
import verify_draw_order as v
from draw_order_rom_reference import original
def main(a):
 out=a.output.resolve();out.mkdir(parents=True,exist_ok=False);a.exe=a.exe.resolve()
 v.check_build(a.exe);v.source(a.rom);rom=a.rom.read_bytes();rng=random.Random(0x80fc80)
 records=[];expected=[];pcs=set();steps=writes=0
 def add(op,state,inputs):
  nonlocal steps,writes
  result=original(rom,op,state['order'],state['depth'],**inputs)
  pcs.update(result['pcs']);steps+=result['steps'];writes+=len(result['writes'])
  records.append(v.binary(op,state,inputs));expected.append({k:result[k]for k in('order','depth')})
 edge=[0,1,2,3,4,7,0x7ffe,0x7fff,0x8000,0x8001,0xfffc,0xfffd,0xfffe,0xffff]
 zero=dict(xs=[0]*12,ys=[0]*12,camera=0)
 # Every pair of16-bit depth edge values at all11 adjacent positions.
 for a0,b0,slot in itertools.product(edge,edge,range(11)):
  depths=[1234]*12;depths[slot]=a0;depths[slot+1]=b0
  add(2,dict(order=v.IDENTITY[:],depth=depths),zero)
 # Subtraction/shift/camera boundaries; one actor rotated through every slot.
 for a0,b0,camera in itertools.product(edge,edge,edge):
  inputs=dict(xs=[a0]*12,ys=[b0]*12,camera=camera)
  add(3,dict(order=v.IDENTITY[::-1],depth=[0xa5a5]*12),inputs)
 # Varied canonical permutations, fullword input and existing depth carry.
 for i in range(768):
  state=dict(order=rng.sample(v.IDENTITY,12),depth=[rng.randrange(65536)for _ in range(12)])
  inp=dict(xs=[rng.randrange(65536)for _ in range(12)],ys=[rng.randrange(65536)for _ in range(12)],camera=rng.randrange(65536))
  add(i%4,state,inp)
 got=v.run_probe(a.exe,out,'source',records)
 for row,want in zip(got,expected):v.check(row['ok'],'source refusal');v.exact({k:row[k]for k in('order','depth')},want)
 # Initialization preserves arbitrary depths and repairs arbitrary old order.
 guard_records=[];guard_expected=[]
 for bad in(0,0x34ea,0x34ec,0x40eb,0xffff,v.IDENTITY[1]):
  for op in range(4):
   state=dict(order=v.IDENTITY[:],depth=[0x7000+i for i in range(12)]);state['order'][0]=bad
   guard_records.append(v.binary(op,state,zero));guard_expected.append((op==0,dict(order=v.IDENTITY[:]if op==0 else state['order'],depth=state['depth'])))
 for row,(ok,want)in zip(v.run_probe(a.exe,out,'guards',guard_records),guard_expected):v.exact(row['ok'],ok);v.exact({k:row[k]for k in('order','depth')},want)
 # Equal keys retain incoming order. A reverse list needs more than one pass.
 equal=v.run_probe(a.exe,out,'ties',[v.binary(2,dict(order=v.IDENTITY[::-1],depth=[5]*12),zero)])[0]
 v.exact(equal['order'],v.IDENTITY[::-1])
 state=dict(order=v.IDENTITY[::-1],depth=list(range(12)));passes=[]
 for i in range(11):
  result=original(rom,2,state['order'],state['depth'],**zero)
  row=v.run_probe(a.exe,out,f'pass-{i:02}',[v.binary(2,state,zero)])[0]
  v.exact(row['order'],result['order']);state={k:row[k]for k in('order','depth')};passes.append(state['order'])
 v.check(passes[0]!=v.IDENTITY and passes[-1]==v.IDENTITY,'singlepass persistence versus fullsort')
 malformed=[b'',b'DOR1',records[0][:-1],records[0]+b'x',b'BAD!'+records[0][4:],records[0][:4]+struct.pack('<H',4)+records[0][6:]]
 for i,data in enumerate(malformed):
  path=out/f'malformed-{i}.input';path.write_bytes(data);r=subprocess.run([str(a.exe),str(path)],capture_output=True)
  v.check(type(r.returncode)is int and r.returncode!=0,'malformed binary accepted')
 report=dict(passed=True,source_cases=len(records),source_words=len(records)*24,instruction_decisions=steps,source_write_positions=writes,source_pcs=sorted(pcs),guard_cases=len(guard_records),null_checks_per_probe=7,persistent_passes=11,malformed_binary_cases=len(malformed),scope='controlled original-ROM source execution; not natural reachability/timing')
 (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2));return report
if __name__=='__main__':
 p=argparse.ArgumentParser()
 for key in('rom','exe','output'):p.add_argument('--'+key,type=Path,required=True)
 main(p.parse_args())
