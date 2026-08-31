"""Controlled B468 dataflow vectors using original ROM tables, not native claims."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
U=65535
def w(b,a):return struct.unpack_from('<H',b,a)[0]
def put(b,a,v):struct.pack_into('<H',b,a,v&U)
def n(a,b=0):return((a-b)&32768)!=0
def mag(a):return(-a if a&32768 else a)&U
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()

def reference(rom,before):
 """Typed source dataflow, original tables/read addresses, binary D=0 domain."""
 b=bytearray(before);actor=w(b,0x96);anchor=w(b,0x9e);original_b2=w(b,0xb2)
 def rr(a):return w(rom,((a>>16)&127)*32768+(a&32767))
 def rb(a):return rom[((a>>16)&127)*32768+(a&32767)]
 def signedbyte(a):
  v=rb(a);return v if v<128 else v-256
 def rng():
  old=w(b,0x7f6);v=0x9146 if not old else((old*2)&U)^(0x1d87 if old&32768 else 0);put(b,0x7f6,v);return v
 direction=w(b,actor+0x88)>>1;put(b,4,direction);put(b,0x4f,direction)
 variant=(rng()&3)*2;put(b,0x51,variant)
 while True:
  selector=7 if w(b,0x904) else rng()&7;put(b,0x18,selector)
  if selector==7 and direction in(0,4):put(b,0x904,0);continue
  put(b,0xba,selector*2)
  if selector==1:
   if not n(original_b2,64):put(b,0x904,0);continue
   put(b,4,w(b,4)^4)
  break
 # B7D8 frame selection uses DBR012C, then point1 offsets in A9/AC.
 upper=rr(0x86b430+selector*2);lower=rr(0x86b440+variant)
 uphase=rr(0x86b450+selector*2);lowphase=rr(0x86b460+variant)
 put(b,0x14,lowphase);put(b,0x1a,uphase);put(b,0x49,0x84)
 orient=w(b,4);mirror=U if orient<3 else 0;put(b,6,mirror);put(b,0x47,mirror)
 lo_descriptor=rr((0x84c28a if w(b,0x12c)else 0x84c218)+lower*2)
 lo_list=rr(0x840000+lo_descriptor+orient*2+8);lo=rr(0x840000+lo_list+lowphase*2)
 hi_descriptor=rr(0x84c2fc+upper*2);hi_list=rr(0x840000+hi_descriptor+orient*2+8);hi=rr(0x840000+hi_list+uphase*2)
 ly=signedbyte(0xa9d86e+lo);lz=signedbyte(0xa9d03e+lo)
 ux=signedbyte(0xaccc2f+hi);uy=signedbyte(0xacbf4b+hi);uz=signedbyte(0xacc397+hi)
 if mirror:ly=-ly;uy=-uy
 total=(ly+uy)&U;mid=(total-65536 if total&32768 else total)//2
 p0=(mid-ux*2)&U;p2=(mid+ux*2)&U;put(b,0,p0);put(b,2,p2);put(b,4,ux-lz-uz)
 # B4DF subtracts the full two-word X and negates full two-word Y.
 x=((w(b,anchor+8)|(w(b,anchor+10)<<16))-(w(b,actor+2)|(w(b,actor+4)<<16)))&0xffffffff
 y=(-(w(b,actor+6)|(w(b,actor+8)<<16)))&0xffffffff
 xl,xh=x&U,x>>16;yl,yh=y&U,y>>16;put(b,0xb4,xl);put(b,0xb8,yl)
 major,minor=mag(xh),mag(yh)
 if n(major,minor):major,minor=minor,major
 put(b,0xb6,minor>>2)
 # AA6A keeps the inherited E0/C2 identities and uses receiver+B2/+6E.
 e0=w(b,0xe0)|(b[0xe2]<<16);stats=w(b,0x3435+w(b,0xc2)*2)
 chance=((rr(e0+0x39)&255)+150-(((32767-w(b,stats+0x18))&U)>>10))&U
 if n(chance):chance=0
 if not n(w(b,actor+0xb2),3):chance=(chance+(chance>>2))&U
 if not n(w(b,0x9c0)):chance=(chance+35)&U if w(b,0x9c0)==w(b,actor+0x6e)else chance>>1
 if n(chance,5):chance=5
 elif not n(chance,255):chance=255
 if rng()&255>=chance:
  err=(direction^4)*4;xh=(xh+rr(0x86ab0d+err))&U;yh=(yh+rr(0x86ab0f+err))&U
 xh=(xh-p0)&U;yh=(yh-p2)&U
 if selector==7:yh=(yh+(-8 if n(w(b,actor+8))else 8))&U
 divisor=(w(b,actor+0x60)&255)<<8;put(b,0xb2,divisor)
 def divide(lo,hi):
  value=lo|(hi<<16);sign=0;den=divisor
  if n(hi):value=(-value)&0xffffffff;sign+=1
  if n(den):den=(-den)&U;sign-=1
  # Separate bitwise long division; do not share the C / and % expression.
  q=0;rem=value
  if den:
   for shift in range(31,-1,-1):
    chunk=den<<shift
    if rem>=chunk:rem-=chunk;q|=1<<shift
  put(b,0x806,rem);put(b,0x808,rem>>16);put(b,0x80a,den>>1);put(b,0x80c,0)
  put(b,0x80e,q);put(b,0x810,q>>16);put(b,0x824,sign);put(b,0xce,value>>16);put(b,0xd0,den)
  quotient=(-q if sign else q)&U;put(b,0xcc,quotient);return quotient,rem&U
 vx,rx=divide(xl,xh);vy,ry=divide(yl,yh);put(b,0xac,rx);put(b,0xb0,ry)
 for off in(0xe,0xba):put(b,actor+off,vx)
 for off in(0x10,0xbc):put(b,actor+off,vy)
 major,minor=mag(vx),mag(vy)
 if n(major,minor):major,minor=minor,major
 minor>>=2;major=(major+minor)&U
 put(b,0xaa,major);put(b,0xae,minor);put(b,actor+0x4c,major);put(b,actor+0x4a,major*2)
 flags=w(b,actor+0x7e)
 if rng()&0x30:flags|=1
 put(b,actor+0x7e,flags|6);put(b,actor+0x4e,direction);put(b,actor+0x50,direction)
 put(b,0x936,2);put(b,0x91c,0);put(b,actor+0x56,selector-1);put(b,actor+0x58,variant);put(b,actor+0x66,upper)
 return b

def main():
 p=argparse.ArgumentParser();p.add_argument('--exe',type=Path,required=True);p.add_argument('--pack',type=Path,required=True);p.add_argument('--rom',type=Path,required=True);p.add_argument('--before',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 out=a.output.resolve();out.mkdir(parents=True,exist_ok=False);rom=a.rom.read_bytes();assert sha(a.rom)=='2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
 seed=a.before.read_bytes();assert len(seed)==131072;actor=w(seed,0x96);vectors=[]
 # These are controlled before-state vectors, never replayed into Mesen.
 for i in range(512):
  b=bytearray(seed);put(b,0x7f6,(i*129+1)&U);put(b,actor+0x88,(i%8)*2)
  put(b,0x904,(i//8)%2);put(b,0xb2,[0,56,61,63,64,66,0x8000,0x8eca][(i//16)%8])
  put(b,0x12c,(i//128)%2);put(b,actor+0xa8,1-w(b,0x12c))
  put(b,actor+0x60,[0,1,2,40,61,255,256,0x8eca][(i//64)%8])
  put(b,actor+2,(i*397)&U);put(b,actor+6,(i*1049)&U)
  put(b,actor+4,[0,1,0x8000,0x7fff,0xffff,320,0xff00,0x4000][(i//4)%8]);put(b,actor+8,(i*317)&U)
  put(b,actor+0xb2,[0,2,3,0x8000,0xffff][i%5]);put(b,0x9c0,[0xffff,0,5][i%3])
  stats=w(b,0x3435+w(b,0xc2)*2);put(b,stats+0x18,(i*271)&U)
  vectors.append(b)
 src=out/'before.bin';src.write_bytes(struct.pack('<I',len(vectors))+b''.join(vectors))
 command=[str(a.exe.resolve()),str(a.pack.resolve()),str(a.rom.resolve()),str(src)]
 run=subprocess.run(command,env={k:v for k,v in os.environ.items()if not k.startswith('NBA95')},capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW)
 (out/'stdout.txt').write_bytes(run.stdout);(out/'stderr.txt').write_bytes(run.stderr)
 report=dict(kind='controlled ROM-table/source-dataflow comparison; no natural coverage extension',command=command,exe_sha256=sha(a.exe),input_sha256=sha(src),oracle_source_sha256=sha(Path(__file__)),rom_sha256=sha(a.rom),baseline_before_sha256=sha(a.before),exit_code=run.returncode,failures=[])
 try:
  assert run.returncode==0
  pack=a.pack.read_bytes();count=struct.unpack_from('<I',pack,12)[0]
  assert run.stderr==f"[ASSETS] Loaded asset pack: '{a.pack.resolve()}' ({len(pack)} bytes, {count} assets)\r\n".encode()
  expected=[reference(rom,b)for b in vectors];lines=run.stdout.decode('ascii').splitlines();assert len(lines)==512*54
  actor_offsets=[2,4,6,8,0x88,0x60,0xb2,0x6e,0xe,0x10,0xba,0xbc,0x4c,0x4a,0x7e,0x4e,0x50,0x56,0x58,0x66]
  global_offsets=[0x7f6,0x904,0x936,0x91c,0,2,4,6,0x14,0x18,0x1a,0x47,0x49,0x4f,0x51,0xaa,0xac,0xae,0xb0,0xb2,0xb4,0xb6,0xb8,0xba,0xcc,0xce,0xd0,0x806,0x808,0x80a,0x80c,0x80e,0x810,0x824]
  addresses=[actor+i for i in actor_offsets]+global_offsets
  coverage=set()
  for ordinal,line in enumerate(lines):
   k,idx,addr,value=line.split();idx,addr,value=map(int,(idx,addr,value));assert k=='W'and idx==ordinal//54 and addr==addresses[ordinal%54] and 0<=value<=65535
   want=w(expected[idx],addr)
   if value!=want:report['failures'].append(dict(case=idx,address=hex(addr),actual=value,expected=want))
   coverage.add(w(expected[idx],0x18))
  report.update(cases=512,compared_words=len(lines),selectors=sorted(coverage),passed=not report['failures'])
 finally:(out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps({k:v if k!='failures'else v[:20]for k,v in report.items()},indent=2));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
