"""Protocol/domain checks for the controlled draw caller, not native coverage."""
import argparse,hashlib,json,struct,subprocess
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--exe',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(exist_ok=False)
checks=[]
def run(name,words,code=3,expected=b''):
 data=words if isinstance(words,bytes)else struct.pack('<16H',*words)
 r=subprocess.run([str(a.exe.resolve())],input=data,capture_output=True)
 assert type(r.returncode)is int and r.returncode==code and r.stdout==expected and r.stderr==b'',name
 checks.append({'name':name,'input_hex':data.hex(),'exit_code':r.returncode,'stdout_hex':r.stdout.hex()})
run('empty stream',b'',0);run('one byte',b'\0',2);run('31 bytes',bytes(31),2)
direct=[0]*16;caller=[1,0,10,0,0,0,0,0,65535,65535,0,0,0,1,0,0]
for name,index,value,base in [('unknown route',0,2,direct),('current8',1,8,direct),('mode256',2,256,direct),('valid2',6,2,direct),('actor10',6,10,caller),('upper256',4,256,caller),('anchor256',5,256,caller),('possessor10',8,10,caller),('receiver10',9,10,caller)]:
 w=base.copy();w[index]=value;run(name,w)
same=caller.copy();same[2]=15;same[9]=0;same[14]=32768;same[15]=1
run('same actor ignores independent receiver XY',same,0,b'\0')
report={'exe_sha256':hashlib.sha256(a.exe.read_bytes()).hexdigest(),'rejections':11,'positive_controls':2,'checks':checks}
(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print('11 protocol refusals and2 positive controls PASS')
