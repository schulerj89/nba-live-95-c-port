"""Capture the real port's first pass with source/executable/asset identities."""
import argparse,hashlib,json,os,subprocess
from pathlib import Path
from PIL import Image
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--exe',required=True,type=Path);p.add_argument('--rom',required=True,type=Path);p.add_argument('--pack',required=True,type=Path);p.add_argument('--output',required=True,type=Path);a=p.parse_args()
 out=a.output.resolve();out.mkdir(parents=True,exist_ok=False);exe=a.exe.resolve();rom=a.rom.resolve();pack=a.pack.resolve()
 assert sha(rom)=='2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'and sha(pack)=='f564c29612928984002ed3f0389d317de639fff122baf61a7bc9ecaef2a6be09'
 cmd=[str(exe),'--headless','--rom',str(rom),'--assets',str(pack),'--tipoff-only','--tipoff-clock','43200','--frames','390','--gameplay-trace',str(out/'trace.jsonl'),'--dump-sequence-dir',str(out),'--dump-sequence-from','275','--debug-state']
 env={k:v for k,v in os.environ.items()if not k.startswith('NBA95')}
 r=subprocess.run(cmd,capture_output=True,env=env,creationflags=subprocess.CREATE_NO_WINDOW)
 (out/'stdout.txt').write_bytes(r.stdout);(out/'stderr.txt').write_bytes(r.stderr);assert r.returncode==0
 stages={304:'before',306:'windup',308:'windup-held',312:'raise',318:'last-attached',320:'release',332:'flight',346:'receiver',350:'catch'}
 assert len(list(out.glob('frame_*.bmp')))==116
 for frame,name in stages.items():Image.open(out/f'frame_{frame:04d}.bmp').convert('RGB').save(out/f'{frame:04d}-{name}.png')
 root=Path(__file__).resolve().parents[1]
 m={'command':cmd,'exit_code':r.returncode,'source_commit':subprocess.check_output(['git','rev-parse','HEAD'],cwd=root).decode().strip(),'dirty_status':subprocess.check_output(['git','status','--short'],cwd=root).decode(),'exe_sha256':sha(exe),'build_manifest_sha256':sha(exe.parent/'manifest.json'),'rom_sha256':sha(rom),'pack_sha256':sha(pack),'stages':stages,'scope':'Actual current private port rendering, normal neutral CPU updates; no altered image pixels or physics. Candidate unreviewed until independent acceptance.','artifacts':{q.name:{'size':q.stat().st_size,'sha256':sha(q)}for q in out.iterdir()if q.is_file()}}
 (out/'manifest.json').write_text(json.dumps(m,indent=2)+'\n');print(out)
if __name__=='__main__':main()
