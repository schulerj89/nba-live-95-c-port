"""Independent draw-order case selection against reviewed frozen ROM diagnostic.

This deliberately reuses the reviewed original-byte executor; it does not
claim a second independent CPU implementation or normal gameplay timing.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import random
import struct
import subprocess

def load(name,path):
    spec=importlib.util.spec_from_file_location(name,path)
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    return module

def main():
    p=argparse.ArgumentParser()
    for k in ('rom','exe','tools','output'):p.add_argument('--'+k,type=Path,required=True)
    a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
    ref=a.tools/'draw_order_rom_reference.py'
    assert hashlib.sha256(ref.read_bytes()).hexdigest()=='0e074abbcb213e586df21db8f39f4d749ca77c5d2713ad476e96895a34626dbb'
    original=load('draw_original',ref).original;rom=a.rom.read_bytes()
    assert hashlib.sha256(rom).hexdigest()=='2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
    identity=[0x34eb+256*i for i in range(12)]
    records=[];expected=[];pcs=set();instructions=0;differences=set()
    randomizer=random.Random(0xA3CEFC93)
    def add(op,order,depth,xs,ys,camera):
        nonlocal instructions
        r=original(rom,op,order,depth,xs,ys,camera)
        records.append(b'DOR1'+struct.pack('<50H',op,*order,*depth,*xs,*ys,camera))
        expected.append((r['order'],r['depth']));pcs.update(r['pcs']);instructions+=r['steps']
        return r
    # Every possible wrapped Y-X word is actually executed, including all
    # negative remainders. Vary record order, camera and untranslated depths.
    for base in range(0,65536,12):
        ds=[(base+i)&65535 for i in range(12)];differences.update(ds)
        xs=[randomizer.randrange(65536) for _ in range(12)]
        ys=[(x+d)&65535 for x,d in zip(xs,ds)]
        camera=[0,0x7fff,0x8000,0xffff][(base//12)%4]
        r=add(3,randomizer.sample(identity,12),[0xbe00+i for i in range(12)],xs,ys,camera)
        # Separate arithmetic cross-check of the original CMP/ROR sequence.
        assert r['depth']==[(((d if d<32768 else d-65536)//4)-camera)&65535 for d in ds]
    # Persistent original/C operations with newly changing inputs. No native
    # afterstate is used. Include initialization amid carried runs and ties.
    order=identity[::-1];depth=[0xa000+i for i in range(12)]
    for step in range(2048):
        xs=[randomizer.randrange(65536) for _ in range(12)]
        ys=xs[:] if step%17==0 else [randomizer.randrange(65536) for _ in range(12)]
        op=0 if step%127==0 else 2 if step%3==0 else 3
        r=add(op,order,depth,xs,ys,randomizer.randrange(65536))
        order,depth=r['order'],r['depth']
    # Literal single-pass witnesses: smallest key moves left all11places;
    # the largest key moves right by one place; equal keys never move.
    for ds in ([7]*12,list(range(12)),[0x8000,0]+[0x7fff]*10):
        add(2,identity[::-1],ds,[0]*12,[0]*12,0)
    path=a.output/'cases.input';path.write_bytes(b''.join(records))
    run=subprocess.run([str(a.exe.resolve()),str(path.resolve())],capture_output=True)
    assert type(run.returncode)is int and run.returncode==0 and run.stderr==b''
    (a.output/'cases.jsonl').write_bytes(run.stdout)
    rows=[json.loads(x) for x in run.stdout.splitlines()]
    assert len(rows)==len(expected)
    for i,(row,want) in enumerate(zip(rows,expected)):
        assert row==dict(index=i+1,operation=struct.unpack_from('<H',records[i],4)[0],ok=True,order=want[0],depth=want[1]),i
    report=dict(passed=True,cases=len(rows),words=len(rows)*24,all_wrapped_differences=len(differences),
        chained_reference_cases=2048,instruction_decisions=instructions,visited_pcs=sorted(pcs),
        reference_sha256=hashlib.sha256(ref.read_bytes()).hexdigest(),
        executable_sha256=hashlib.sha256(a.exe.read_bytes()).hexdigest(),
        scope='Independent exhaustive difference and persistent case selection against reviewed actual-ROM executor; mathematical shift cross-check; no additional natural reachability or timing claim.')
    (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k!='visited_pcs'}))

if __name__=='__main__':main()
