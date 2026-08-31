"""Controlled source-only ROM differential and strict typed API contracts.
Native fixture bytes are isolated component inputs only, never normal-state seeds.
The test changes before-state fields; no expected-after input is passed to C.
"""
import argparse,json,struct,subprocess
from pathlib import Path
import verify_period_roles_v2 as v
from period_roles_rom_reference_v2 import original

def main(a):
 out=a.output.resolve();out.mkdir(parents=True,exist_ok=False);v.check_build(a.exe.resolve());rom=a.rom.read_bytes();v.source(a.rom)
 names=v.mapping();idx={n:i for i,(n,_,_)in enumerate(names)}
 fixtures=[]
 for f in sorted(a.native.glob('*.input')):
  raw=f.read_bytes();v.check(len(raw)==450,'isolated fixture extent');magic,version,*words=struct.unpack('<225H',raw);v.check((magic,version)==(0x5252,2),'isolated fixture protocol');fixtures.append((f.stem,words))
 v.check(len(fixtures)==4,'four typed native before fixtures')
 cases=[];pcs=set();total=0;writes=0
 def run(label,base,changes):
  nonlocal total,writes
  words=base[:]
  for name,value in changes.items():words[idx[name]]=value&65535
  expected,touched,steps,bus=original(rom,words,names)
  got=v.run_probe(a.exe,out,label,v.binary_input(words))
  for j,(want,actual)in enumerate(zip(expected,got)):
   differences=[(n,g,e)for(n,_,_),g,e in zip(names,actual['words'],want['words'])if g!=e]
   v.check(not differences,label+' row'+str(j)+': '+str(differences[:20]))
  v.typed(got,expected);pcs.update(touched);total+=steps;writes+=len(bus)
  cases.append(dict(case=label,rows=len(got),terminal=got[-1]['kind'],pc=got[-1]['pc'],source_instructions=steps,source_writes=len(bus)))
  return got
 for label,base in fixtures:
  run(label+'-unchanged',base,{})
  for cadence in (0,1,2,3,0xffff,0xfff0,0x8002,0x8000):
   run(label+'-cadence-'+str(cadence),base,{'prefix.cadence_09d2':cadence,'nearest_offense_09de':0x34eb})
  run(label+'-unmapped-record-stop',base,{'prefix.rebuild_09d6':1})
  for rng in (0,1,0x8000,0xffff,0x9146):
   run(label+'-rebuild-'+str(rng),base,{'prefix.rebuild_09d6':1,'rng_07f6':rng,'nearest_offense_09de':0x34eb})
  run(label+'-negative-camera-rebuild',base,{'prefix.rebuild_09d6':1,'prefix.camera_093a':0xffff,'nearest_offense_09de':0x34eb})
 base=fixtures[0][1]
 for live in (0x81,0x82):
  for owner in (0,2,5,7,0xffff):
   for selected in (0,5):
    change={'prefix.cadence_09d2':0x8002,'live_0936':live,'prefix.camera_093a':0,'prefix.owner_093e':owner,'ball_assignment_74':selected*2,'ball_anchor_distance_8c':0,'receiver_0946':0xffff}
    run(f'primary-{live}-{owner}-{selected}',base,change)
 for mode in (6,7,11,0x8000,0xffff):
  change={'prefix.cadence_09d2':0,'prefix.owner_093e':0xffff,'receiver_0946':0xffff,'ball_anchor_distance_8c':0}
  change.update({f'actors[{i}].mode_5e':mode for i in range(10)})
  run('ownerless-mode-'+str(mode),base,change)
 for x,y in ((32767,-32768),(-32768,0),(0,-32768),(-32768,-32768),(0,1),(0,0)):
  change={'prefix.rebuild_09d6':1,'rng_07f6':0x8000,'ball_anchor_distance_8c':0,'prefix.ball_x':0,'prefix.ball_y':0}
  change.update({f'prefix.actors[{i}].x':x for i in range(10)});change.update({f'prefix.actors[{i}].y':y for i in range(10)})
  run(f'wrapped-distance-{x}-{y}',base,change)
 # Distinct valid base/alternate bijections prove the three rebuild passes
 # are not accidentally collapsed to one assignment source.
 change={'prefix.rebuild_09d6':0xffff,'rng_07f6':0x1234,'nearest_offense_09de':0x34eb}
 for field,shift in (('base_76',1),('alternate_78',2)):
  for i in range(5):
   j=(i+shift)%5;change[f'actors[{i}].{field}']=(j+5)*2;change[f'actors[{j+5}].{field}']=i*2
 run('distinct-bijections-rebuild',base,change)
 for mode in (0,1,6,7,0x8000,0xffff):
  for inbound in (0,1,65535):
   change={'prefix.rebuild_09d6':1,'ball_anchor_distance_8c':0,'inbound_0954':inbound,'prefix.owner_093e':0,'rng_07f6':0x8001}
   change.update({f'actors[{i}].mode_5e':mode for i in range(10)})
   run(f'rebuild-mode-{mode}-inbound-{inbound}',base,change)
 run('assignment-child-stop',base,{'prefix.cadence_09d2':0,'ball_anchor_distance_8c':0,'ball_assignment_74':0xffff})
 run('table-index-stop',base,{'prefix.cadence_09d2':0,'ball_anchor_distance_8c':0,'ball_assignment_74':20})
 # Invalid initial domains reject before mutation/output; terminal immutability
 # and FIRST_RETURN-only resume are checked by the fresh C probe on every run.
 contracts=[]
 def reject(name,data,code):
  path=out/(name+'.input');path.write_bytes(data);r=subprocess.run([str(a.exe.resolve()),str(path)],capture_output=True,text=True)
  v.check(type(r.returncode)is int and r.returncode==code and r.stdout==''and r.stderr=='',name+' exact rejection');contracts.append(name)
 for name,field,val in [('live0','live_0936',0),('actor-id','actors[2].id_00',0),('team','actors[2].team_6e',5),('base-odd','actors[0].base_76',11),('base-reciprocal','actors[0].base_76',12),('current-negative','prefix.actors[0].assignment_74',65535),('alternate','actors[0].alternate_78',0),('order-duplicate','contexts[0].order_49[0]',10),('order-odd','contexts[0].order_49[0]',11),('ball-pointer','prefix.ball_pointer_0910',0)]:
  words=base[:];words[idx[field]]=val;reject(name,v.binary_input(words),4)
 data=v.binary_input(base);reject('trailing-byte',data+b'X',3);reject('short-word',data[:-1],3);reject('wrong-version',data[:2]+b'\x01\0'+data[4:],3)
 report=dict(passed=True,source_only_cases=cases,contract_tests=contracts,source_pc_count=len(pcs),source_pcs=[f'{pc:06x}'for pc in sorted(pcs)],source_instructions=total,source_writes=writes,typed_fields=len(names),scope='controlled original-ROM instruction decisions; no timing, stack/register or natural branch-reachability claim',reference_sha256=v.sha(Path(__file__).with_name('period_roles_rom_reference_v2.py')))
 (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print('PASS',len(cases),'source cases;',len(contracts),'contracts;',total,'ROM instructions')
if __name__=='__main__':
 p=argparse.ArgumentParser()
 for name in ('rom','exe','native','output'):p.add_argument('--'+name,type=Path,required=True)
 main(p.parse_args())
