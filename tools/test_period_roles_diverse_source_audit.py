"""Independent diverse input selection against the reviewed fixed-ROM diagnostic.

The diagnostic remains a source-only reference: it does not prove these inputs
occur in normal play, and neither native after-state nor expected fields enter C.
"""
import argparse,hashlib,importlib.util,json,random,struct,sys
from pathlib import Path
def main():
 p=argparse.ArgumentParser()
 for k in('source','rom','exe','native','output'):p.add_argument('--'+k,type=Path,required=True)
 a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);sys.path.insert(0,str(a.source.resolve()/'tools'))
 import verify_period_roles_v2 as v
 from period_roles_rom_reference_v2 import original
 v.check_build(a.exe.resolve());v.source(a.rom);rom=a.rom.read_bytes();names=v.mapping();idx={n:i for i,(n,_,_)in enumerate(names)};seed=list(struct.unpack('<225H',next(a.native.glob('*.input')).read_bytes()))[2:];r=random.Random(0xc0f50831);results=[];pcs=set();instructions=0;failures=[]
 edge=[0,1,2,5,6,7,8,0x7fff,0x8000,0x8001,0xffff]
 for case in range(640):
  words=seed[:]
  def setv(key,value):words[idx[key]]=value&65535
  def value():return r.choice(edge)if case<320 else r.randrange(65536)
  for name,_,width in names:
   if width==2:setv(name,value())
  for team in range(2):
   setv(f'prefix.contexts[{team}].opponent_02',0x476b if team==0 else 0x46eb);setv(f'prefix.contexts[{team}].first_actor_04',0x34eb+team*1280)
   order=list(range(5 if team==0 else 0,10 if team==0 else 5));r.shuffle(order)
   for slot,actor in enumerate(order):setv(f'contexts[{team}].order_49[{slot}]',actor*2)
  for field in('assignment_74','base_76','alternate_78'):
   permutation=list(range(5));r.shuffle(permutation)
   for i,j in enumerate(permutation):
    prefix='prefix.'if field=='assignment_74'else'';setv(f'{prefix}actors[{i}].{field}',(j+5)*2);setv(f'{prefix}actors[{j+5}].{field}',i*2)
  for i in range(10):setv(f'actors[{i}].id_00',i);setv(f'actors[{i}].team_6e',0 if i<5 else 5)
  setv('live_0936',0x81+(case%2));setv('prefix.ball_pointer_0910',0x3eeb)
  setv('prefix.rebuild_09d6',0 if case%3==0 else value());setv('nearest_offense_09de',0x34eb+r.randrange(10)*256 if case%2 else value());setv('ball_anchor_distance_8c',r.randrange(240)if case%4 else value());setv('ball_assignment_74',r.randrange(10)*2 if case%3 else value())
  try:
   expected,seen,steps,writes=original(rom,words,names);pcs.update(seen);instructions+=steps
   got=v.run_probe(a.exe,a.output,'case-'+str(case),v.binary_input(words));v.typed(got,expected)
  except (AssertionError,ValueError,IndexError)as e:
   failures.append(dict(case=case,error=repr(e)));break
  results.append(dict(case=case,rows=len(got),terminal=got[-1]['kind'],pc=hex(got[-1]['pc'])))
 report=dict(passed=not failures,cases=results,failures=failures,source_instructions=instructions,source_pcs=len(pcs),reference_sha256=v.sha(Path(v.__file__).with_name('period_roles_rom_reference_v2.py')),scope='independently selected full-word/edge inputs against reviewed original-ROM source diagnostic; no natural reachability or timing claim')
 (a.output/'report.json').write_text(json.dumps(report,indent=2));print({k:val for k,val in report.items()if k!='cases'})
if __name__=='__main__':main()
