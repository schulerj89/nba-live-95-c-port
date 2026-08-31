"""Original-byte-pinned C1 controlled branch contracts; not a natural witness."""
import argparse,hashlib,json,os,subprocess
from pathlib import Path
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--exe',type=Path,required=True);p.add_argument('--rom',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 out=a.output.resolve();out.mkdir(parents=True,exist_ok=False);rom=a.rom.read_bytes();assert hashlib.sha256(rom).hexdigest()=='2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
 def data(pc,n):return rom[((pc>>16)&0x7f)*0x8000+(pc&0x7fff):][:n]
 # Independent original branch/store authority, not bytes from the C module.
 for pc,h in {0x80cee7:'adf607f00a0a900349871d8df6076b',0x86c0f5:'22e7ce80290300f0034c89c1',0x86c154:'a958029d1200',0x86c15f:'a9ffff8d3e09a901008d020aa9eb3e8d1009',0x86c476:'bd5e00c90e00f005c90a00d00f9e60009e7e009e2800a9ffff8d46096b'}.items():assert data(pc,len(bytes.fromhex(h)))==bytes.fromhex(h),hex(pc)
 def rng(s):return 0x9146 if s==0 else((s<<1)&65535)^(0x1d87 if s&32768 else 0)
 env={k:v for k,v in os.environ.items()if not k.startswith('NBA95')};cmd=[str(a.exe.resolve()),'--controlled']
 r=subprocess.run(cmd,capture_output=True,env=env,creationflags=subprocess.CREATE_NO_WINDOW)
 (out/'stdout.txt').write_bytes(r.stdout);(out/'stderr.txt').write_bytes(r.stderr)
 assert r.returncode==0 and r.stderr==b''
 lines=r.stdout.decode('ascii').splitlines();assert len(lines)==9216
 failures=[];counts={'accepted':0,'kept_owner':0,'dropped_owner':0,'nonowner36':0,'receiver_only_clear':0}
 for ordinal,line in enumerate(lines):
  words=line.split();assert words[0]=='C'and len(words)==26
  v=list(map(int,words[1:]));assert all(0<=x<=65535 for x in v)
  seed,owner,boost,mode=v[:4]
  mi=ordinal%3;bb=(ordinal//3)%2;oo=(ordinal//6)%2;nn=(ordinal//12)%256+1;bank=ordinal//3072
  assert(seed,owner,boost,mode)==((0,0x3000,0xb000)[bank]+nn,oo,bb,(15,10,14)[mi])
  s=rng(seed);accepted=(s&7)==0;action=0;drop=False;receiver=4;timer=0x123;selector=0x333;vx=vy=0
  if accepted:
   if mode in(10,14):receiver=65535
   s=rng(s);hx=1600 if boost else 640;vx=hx//2+hx//8+(s&255)-128
   s=rng(s);hy=800 if boost else -320;vy=hy//2+hy//8+(s&255)-128
   mag=max(abs(vx),abs(vy))+min(abs(vx),abs(vy))//4
   action=0x35
   if boost and mag>=0x3f0:
    s=rng(s)
    if(s&15)==0:action=0x36
   if not owner:
    if action==0x36:drop=True
    else:s=rng(s);drop=(s&3)==0
   timer=174 if action==0x36 else 30;selector=0 if action==0x36 else 65535
   counts['accepted']+=1
   if not owner:counts['dropped_owner'if drop else'kept_owner']+=1
   if owner and action==0x36:counts['nonowner36']+=1
   if mode in(10,14):counts['receiver_only_clear']+=1
  expected=[seed,owner,boost,mode,int(accepted),s,65535 if drop else(9 if owner else 0),8 if accepted else mode,action,600 if action==0x36 else 0,480 if drop and action==0x36 else 73,0,receiver,0x1234,0x3eeb if drop else 0x55aa,10 if drop else 0,timer,selector,0x25,12,5,0,2,0x47,1 if drop else 0]
  if v!=expected:failures.append({'ordinal':ordinal,'actual':v,'expected':expected})
 report={'command':cmd,'exe_sha256':sha(a.exe),'rom_sha256':sha(a.rom),'cases':len(lines),'counts':counts,'mismatches':len(failures),'first_failures':failures[:12],'scope':'controlled source contracts; no natural contact reachability or whole parent parity','passed':not failures}
 (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2));assert all(counts.values())and not failures
if __name__=='__main__':main()
