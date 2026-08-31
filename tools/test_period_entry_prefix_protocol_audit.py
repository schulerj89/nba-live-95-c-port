"""Independent before-only command, native topology and build integrity checks."""
import argparse,copy,json,sys
from pathlib import Path
from unittest.mock import patch
def main():
 p=argparse.ArgumentParser()
 for k in('source','rom','exe','output'):p.add_argument('--'+k,type=Path,required=True)
 a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);sys.path.insert(0,str(a.source.resolve()/'tools'));import verify_period_entry_prefix as v
 cap=v.OWNER/'build/period-restart-attribution-v1/period-0-ready1-children-v2';rows0=[json.loads(s)for s in(cap/'boundaries.jsonl').read_text().splitlines()];entry=next(r for r in rows0 if r['tag']=='formation.entry');expected=[str(a.exe.resolve()),str(a.rom.resolve()),str(cap/entry['raw'])];read0=Path.read_text;run0=v.subprocess.run;results=[]
 tests=['baseline','duplicate-entry','duplicate-clock','reverse-clocks','wrong-table-A','wrong-table-Y','wrong-table-carry','wrong-reset-A','wrong-clock-X','decimal-entry','extra-native-key','bool-native-SP','build-missing-source','build-extra-source','build-bool-schema','build-bool-status','build-wrong-executable','build-extra-key']
 for kind in tests:
  hits=[0];calls=[]
  def read(path,*args,**kwargs):
   txt=read0(path,*args,**kwargs)
   if path.name=='boundaries.jsonl'and path.parent==cap and kind!='baseline'and not kind.startswith('build-'):
    rows=[json.loads(s)for s in txt.splitlines()];indexes={r['tag']:i for i,r in enumerate(rows)};first=indexes['formation.entry'];clock=indexes['clock.select'];ready=indexes['clock.ready'];table=indexes['formation.table'];hits[0]+=1
    if kind=='duplicate-entry':rows.insert(first,copy.deepcopy(rows[first]))
    elif kind=='duplicate-clock':rows.insert(clock,copy.deepcopy(rows[clock]))
    elif kind=='reverse-clocks':rows[clock],rows[ready]=rows[ready],rows[clock]
    elif kind=='wrong-table-A':rows[table]['a']^=1
    elif kind=='wrong-table-Y':rows[table]['y']^=1
    elif kind=='wrong-table-carry':rows[table]['ps']^=1
    elif kind=='wrong-reset-A':rows[clock]['a']^=1
    elif kind=='wrong-clock-X':rows[ready]['x']^=1
    elif kind=='decimal-entry':rows[first]['ps']|=8
    elif kind=='extra-native-key':rows[first]['extra']=0
    elif kind=='bool-native-SP':rows[first]['sp']=False
    return '\n'.join(json.dumps(row)for row in rows)
   if path.name=='build-manifest.json'and kind.startswith('build-'):
    m=json.loads(txt);hits[0]+=1
    if kind=='build-missing-source':m['sources'].pop('include/nba_types.h')
    elif kind=='build-extra-source':m['sources']['unexpected.c']='0'*64
    elif kind=='build-bool-schema':m['schema']=True
    elif kind=='build-bool-status':m['compiler_exit']=False
    elif kind=='build-wrong-executable':m['executable']['path']=str(a.exe.resolve().with_name('other.exe'))
    elif kind=='build-extra-key':m['extra']=0
    return json.dumps(m)
   return txt
  def run(cmd,*args,**kwargs):assert cmd==expected,cmd;calls.append(cmd);return run0(cmd,*args,**kwargs)
  try:
   with patch.object(Path,'read_text',read),patch.object(v.subprocess,'run',run):v.verify(cap,a.rom.resolve(),a.exe.resolve(),None)
  except (AssertionError,ValueError,KeyError,TypeError,IndexError)as e:accepted=False;detail=repr(e)
  else:accepted=True;detail='canonical before-only command and all196608words compared'
  assert kind=='baseline'or hits[0]>0
  results.append(dict(name=kind,accepted=accepted,expected=kind=='baseline',mutated_reads=hits[0],C_calls=len(calls),detail=detail))
 report=dict(passed=all(r['accepted']==r['expected']for r in results),cases=results,scope='actual verifier from immutable hashes through canonical before-only command; no source/raw fixture mutations')
 (a.output/'report.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
if __name__=='__main__':main()
