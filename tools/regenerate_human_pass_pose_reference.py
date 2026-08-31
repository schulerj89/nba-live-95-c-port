"""Fresh original ROM, bounded recomp and independently seeded Ghidra references."""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path

def main():
 p=argparse.ArgumentParser()
 for name in ['rom','output','recompiler','ghidra','jdk']:p.add_argument('--'+name,type=Path,required=True)
 a=p.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
 sha=lambda v:hashlib.sha256(v).hexdigest();rom=a.rom.read_bytes()
 if sha(rom)!='2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870':raise ValueError('wrong original ROM')
 sys.path.insert(0,str(a.recompiler.resolve()));from v2.emit_bank import BankEntry,emit_bank
 entries={0x86:[('PassPoseCommit',0xaf1d,0xaf66)],0x87:[('ResolvePose',0xaec3,0xaf75),('AttachBall',0xb649,0xb66a),('AttachOffset',0xb832,0xb953)]}
 script=Path(__file__).parent/'ghidra/DumpHumanPassPose.java'
 shutil.copyfile(script,out/script.name);shutil.copyfile(__file__,out/Path(__file__).name)
 ghidra_root=a.ghidra.resolve().parent.parent
 tools=[a.ghidra.resolve(),ghidra_root/'Ghidra/application.properties',*sorted((ghidra_root/'Ghidra/Processors/65816').rglob('*'))]
 sources=[*sorted(a.recompiler.resolve().rglob('*.py')),*[v for v in tools if v.is_file()]]
 m=dict(rom_sha256=sha(rom),tool_sources={str(v):sha(v.read_bytes())for v in sources},ranges={},commands=[],artifacts={})
 env=os.environ.copy();env['JAVA_HOME']=str(a.jdk)
 project=out/'project';project.mkdir()
 for bank,items in entries.items():
  raw=rom[(bank&0x7f)*0x8000:((bank&0x7f)+1)*0x8000];binary=out/f'bank{bank:02x}.bin';binary.write_bytes(raw)
  (out/f'human_pass_pose_bank{bank:02x}.c').write_text(emit_bank(rom,bank,[BankEntry(n,s,e,0,0)for n,s,e in items]))
  for name,start,end in items:
   data=raw[start-0x8000:end-0x8000]
   m['ranges'][name]=dict(start=f'{bank:02X}:{start:04X}',end_exclusive=f'{bank:02X}:{end:04X}',bytes=data.hex(),sha256=sha(data))
  command=[str(a.ghidra),str(project),'human','-import',str(binary),'-processor','65816:LE:16:default',
   '-loader','BinaryLoader','-loader-baseAddr','0x8000','-noanalysis','-scriptPath',str(out),'-postScript',script.name,str(out),f'{bank:02x}']
  with(out/f'ghidra{bank:02x}.log').open('w')as f:run=subprocess.run(command,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=120)
  m['commands'].append(dict(args=command,exit_code=run.returncode))
  if run.returncode or not(out/f'human_pass_pose_bank{bank:02x}.txt').is_file():raise RuntimeError('Ghidra failed; retain output')
 for name,address,size in [('AnimationBank84',0x848000,0x8000),('LowerY',0xa9d86e,0x830),('LowerZ',0xa9d03e,0x830),('Upper0X',0xaca9cf,0x830),('Upper0Y',0xacb267,0x830),('Upper0Z',0xaca583,0x830),('Upper1X',0xaccc2f,0x830),('Upper1Y',0xacbf4b,0x830),('Upper1Z',0xacc397,0x830)]:
  offset=((address>>16)&0x7f)*0x8000+(address&0x7fff);data=rom[offset:offset+size]
  path=out/(name+'.bin');path.write_bytes(data)
  m['ranges'][name]=dict(start=f'{address:06X}',bytes_count=size,sha256=sha(data))
 m['artifacts']={v.name:dict(bytes=v.stat().st_size,sha256=sha(v.read_bytes()))for v in out.iterdir()if v.is_file()}
 (out/'manifest.json').write_text(json.dumps(m,indent=2)+'\n');print(out)

if __name__=='__main__':main()
