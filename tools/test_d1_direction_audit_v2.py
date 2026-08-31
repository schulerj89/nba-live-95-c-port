import argparse,hashlib,json,struct,subprocess,itertools,random
from pathlib import Path
from d1_direction_rom_reference_v2 import caller,facing,PCS,set_rom

p=argparse.ArgumentParser();p.add_argument('--expected-mismatches',type=int,default=0);p.add_argument('--rom',type=Path,required=True);p.add_argument('--exe',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();assert a.expected_mismatches>=0;set_rom(a.rom);a.output.mkdir(exist_ok=False)
cases=[];expected=bytearray()
def direct(cur,mode,status,upper,anchor,valid,dx,dy):
 # Reconstruct a source route producing the selected direct candidate. The
 # mode8 branch overrides all candidates; others here use resolved mode10.
 v=caller(actor=0,current=cur,mode=8 if mode==8 else 10 if valid else 0,status=status,upper=upper,anchor=anchor,receiver=65535,camera=0,ax=0,ay=0,tx=0,ty=0,bx=dx,by=dy)
 cases.append((0,cur,mode,status,upper,anchor,int(valid),dx&65535,dy&65535,0,0,0,0,0,0,0));expected.append(v)
for anchor in range(65536):
 for cur in [0,3,7]:direct(cur,0,0,20,anchor,False,0,0)
edges=[0,1,2,3,7,8,32767,32768,32769,65533,65534,65535]
for dx,dy,cur in itertools.product(edges,edges,range(8)):direct(cur,10,0,0,0,True,dx,dy)
r=random.Random(0xF02D)
for _ in range(12000):direct(r.randrange(8),r.choice([0,8,10,14,15,255]),r.randrange(65536),r.choice([0,20,21,65535]),r.randrange(65536),True,r.randrange(65536),r.randrange(65536))
def real(cur,mode,status,upper,anchor,index,camera,poss,receiver,ax,ay,bx,by,tx,ty):
 if receiver==index:tx,ty=ax,ay
 v=caller(actor=index,current=cur,mode=mode,status=status,upper=upper,anchor=anchor,receiver=receiver,camera=camera,ax=ax,ay=ay,bx=bx,by=by,tx=tx,ty=ty)
 cases.append((1,cur,mode,status,upper,anchor,index,camera,poss&65535,receiver&65535,ax&65535,ay&65535,bx&65535,by&65535,tx&65535,ty&65535));expected.append(v)
for cur,mode,status,upper,anchor,index,match,poss in itertools.product(range(8),[8,10,14,15],[0,8,16,24],[0,20,21],[0,6,16,255],[0,9],[False,True],[0,9,65535]):
 real(cur,mode,status,upper,anchor,index,0x34eb+index*256 if match else 0,poss,1,0,0,0,1,20,-3)
for _ in range(12000):
 index=r.randrange(10);receiver=r.choice([65535,*range(10)])
 real(r.randrange(8),r.choice([0,8,10,14,15,255]),r.randrange(65536),r.choice([0,20,21,255]),r.randrange(256),index,r.choice([0,0x34eb+index*256,0x3eeb]),r.choice([65535,*range(10)]),receiver,*[r.randrange(65536)for _ in range(6)])
payload=b''.join(struct.pack('<16H',*c)for c in cases)
(a.output/'input.bin').write_bytes(payload);(a.output/'expected.bin').write_bytes(expected)
run=subprocess.run([str(a.exe.resolve())],input=payload,capture_output=True)
(a.output/'actual.bin').write_bytes(run.stdout);(a.output/'stderr.bin').write_bytes(run.stderr)
assert type(run.returncode)is int and run.returncode==0 and run.stderr==b'' and len(run.stdout)==len(expected)
bad=[];count=0
for i,(x,y)in enumerate(zip(run.stdout,expected)):
 assert x<8
 if x!=y:
  count+=1
  if len(bad)<12:bad.append({'i':i,'words':cases[i],'actual':x,'original':y})
report={'cases':len(cases),'mismatches':count,'expected_mismatches':a.expected_mismatches,'passed':count==a.expected_mismatches,'first':bad,'original_pcs':[hex(x)for x in sorted(PCS)],'exe_sha256':hashlib.sha256(a.exe.read_bytes()).hexdigest(),'source_only':True}
(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k!='original_pcs'},indent=2))
assert count==a.expected_mismatches
