"""Compare base, optional-asset fallback, and literal-pose gameplay runs."""
import argparse,hashlib,json,os,subprocess
from pathlib import Path
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def run(exe,rom,pack,out,tag,frames):
 trace=out/f'{tag}.jsonl';bmp=out/f'{tag}.bmp'
 command=[str(exe),'--rom',str(rom),'--assets',str(pack),'--headless','--tipoff-only','--frames',str(frames),'--gameplay-trace',str(trace),'--dump-frame',str(bmp)]
 env={k:v for k,v in os.environ.items()if not k.startswith('NBA95')}
 r=subprocess.run(command,capture_output=True,env=env,creationflags=subprocess.CREATE_NO_WINDOW)
 (out/f'{tag}.stdout.txt').write_bytes(r.stdout);(out/f'{tag}.stderr.txt').write_bytes(r.stderr)
 assert r.returncode==0 and trace.exists()and bmp.exists();return command,trace,bmp
def main():
 p=argparse.ArgumentParser(description=__doc__)
 for n in('base-exe','exe','rom','base-pack','pack','output'):p.add_argument('--'+n,type=Path,required=True)
 p.add_argument('--frames',type=int,default=390);a=p.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
 base=run(a.base_exe,a.rom,a.base_pack,out,'base',a.frames)
 fallback=run(a.exe,a.rom,a.base_pack,out,'fallback',a.frames)
 literal=run(a.exe,a.rom,a.pack,out,'literal',a.frames)
 assert base[1].read_bytes()==fallback[1].read_bytes()==literal[1].read_bytes()
 assert base[2].read_bytes()==fallback[2].read_bytes()
 A=base[2].read_bytes();B=literal[2].read_bytes();assert len(A)==len(B)and A[:54]==B[:54]
 rgb=sum(x!=y for x,y in zip(A[54:],B[54:]));assert 0<rgb<4096
 rows=base[1].read_bytes().count(b'\n');assert rows==a.frames
 report={'passed':True,'frames':a.frames,'gameplay_rows':rows,'gameplay_trace_sha256':sha(base[1]),'base_exe_sha256':sha(a.base_exe),'exe_sha256':sha(a.exe),'base_pack_sha256':sha(a.base_pack),'pack_sha256':sha(a.pack),'base_fallback_image_sha256':sha(base[2]),'literal_image_sha256':sha(literal[2]),'literal_rgb_byte_differences':rgb,'commands':[base[0],fallback[0],literal[0]],'limits':['Exact exported gameplay state, including ball world/screen/velocity/owner/state fields, is unchanged for this 390-frame natural CPU journey.','Pixel difference proves the optional resource activates the literal compositor; it is not a native OAM/scanout golden.','No graphics queue, NMI, allocation, human-control or full-game completion claim.']}
 (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(f'PASS: {rows} exact state rows; fallback identical; literal RGB bytes changed={rgb}')
if __name__=='__main__':main()
