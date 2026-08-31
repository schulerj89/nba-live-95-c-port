"""Independent metadata/adapter-domain negative checks, never native edits."""
import argparse,copy,json,struct
from pathlib import Path
def main():
 p=argparse.ArgumentParser();p.add_argument('--verifier',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 text=a.verifier.read_text().split('\ncases=[];compared=0;first=None\n',1)[0]
 assert 'def validate_metadata' in text
 text=text.replace("OUT=BASE/'strict-v1';OUT.mkdir(exist_ok=False)","OUT=AUDIT_OUT")
 env={'__file__':str(a.verifier.resolve()),'AUDIT_OUT':a.output};exec(compile(text,str(a.verifier),'exec'),env);validate=env['validate_metadata'];root=env['ROOT'];directory=root/'build/period-restart-attribution-v1/period-0-ready1-children-v2';rows=[json.loads(s)for s in(directory/'boundaries.jsonl').read_text().splitlines()]
 before=next(r for r in rows if r['tag']=='assignment.before');raw=(directory/before['raw']).read_bytes();validate(before,0x86e0ac,raw,'assignment');results=[]
 cases=[('extra-row-key',lambda d:d.update(extra=0)),('missing-tag',lambda d:d.pop('tag')),('nontext-tag',lambda d:d.update(tag=0)),('missing-raw-path',lambda d:d.pop('raw')),('external-raw-path',lambda d:d.update(raw='../outside.bin')),('missing-fields',lambda d:d.pop('fields')),('wrong-fields-type',lambda d:d.update(fields=[])),('boolean-field-word',lambda d:d['fields'].update({'093e':False})),('wrong-court-clock',lambda d:d.update(court=0))]
 for name,edit in cases:
  row=copy.deepcopy(before);edit(row)
  try:validate(row,0x86e0ac,raw,'assignment')
  except(ValueError,TypeError,KeyError,AssertionError)as e:accepted=False;detail=repr(e)
  else:accepted=True;detail='metadata accepted'
  results.append(dict(name=name,accepted=accepted,detail=detail))
 for name,mode,tag,pc,change in [('noncanonical-actor-ID','assignment','assignment.before',0x86e0ac,lambda b:struct.pack_into('<H',b,0x34eb,1)),('carried-roster-pointer','assignment','assignment.before',0x86e0ac,lambda b:b.__setitem__(slice(0x3471,0x3475),b[0x3475:0x3479])),('nonzero-sort-sentinel','sort','assignment.after',0x86e0b0,lambda b:struct.pack_into('<H',b,0x34d1,0x4000))]:
  row=next(r for r in rows if r['tag']==tag);data=bytearray((directory/row['raw']).read_bytes());change(data)
  try:validate(row,pc,bytes(data),mode)
  except(ValueError,TypeError,KeyError,AssertionError)as e:accepted=False;detail=repr(e)
  else:accepted=True;detail='source-incompatible adapter prestate accepted'
  results.append(dict(name=name,accepted=accepted,detail=detail))
 report=dict(passed=not any(c['accepted']for c in results),cases=results,scope='parsed gate and unsupported raw-domain copies; original files remain hash-identical')
 (a.output/'report.json').write_text(json.dumps(report,indent=2));print(report)
if __name__=='__main__':main()
