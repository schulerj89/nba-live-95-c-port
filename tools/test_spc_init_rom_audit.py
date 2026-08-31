"""Small audit-only original-ROM executor, not linked into the port.

Compare instruction entry registers and ordered writes, without a timing claim.
The executable under review independently emits its own records.
"""
import argparse,hashlib,json,struct,subprocess
from pathlib import Path

def execute(rom,seed):
    pc,a,x,y,sp,ps=struct.unpack('<H5B',seed[:7]);mem=bytearray(seed[16:]);dsp=seed[15]
    entries=[];writes=[];pcs=set()
    def nz(value):return (ps&0x7d)|(value&0x80)|(2 if value==0 else 0)
    def put(addr,value):
        nonlocal dsp
        mem[addr]=value;writes.append((pc,addr,value))
        if addr==0xf2:dsp=value
    for count in range(200000):
        entries.append((pc,a,x,y,sp,ps));pcs.add(pc)
        if pc in (0x384,0x3db):break
        off=0x4687+pc-0x380;op=rom[off];arg=rom[off+1]
        assert mem[pc]==op
        length=1
        if op==0x20:ps&=0xdf
        elif op==0xcd:length=2;x=arg;ps=nz(x)
        elif op==0xbd:sp=x
        elif op==0xe8:length=2;a=arg;ps=nz(a)
        elif op==0x8d:length=2;y=arg;ps=nz(y)
        elif op==0xd4:length=2;put((arg+x)&255,a)
        elif op==0xd6:length=3;put(((arg|rom[off+2]<<8)+y)&65535,a)
        elif op==0x1d:x=(x-1)&255;ps=nz(x)
        elif op==0xdc:y=(y-1)&255;ps=nz(y)
        elif op==0xfc:y=(y+1)&255;ps=nz(y)
        elif op==0xfd:y=a;ps=nz(y)
        elif op in (0x10,0xd0,0xfe):
            length=2
            if op==0xfe:y=(y-1)&255
            taken=not(ps&128) if op==0x10 else not(ps&2) if op==0xd0 else y!=0
            if taken:length+=arg-256 if arg&128 else arg
        elif op==0x8f:length=3;put(rom[off+2],arg)
        elif op==0x80:ps|=1
        elif op==0xa4:
            length=2;operand=mem[arg];borrow=0 if ps&1 else 1;result=a-operand-borrow;new=result&255
            carry=int(result>=0);half=8 if (a&15)-(operand&15)-borrow>=0 else 0
            overflow=64 if ((a^operand)&(a^new)&128) else 0
            ps=(ps&0x34)|carry|half|overflow|(new&128)|(2 if new==0 else 0);a=new
        elif op==0xc4:length=2;put(arg,a)
        elif op==0xf8:length=2;x=mem[arg];ps=nz(x)
        elif op==0xd7:length=2;put(((mem[arg]|mem[(arg+1)&255]<<8)+y)&65535,a)
        elif op==0xab:length=2;value=(mem[arg]+1)&255;put(arg,value);ps=nz(value)
        else:raise AssertionError((hex(pc),hex(op)))
        pc=(pc+length)&65535
    else:raise AssertionError('bounded executor limit')
    final=struct.pack('<H5B',pc,a,x,y,sp,ps)+seed[7:15]+bytes([dsp])+mem
    return entries,writes,final,pcs

def main():
    p=argparse.ArgumentParser()
    for k in ('rom','exe','output'):p.add_argument('--'+k,type=Path,required=True)
    a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);rom=a.rom.read_bytes()
    assert hashlib.sha256(rom).hexdigest()=='2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
    cases=[];allpcs=set()
    vectors=[(0x380,ps) for ps in (0,0xff,0x20,0x7f)]+[(0x387,ps) for ps in (0,0xdf,0x15,0x48)]
    for i,(pc,ps) in enumerate(vectors):
        ram=bytearray(((j*73+i*31)^0xa5)&255 for j in range(65536));ram[0x380:0x870]=rom[0x4687:0x4b77]
        seed=struct.pack('<H5B',pc,0x35+i,0x67+i,0x9a+i,0xbc+i,ps)+bytes(range(0xc1,0xc9))+bytes([0xfa-i])+ram
        ip=a.output/f'{i}.input';ins=a.output/f'{i}.instructions';wp=a.output/f'{i}.writes';ep=a.output/f'{i}.output';ip.write_bytes(seed)
        r=subprocess.run([str(a.exe.resolve()),str(ip.resolve()),str(ins.resolve()),str(wp.resolve()),str(ep.resolve())],capture_output=True)
        assert r.returncode==0 and r.stderr==b'',r.stderr
        entries,writes,final,pcs=execute(rom,seed);allpcs|=pcs
        cins=[row[:-1] for row in struct.iter_unpack('<H5BQ',ins.read_bytes())]
        cw=[row[:-1] for row in struct.iter_unpack('<HHBQ',wp.read_bytes())]
        assert cins==entries and cw==writes and ep.read_bytes()==final
        if pc==0x387:
            assert final[16+0x8ff]==seed[16+0x8ff] and all(addr!=0x8ff for _,addr,_ in cw)
            assert {addr for _,addr,_ in cw if addr>=0x870}==set(range(0x870,0x8ff))|set(range(0x900,65536))
        cases.append(dict(entry=pc,initial_ps=ps,instructions=len(entries),writes=len(writes),preserved_08ff=final[16+0x8ff],endpoint_sha256=hashlib.sha256(final).hexdigest()))
    report=dict(passed=True,source_pc_count=len(allpcs),cases=cases,scope='controlled original-ROM instruction/data proof; no cycle or natural-state claim')
    (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(report)
if __name__=='__main__':main()
