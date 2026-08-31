"""Independent full native-case protocol/domain mutations; original files untouched."""
import argparse,copy,json,sys
from pathlib import Path
from unittest.mock import patch
def main():
 p=argparse.ArgumentParser()
 for k in('source','rom','exe','native','output'):p.add_argument('--'+k,type=Path,required=True)
 a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);sys.path.insert(0,str(a.source.resolve()/'tools'));import verify_period_roles_v2 as v
 run0=v.subprocess.run;firstbyte=next(i for i,(_,_,width)in enumerate(v.mapping())if width==1);results=[]
 tests=[('baseline',None),('first-byte-overflow',lambda rows:rows[0]['words'].__setitem__(firstbyte,256)),('first-byte-ffff',lambda rows:rows[0]['words'].__setitem__(firstbyte,65535)),('final-byte-overflow',lambda rows:rows[-1]['words'].__setitem__(firstbyte,256)),('first-word-bool',lambda rows:rows[0]['words'].__setitem__(0,False)),('first-word-overflow',lambda rows:rows[0]['words'].__setitem__(0,65536)),('first-record-ptr',lambda rows:rows[0].update(record_pointer=1)),('first-call-count',lambda rows:rows[0].update(completed_calls=0)),('wrong-first-pc',lambda rows:rows[0].update(pc=0)),('first-extra-field',lambda rows:rows[0].update(extra=1)),('reverse-rows',lambda rows:rows.reverse()),('duplicate-first',lambda rows:rows.insert(0,copy.deepcopy(rows[0]))),('only-final',lambda rows:rows.pop(0))]
 for name,change in tests:
  out=a.output/name;out.mkdir();hits=[0]
  def run(*args,**kwargs):
   result=run0(*args,**kwargs)
   if change:
    rows=[json.loads(line)for line in result.stdout.splitlines()];change(rows);result.stdout='\n'.join(json.dumps(row)for row in rows)+'\n';hits[0]+=1
   return result
  try:
   with patch.object(v.subprocess,'run',run):v.verify_case(a.native.resolve(),a.rom.resolve(),a.exe.resolve(),out)
  except (ValueError,AssertionError,KeyError,TypeError,IndexError)as e:accepted=False;detail=repr(e)
  else:accepted=True;detail='complete223finalnativefields compared'
  assert change is None or hits[0]>0;results.append(dict(name=name,accepted=accepted,expected=change is None,hits=hits[0],detail=detail))
 report=dict(passed=all(r['accepted']==r['expected']for r in results),byte_field=v.mapping()[firstbyte],cases=results,scope='native final comparison; first-boundary value parity is not claimed, but its declared byte-domain schema is required')
 (a.output/'report.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
if __name__=='__main__':main()
