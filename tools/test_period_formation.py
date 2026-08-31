"""Source-only composition cases, alias contracts, and explicit refusals."""
import argparse,hashlib,importlib.util,json,subprocess
from pathlib import Path
import verify_period_formation as v
import verify_period_roles_v3 as role_v
from period_roles_rom_reference_v2 import original as role_original

def main(a):
 for k,value in vars(a).items():
  if isinstance(value,Path):setattr(a,k,value.resolve())
 a.output.mkdir(parents=True,exist_ok=False);v.check_build(a.exe);v.check(v.sha(a.pack)==v.PACK_SHA,'pack');rom=a.rom.read_bytes();v.check(v.sha(a.rom)==v.ROM_SHA,'ROM')
 names=v.mapping();index={n:i for i,(n,_,_)in enumerate(names)}
 inputs=[]
 for p in sorted(a.native.glob('*.input')):
  data=p.read_bytes();v.check(data[:4]==b'PFC1'and len(data)==2110,'typed initial fixture');at=4;values=[]
  for _,_,w in names:values.append(int.from_bytes(data[at:at+w],'little'));at+=w
  inputs.append((p.stem,values))
 v.check(len(inputs)==4,'four isolated component inputs');base=inputs[0][1];reports=[];role_checks=0;coordinate_checks=0
 v.check(v.sha(a.coordinates)=='881ee55b4e447218353335ca2550ef878124d0d8532314038776f2e69340e8d0','accepted independent coordinate oracle identity')
 spec=importlib.util.spec_from_file_location('independent_coordinate',a.coordinates);coordinates=importlib.util.module_from_spec(spec);spec.loader.exec_module(coordinates)
 role_aliases=json.loads((v.ROOT/'.analysis/period-formation-role-alias-map-v1.json').read_text())['mapped'];role_indices={n:i for i,(n,_,_)in enumerate(role_v.mapping())}
 v.check(len(role_aliases)==211 and len({row[0]for row in role_aliases})==211 and len({row[1]for row in role_aliases})==211,'role semantic alias closure')
 for source,target,width in role_aliases:
  v.check(role_v.mapping()[role_indices[source]][1:]==names[index[target]][1:] and width==names[index[target]][2],'alias source-address and width identity')
 def proof_roles(rows):
  nonlocal role_checks
  start=next((r for r in rows if r['pc']==0x86e1e5),None);end=next((r for r in rows if r['pc']==0x86e1f7 or r['kind']==4),None)
  if not start or not end:return
  initial=[0]*223
  # Only dead CPU temporaries start atzero. Production preflight rejects
  # paths that could consume carried92, and all211semantic fields are explicit.
  for source,target,width in role_aliases:initial[role_indices[source]]=start['values'][index[target]]
  expected,_,_,_=role_original(rom,initial,role_v.mapping());last=expected[-1]
  for source,target,width in role_aliases:
   got=end['values'][index[target]];want=last['words'][role_indices[source]];v.check(got==want,'role alias '+target);role_checks+=1
  v.check(end['pc']==last['pc'],'role source stop/return PC')
 def run(name,changes,terminal=2,pc=0x86e207,reason=0):
  values=base[:]
  for field,value in changes.items():values[index[field]]=value&((1<<(8*names[index[field]][2]))-1)
  rows=v.run(a.exe,a.pack,a.output,name,values);last=rows[-1]
  v.check((last['kind'],last['pc'],last['refusal'])==(terminal,pc,reason),name+' terminal '+str(last)[:190])
  proof_roles(rows);reports.append(dict(name=name,boundaries=len(rows),kind=terminal,pc=pc));return values,rows
 for period in range(5):
  for tip in (0,5):
   for anchor in (-336,336):
    values,rows=run(f'formation-{period}-{tip}-{anchor}',{'input.period':period,'input.tip_winner':tip,'input.anchor_x[0]':anchor,'input.anchor_x[1]':-anchor})
    for pair in range(5):
     expected,_=coordinates.original(rom,period,tip,anchor,pair)
     row=next(r for r in rows if r['pc']==0x86dfcb and r['actor']==pair)
     for actor,fields in expected.items():
      for name,want in fields.items():v.check(row['values'][index[f'parent.actors[{actor}].{name}']]==want,'source coordinate '+name);coordinate_checks+=1
 # Exact actor fractions and original stale ready/dead-ball carry survive.
 changes={'parent.ready_09ba':0xfeed,'parent.dead_ball_x_09b0':0x1234,'parent.dead_ball_y_09b2':0xabcd}
 for i in range(10):
  for axis,word in [('x',0x1234),('y',0x5678),('z',0x9abc)]:changes[f'parent.actors[{i}].{axis}_fraction']=(word+i)&65535
 values,rows=run('fraction-and-ready-carry',changes)
 for field,value in changes.items():v.check(rows[-1]['values'][index[field]]==(value&65535),'exact original carry '+field)
 # All queues are real carried words, not reconstructed from cursors/phases.
 changes={'delta':0}
 for i in range(10):
  for part in ('upper','lower'):
   for slot in range(3):changes[f'actors[{i}].{part}_queue[{slot}]']=0xa100+i*16+slot
 values,rows=run('full-queue-carry',changes)
 for field,value in changes.items():v.check(rows[-1]['values'][index[field]]==value,'full queue copy '+field)
 changes={'parent.ball.id':0xffff,'parent.list_cursor':0xabcd}
 for i in range(10):
  changes[f'parent.actors[{i}].id']=65535;changes[f'parent.actors[{i}].list_link']=0xabcd;changes[f'parent.actors[{i}].field_a6']=0xbeef
 _,rows=run('outputs-not-initial-assumptions',changes)
 for i in range(10):v.check(rows[-1]['values'][index[f'parent.actors[{i}].id']]==i,'published actor ID')
 for rebuild in (0,1,0xffff):
  for cadence in (0,1,12,0x8002,0xffff):
   values,rows=run(f'role-{rebuild}-{cadence}',{'role_rebuild':rebuild,'role_cadence':cadence,'ball_anchor_distance':0})
   want=30 if rebuild else ((cadence-2)&65535)
   if not rebuild and want&32768:want=(want+30)&65535
   want=(want-2)&65535
   if want&32768:want=(want+30)&65535
   v.check(rows[-1]['values'][index['role_cadence']]==want,'literal two-call cadence')
 _,rows=run('role-unmapped-record',{'role_rebuild':1},4,0x85bf51)
 v.check(rows[-1]['role_kind']==3 and rows[-1]['role_pointer']==0xa5 and rows[-1]['values'][index['role_rebuild']]==0,'real rebuild before explicit read stop')
 _,rows=run('role-assignment-child',{'role_cadence':0,'ball_anchor_distance':0,'ball_assignment':0xffff},4,0x85bf98)
 v.check(rows[-1]['role_kind']==4,'unresolved assignment child remains explicit')
 for name,changes,pc,reason in [
  ('human-appearance',{'controllers.actor_assignment[0]':0},0x86dfcb,1),
  ('base-domain',{'actors[0].base_state':19},0x86dfcb,1),
  ('roster-table',{'roster_table[0][0]':0},0x86e0ac,2),
  ('roster-slot',{'contexts[0].roster[0]':12},0x86e0ac,2),
  ('selector',{'contexts[0].selector[0]':127},0x86e0ac,2),
  ('team',{'contexts[0].team':29},0x86e0ac,2),
  ('draw-permutation',{'draw_order[0]':0},0x86e1f7,8)]:
  _,rows=run(name,changes,3,pc,reason)
  v.check(rows[-1]['values']==rows[-2]['values'],'child refusal preserves last complete checkpoint '+name)
 # Initial malformed/domain rejection must produce no asset side effects/output.
 rejects=[]
 def reject(name,data,code):
  p=a.output/(name+'.input');p.write_bytes(data);r=subprocess.run([str(a.exe),str(a.pack),str(p)],capture_output=True,text=True)
  v.check(type(r.returncode)is int and r.returncode==code and r.stdout==''and r.stderr=='',name+' exact rejection');rejects.append(name)
 for name,field,value in [('period','input.period',5),('tip','input.tip_winner',1),('sentinel','leading_sentinel',1),('opponent','contexts[0].opponent_pointer',0),('actor-base','contexts[0].first_actor_pointer',0)]:
  values=base[:];values[index[field]]=value;reject(name,v.binary(values),4)
 data=v.binary(base);reject('short',data[:-1],3);reject('extra',data+b'x',3);reject('magic',b'BAD!'+data[4:],3)
 report=dict(passed=True,cases=reports,source_coordinate_fields=coordinate_checks,source_role_alias_fields=role_checks,entry_rejections=rejects,scope='controlled current-state composition/alias/source proof; native fixtures only isolated entry inputs; no normal-init/phase claim',coordinate_reference_sha256=v.sha(a.coordinates))
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print('PASS',len(reports),'composition cases;',coordinate_checks,'ROMcoordinates;',role_checks,'ROMrolefields;',len(rejects),'entryrejections')
if __name__=='__main__':
 p=argparse.ArgumentParser()
 for name in ('rom','pack','exe','native','coordinates','output'):p.add_argument('--'+name,type=Path,required=True)
 main(p.parse_args())
