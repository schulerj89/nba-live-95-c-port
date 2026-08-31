"""Fresh bounded body-resource and unchanged-state comparison of actual C journeys."""
import argparse,hashlib,json,os,re,subprocess
from pathlib import Path
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def run(exe,pack,out,label,baseline):
 exe=exe.resolve();m=json.loads((exe.parent/'manifest.json').read_text())
 assert m['baseline']is baseline and m['exe_sha256']==sha(exe)
 for name,digest in m['source_and_headers'].items():assert sha(Path(name))==digest,name
 cmd=[str(exe),str(pack)];env={k:v for k,v in os.environ.items()if not k.startswith('NBA95')}
 r=subprocess.run(cmd,env=env,capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW)
 (out/(label+'.stdout')).write_bytes(r.stdout);(out/(label+'.stderr')).write_bytes(r.stderr)
 lines=r.stdout.decode('ascii').splitlines();loader=f"[ASSETS] Loaded asset pack: '{pack}' ({pack.stat().st_size} bytes, 264 assets)"
 assert lines[0]==loader and lines[1]=='[TIPOFF] $86:CCFC contact -> $86:B04C receiver -> $86:99C4 deflection -> $86:D365 possession.'
 result=lines[-1].split();assert len(result)==6 and result[0]=='RESULT'
 frames,checked,bad,mask,rendered=map(int,result[1:]);assert frames==63800 and checked==628226 and mask==255 and rendered==116
 assert bad==(12466 if baseline else 0)and r.returncode==(10 if baseline else 0)
 states=[];failures=[]
 for line in lines[2:-1]:
  if line.startswith('BODY_FAIL '):
   f=line.split();assert len(f)==7 and all(v.isdecimal()for v in f[1:]);failures.append(line)
  else:
   assert re.fullmatch(r'STATE [1-9][0-9]* [0-9a-f]{16}',line)
   assert int(line.split()[1])==len(states)+1;states.append(line)
 assert len(states)==63800 and len(failures)==(8 if baseline else 0)
 allowed={(0x83DA12,17),(0x83EC60,17),(0x83D333,6),(0x83E9D4,39)}
 allowed_lines={f'[HUD] Untranslated original overlay child ${pc:06X} (kind={kind}); gameplay continues, no substituted panel'for pc,kind in allowed}
 errors=r.stderr.decode('ascii').splitlines();assert errors and all(line in allowed_lines for line in errors)
 return {'command':cmd,'exit_code':r.returncode,'exe_sha256':sha(exe),'manifest_sha256':sha(exe.parent/'manifest.json'),'checked_body_pairs':checked,'mismatches':bad,'pass_direction_mask':mask,'nonmutating_render_frames':rendered,'state_stream_sha256':hashlib.sha256(('\n'.join(states)+'\n').encode()).hexdigest(),'stderr_sha256':hashlib.sha256(r.stderr).hexdigest(),'first_failures':failures},states
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--candidate',required=True,type=Path);p.add_argument('--baseline',required=True,type=Path);p.add_argument('--pack',required=True,type=Path);p.add_argument('--output',required=True,type=Path);a=p.parse_args()
 out=a.output.resolve();out.mkdir(parents=True,exist_ok=False);pack=a.pack.resolve()
 assert sha(pack)=='f564c29612928984002ed3f0389d317de639fff122baf61a7bc9ecaef2a6be09'
 candidate,cs=run(a.candidate,pack,out,'candidate',False);baseline,bs=run(a.baseline,pack,out,'baseline',True)
 assert cs==bs and candidate['stderr_sha256']==baseline['stderr_sha256']
 report={'passed':True,'pack_sha256':sha(pack),'candidate':candidate,'baseline':baseline,'identical_state_frames':63800,'limits':'All telemetry except explicitly zeroed body-resource-derived appearance fields hashed per frame; not a full native gameplay/OAM/timing claim. FNV is a diagnostic state checksum, not a cryptographic attestation. No new native golden.'}
 (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
if __name__=='__main__':main()
