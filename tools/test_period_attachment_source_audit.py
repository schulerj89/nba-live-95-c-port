"""Fixed state12 CPU attachment source/ROM tables; no timing or human proof."""
import argparse,hashlib,itertools,json,struct,subprocess
from pathlib import Path
def main():
 p=argparse.ArgumentParser()
 for k in('rom','exe','pack','replay','output'):p.add_argument('--'+k,type=Path,required=True)
 a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);rom=a.rom.read_bytes();assert hashlib.sha256(rom).hexdigest()=='2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
 def rb(addr):return rom[((addr>>16)&127)*32768+(addr&32767)]
 def rw(addr):return rb(addr)|rb(addr+1)<<8
 def signed(v):return v-256 if v&128 else v
 cases=json.loads(a.replay.read_text())['cases'];seed=Path(next(c for c in cases if c['mode']=='attachment')['command'][2]).read_bytes();word=lambda b,p:struct.unpack_from('<H',b,p)[0];actor=word(seed,0x954);base=0x34eb+256*actor
 assert word(seed,base+0x16)==65535 and word(seed,0x4726+128*(actor//5))==0
 descriptors=[rw(t+24)for t in(0x84c218,0x84c28a,0x84c2fc)]
 assert [tuple(rw(0x840000+d+off)for off in(0,2,4,6))for d in descriptors]==[(0,0,512,1)]*3
 count=0;table_values=[]
 for facing,alt,flags in itertools.product(range(8),range(2),(0,1,2,3)):
  raw=bytearray(seed);put=lambda b,p,v:struct.pack_into('<H',b,p,v&65535)
  coords=[0x7fff,0x8000,0xffff,1];x,y,z=coords[count%4],coords[(count+1)%4],coords[(count+2)%4]
  for off,val in[(4,x),(8,y),(12,z),(0x4e,facing),(0xa8,alt),(0x28,flags|0x8000)]:put(raw,base+off,val)
  expected=bytearray(raw)
  # BC9B CPU no-controller early return; B538/B555 lock=0 cancellation.
  assert word(raw,base+0x46)==word(raw,base+0x48)==0
  for off,val in[(0x18,65535),(0x1a,65535),(0x30,12),(0x32,12),(0x38,12),(0x3a,0),(0x3c,0),(0x46,0),(0x48,0),(0x34,12),(0x36,12),(0x3e,0),(0x40,0),(0x52,facing)]:put(expected,base+off,val)
  upper=rw(0x840000+rw(0x840000+descriptors[2]+8+2*facing));lower=rw(0x840000+rw(0x840000+descriptors[alt]+8+2*facing))
  assert upper>=0xf0 # variant conditional is unentered for this source state.
  mirror=flags|(0x8000 if facing<3 else 0);put(expected,base+0x28,mirror);put(expected,base+0x2a,upper);put(expected,base+0x2c,lower)
  uy=signed(rb(0xacb267+upper));ly=signed(rb(0xa9d86e+lower));ux=signed(rb(0xaca9cf+upper));uz=signed(rb(0xaca583+upper));lz=signed(rb(0xa9d03e+lower));assert uy!=-128 and ly!=-128
  f=mirror^(3 if mirror&32768 else 0);ly=-ly if f&2 else ly;uy=-uy if f&1 else uy
  half=(uy+ly)//2;dx=half-2*ux;dy=half+2*ux;dz=ux-lz-uz
  put(expected,0x922,word(raw,0x3eef));put(expected,0x3eef,x+dx);put(expected,0x3ef3,y+dy);put(expected,0x3ef7,z+dz)
  inp=a.output/'controlled.input';out=a.output/'controlled.output';inp.write_bytes(raw)
  r=subprocess.run([str(a.exe.resolve()),str(a.pack.resolve()),str(inp.resolve()),'attachment',str(out.resolve())],capture_output=True);assert r.returncode==0
  actual=out.read_bytes();regions=[(0x34d3,24),(0x34eb,2816),(0x900,0x10a),(0x46eb,0x240)];diff=[(hex(p),actual[p],expected[p])for start,size in regions for p in range(start,start+size)if actual[p]!=expected[p]];assert not diff,(facing,alt,flags,diff)
  table_values.append(dict(facing=facing,alternate=alt,flags=flags,upper=upper,lower=lower,dx=dx,dy=dy,dz=dz));count+=1
 report=dict(passed=True,controlled_calls=count,state12_descriptors=[hex(d)for d in descriptors],cases=table_values,scope='CPU no-controller state12 source tables; full-word XYZ wrap; no human transfer, arbitrary pose/byte extrema, CPU or timing proof')
 (a.output/'report.json').write_text(json.dumps(report,indent=2));print('PASS',count)
if __name__=='__main__':main()
