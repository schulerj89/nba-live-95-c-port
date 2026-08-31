"""Independent ROM dataflow edges and strict native/response mutations."""
import argparse,copy,json,subprocess,types
from pathlib import Path
from unittest.mock import patch
import verify_period_entry_prefix as v
import period_entry_prefix_reference as reference
def main():
 p=argparse.ArgumentParser()
 for name in('rom','probe','output'):p.add_argument('--'+name,type=Path,required=True)
 a=p.parse_args();out=a.output.resolve();out.mkdir(exist_ok=False);rom=a.rom.resolve();exe=a.probe.resolve();data=rom.read_bytes()
 capture=v.OWNER/'build/period-restart-attribution-v1/period-0-ready1-children-v2'
 v.build(exe);contract=v.contract();manifest,native=contract.read_native(capture,rom)
 base=next(r for r in native if r['tag']=='formation.entry')['memory']
 cases=[]
 inputs=[(period,quarter)for period in(0,1,2,3,4,5,0x7fff,0x8000,0xffff)for quarter in range(4)]
 inputs.extend((period,quarter)for period in(0,1,2,3,4,0xffff)for quarter in(4,0x8000,0xffff))
 anchors=(0,1,0x150,0x7fff,0x8000,0xffff)
 inputs.extend((2,quarter)for quarter in range(36))
 for index,(period,quarter)in enumerate(inputs):
  before=bytearray((i*37+index*19)&255 for i in range(131072))
  for address,value in((0x926,period),(0x17b1,quarter),(0xa0c,(index*1237)&65535),(0x46f5,anchors[index%6]),(0x4775,anchors[(index//6)%6]),(0x9ba,(index*713)&65535),(0x9b0,0x8000),(0x9b2,0xffff)):
   reference.put(before,address,value)
  ok,expected,_=reference.reference(data,bytes(before));path=out/f'edge-{index:03d}.bin';path.write_bytes(before)
  actual,run=v.probe(exe,rom,path,True);(out/f'edge-{index:03d}.stdout').write_text(run.stdout)
  want=[v.word(expected,address)for _,address in v.mapping()]
  assert actual[0]['result']==int(ok)and actual[0]['words']==want,(index,period,quarter)
  cases.append(dict(period=period,quarter=quarter,valid=ok,anchors=[v.word(before,0x46f5),v.word(before,0x4775)],compared_words=159))
 # Also compare the fixed-source dataflow with all four native intermediate pairs.
 for seed in range(4):
  cap=v.OWNER/'build/period-restart-attribution-v1'/f'period-{seed}-ready1-children-v{3 if seed==3 else 2}'
  _,rows=v.attest(cap,rom);ok,expected,stages=reference.reference(data,rows[0]['memory'])
  assert ok and all(s==r['memory']for s,r in zip(stages,rows[1:]))
 baseline=v.verify(capture,rom,exe,None);first=next(r for r in native if r['tag']=='formation.entry')
 parsed,run=v.probe(exe,rom,capture/first['raw']);checks=[]
 def check(name,context):
  try:
   with context:v.verify(capture,rom,exe,None)
   rejected=False
  except(ValueError,KeyError,TypeError,IndexError,AssertionError):rejected=True
  checks.append(dict(name=name,rejected=rejected))
 for tag in('formation.entry','clock.select','clock.ready','formation.table'):
  idx=next(i for i,r in enumerate(native)if r['tag']==tag)
  for field,value in(('d',1),('ps',native[idx]['ps']|8),('ps',native[idx]['ps']|16),('ps',native[idx]['ps']|32),('sp',native[idx]['sp']-1),('frame',native[idx]['frame']+1),('court',native[idx]['court']+1)):
   changed=copy.deepcopy(native);changed[idx][field]=value
   check(tag+' '+field+' '+str(value),patch.object(v,'contract',return_value=types.SimpleNamespace(read_native=lambda *args,rs=changed:(manifest,rs))))
 for row_index in(0,1,2):
  for kind in('bool','float','negative','overflow','missing','extra','wrongword'):
   rows=copy.deepcopy(parsed)
   if kind=='missing':rows[row_index].pop('result')
   elif kind=='extra':rows[row_index]['extra']=0
   else:rows[row_index]['words'][0]={'bool':False,'float':0.0,'negative':-1,'overflow':65536,'wrongword':rows[row_index]['words'][0]^1}[kind]
   text='\n'.join(json.dumps(r)for r in rows)+'\n'
   check(f'output {row_index} {kind}',patch.object(v.subprocess,'run',return_value=subprocess.CompletedProcess([],0,text,'')))
 for name,code,stdout,stderr in(('stderr',0,run.stdout,'ERROR\n'),('blankstderr',0,run.stdout,'\n'),('boolstatus',False,run.stdout,''),('floatstatus',0.0,run.stdout,''),('missingrow',0,'\n'.join(run.stdout.splitlines()[:-1])+'\n',''),('extrarow',0,run.stdout+run.stdout.splitlines()[0]+'\n',''),('duplicateresult',0,run.stdout.replace('"result":','"result":0,"result":',1),'')):
  check(name,patch.object(v.subprocess,'run',return_value=subprocess.CompletedProcess([],code,stdout,stderr)))
 report=dict(passed=all(c['rejected']for c in checks),test_sha256=v.sha(__file__),reference_sha256=v.sha(reference.__file__),verifier_sha256=v.sha(v.__file__),module_sha256=v.sha(v.ROOT/'src/nba_period_entry_prefix.c'),probe_sha256=v.sha(exe),source_only_cases=cases,source_only_words=sum(c['compared_words']for c in cases),mutations=checks)
 (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(passed=report['passed'],source_cases=len(cases),source_words=report['source_only_words'],mutations=len(checks),accepted=[c for c in checks if not c['rejected']])))
 if not report['passed']:raise SystemExit(1)
if __name__=='__main__':main()
