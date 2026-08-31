"""Independent malformed build/stdio views against the closed probe contract."""
import argparse,copy,json,struct,subprocess
from pathlib import Path
from unittest.mock import patch
import test_sprite_pose as verifier
def main():
 p=argparse.ArgumentParser(description=__doc__)
 for name in('exe','pack','positive','output'):p.add_argument('--'+name,type=Path,required=True)
 a=p.parse_args();exe=a.exe.resolve();pack=a.pack.resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
 inputs=list(struct.iter_unpack('<11H',(a.positive/'native.input.bin').read_bytes()))
 stdout=(a.positive/'native.output.bin').read_bytes();stderr=(a.positive/'native.stderr.txt').read_bytes()
 results=[]
 def refuses(name,call):
  try:call()
  except(AssertionError,KeyError,ValueError):results.append({'case':name,'rejected':True});return
  results.append({'case':name,'rejected':False});raise AssertionError(name)
 cases=[('extra_stdout',0,stdout+b'\0',stderr),('missing_word',0,stdout[:-2],stderr),('empty_stdout',0,b'',stderr),('extra_stderr',0,stdout,stderr+b'ERROR\r\n'),('empty_stderr',0,stdout,b''),('wrong_pack',0,stdout,stderr.replace(b'nba95_assets',b'forged_assets')),('nonzero_exit',1,stdout,stderr),('bool_exit',False,stdout,stderr)]
 for name,code,so,se in cases:
  fake=subprocess.CompletedProcess([],code,so,se)
  with patch.object(verifier.subprocess,'run',return_value=fake):refuses(name,lambda:verifier.invoke(exe,pack,'pose',inputs,out,name))
 original=json.loads(exe.with_name('manifest.json').read_text())
 mutations=[('schema_bool',lambda m:m.update(schema=True)),('wrong_baseline',lambda m:m.update(baseline=0)),('missing_source',lambda m:m['source_and_headers'].pop(next(iter(m['source_and_headers'])))),('missing_object',lambda m:m['objects'].pop(next(iter(m['objects'])))),('count_bool',lambda m:m.update(translation_units=True)),('wrong_exe',lambda m:m.update(exe_sha256='0'*64)),('extra_key',lambda m:m.update(extra=0)),('missing_manifest_field',lambda m:m.pop('base_commit'))]
 for name,mutation in mutations:
  m=copy.deepcopy(original);mutation(m)
  with patch.object(verifier.json,'loads',return_value=m):refuses(name,lambda:verifier.attest_build(exe,False))
 verifier.attest_build(exe,False)
 (out/'report.json').write_text(json.dumps({'passed':True,'cases':results,'limits':'In-memory protocol/build mutations only; immutable native capture and source files untouched. Not a general verifier certification.'},indent=2)+'\n');print(f'{len(results)} malformed protocol/build views rejected')
if __name__=='__main__':main()
