"""Independent post-hash route/build/output corruptions, no frozen file writes."""
import argparse,copy,importlib,json,subprocess,sys
from pathlib import Path
from unittest.mock import patch

def main():
 p=argparse.ArgumentParser()
 for n in ('source','rom','pack','exe','native','output'):p.add_argument('--'+n,type=Path,required=True)
 a=p.parse_args()
 for n,value in vars(a).items():setattr(a,n,value.resolve())
 a.output.mkdir(parents=True,exist_ok=False);sys.path.insert(0,str(a.source/'tools'));v=importlib.import_module('verify_period_formation');v.check_build(a.exe)
 read0=Path.read_text;run0=subprocess.run;reports=[]
 mutations=['duplicate-first','missing-after','swap-appearance','insert-late-first','decimal-last','wide-index-last','field-vs-raw','entry-X','entry-Y','unknown-tag','decreasing-clock','duplicate-key','manifest-command','manifest-source-omit','isolation-initial-type','completion']
 output_cases=['extra-result-row','first-word-float','last-word-bool','wide-byte-last','wide-address-last','extra-JSON-key','wrong-last-role-pointer','missing-stderr','stderr-nontext','stdout-nontext']
 build_cases=['build-exe-path','build-header-omit','build-status-float','build-source-bytes-bool']
 for name in ['baseline']+mutations+output_cases+build_cases:
  hits=[0];calls=[];out=a.output/name;out.mkdir()
  def read(path,*args,**kwargs):
   text=read0(path,*args,**kwargs)
   if name in build_cases and path==a.exe.parent/'build-manifest.json':
    m=json.loads(text);hits[0]+=1
    if name=='build-exe-path':m['executable']['path']=str(a.exe.with_name('not-built.exe'))
    elif name=='build-header-omit':m['sources'].pop(next(n for n in m['sources']if n.endswith('nba_period_formation.h')))
    elif name=='build-status-float':m['compiler_exit']=0.0
    else:next(iter(m['sources'].values()))['bytes']=True
    return json.dumps(m)
   if path==a.native/'manifest.json'and name in ('manifest-command','manifest-source-omit','isolation-initial-type','completion'):
    m=json.loads(text);hits[0]+=1
    if name=='manifest-command':m['arguments'].insert(1,'--load-state=foreign')
    elif name=='manifest-source-omit':m['sources'].pop(next(n for n in m['sources']if n.endswith('capture.lua')))
    elif name=='isolation-initial-type':m['isolation']['settings']['Debug']['ScriptWindow']['ScriptTimeout']=60.0
    else:m['completion']=''
    return json.dumps(m)
   if path!=a.native/'boundaries.jsonl'or name not in mutations:return text
   rows=[json.loads(s)for s in text.splitlines()];before=next(i for i,r in enumerate(rows)if r['tag']=='appearance.first.before');start=next(i for i,r in enumerate(rows)if r['tag']=='formation.table');hits[0]+=1
   if name=='duplicate-first':rows.insert(before,copy.deepcopy(rows[before]))
   elif name=='missing-after':rows.pop(before+1)
   elif name=='swap-appearance':rows[before],rows[before+2]=rows[before+2],rows[before]
   elif name=='insert-late-first':rows.insert(-1,copy.deepcopy(rows[before]))
   elif name=='decimal-last':rows[-1]['ps']|=8
   elif name=='wide-index-last':rows[-1]['index']=65536
   elif name=='field-vs-raw':rows[before]['fields']['093e']^=1
   elif name=='entry-X':rows[start]['x']=1
   elif name=='entry-Y':rows[start]['y']=0
   elif name=='unknown-tag':rows[before]['tag']='appearance.maybe'
   elif name=='decreasing-clock':rows[-1]['frame']=0
   elif name=='duplicate-key':return text.replace('"index":','"index":1,"index":',1)
   return '\n'.join(json.dumps(r)for r in rows)
  def run(command,*args,**kwargs):
   # Every C invocation receives exactly the canonical single DD97-before input.
   rows=[json.loads(s)for s in read0(a.native/'boundaries.jsonl').splitlines()];before=next(r for r in rows if r['tag']=='formation.table');raw=(a.native/before['raw']).read_bytes()
   assert command==[str(a.exe),str(a.pack),str(out/(a.native.name+'.input'))],command
   assert Path(command[-1]).read_bytes()==v.binary(v.project(raw));calls.append(command)
   result=run0(command,*args,**kwargs)
   if name not in output_cases:return result
   hits[0]+=1
   if name=='missing-stderr':result.stderr='';return result
   if name=='stderr-nontext':result.stderr=result.stderr.encode();return result
   if name=='stdout-nontext':result.stdout=result.stdout.encode();return result
   if name=='extra-JSON-key':result.stdout=result.stdout.replace('"kind":','"unknown":0,"kind":',1);return result
   rows=[json.loads(s)for s in result.stdout.splitlines()]
   if name=='extra-result-row':rows.append(copy.deepcopy(rows[-1]))
   elif name=='first-word-float':rows[0]['values'][0]=float(rows[0]['values'][0])
   elif name=='last-word-bool':rows[-1]['values'][0]=True
   elif name=='wide-byte-last':rows[-1]['values'][next(i for i,(_,_,w)in enumerate(v.mapping())if w==1)]=256
   elif name=='wide-address-last':rows[-1]['values'][next(i for i,(_,_,w)in enumerate(v.mapping())if w==4)]=1<<32
   else:rows[-1]['role_pointer']=1
   result.stdout=''.join(json.dumps(r)+'\n'for r in rows);return result
  try:
   with patch.object(Path,'read_text',read),patch.object(v.subprocess,'run',run):
    v.check_build(a.exe);r=v.case(a.native,a.rom,a.exe,a.pack,out)
  except (ValueError,AssertionError,TypeError,KeyError,IndexError,StopIteration)as e:accepted=False;detail=repr(e)
  else:accepted=True;detail=str(r['typed_comparisons'])+' canonical values compared'
  assert name=='baseline'or hits[0]>0,(name,'unreached')
  reports.append(dict(name=name,accepted=accepted,expected=name=='baseline',hits=hits[0],c_calls=len(calls),detail=detail))
 report=dict(passed=all(r['accepted']==r['expected']for r in reports),cases=reports,scope='parsed mutations after hashing; original fixtures unchanged; exact single-before command/input asserted for every C run')
 (a.output/'report.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
if __name__=='__main__':main()
