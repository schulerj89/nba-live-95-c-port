"""Exhaustive control values/enable edges against the pinned F1 hardware rules."""
import argparse,ctypes as C,hashlib,itertools,json,random
from pathlib import Path
U8=C.c_uint8
class Timer(C.Structure):_fields_=[(k,U8)for k in ('stage0','stage1','previous_stage1','stage2','output','target')]+[(k,C.c_bool)for k in ('enabled','globally_enabled')]
class Hw(C.Structure):_fields_=[('timer',Timer*3),('staged_cpu_input',U8*4),('pending_cpu_input_update',C.c_bool),('rom_enabled',C.c_bool),('aram_write_enabled',C.c_bool)]
class Bus(C.Structure):_fields_=[('aram',U8*65536),('cpu_to_spc',U8*4),('spc_to_cpu',U8*4),('dsp_address',U8)]

def main():
    p=argparse.ArgumentParser();p.add_argument('--dll',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
    dll=C.CDLL(str(a.dll.resolve()));fn=dll.nba_setup_spc_control_commit;fn.argtypes=[C.POINTER(Hw),C.POINTER(Bus),U8];fn.restype=C.c_bool
    visible=dll.nba_setup_spc_control_ipl_visible;visible.argtypes=[C.POINTER(Hw),C.c_uint16];visible.restype=C.c_bool
    assert (C.sizeof(Hw),C.sizeof(Bus))==(dll.audit_hw_size(),dll.audit_bus_size())
    rng=random.Random(0x38_4f1);ram=bytes(rng.randrange(256)for _ in range(65536));count=0
    for value,old_enables,write_enabled,pending in itertools.product(range(256),range(8),range(2),range(2)):
        hw=Hw();bus=Bus();C.memmove(bus.aram,ram,len(ram));hw.aram_write_enabled=bool(write_enabled);hw.pending_cpu_input_update=bool(pending);hw.rom_enabled=not bool(value&128)
        hw.staged_cpu_input[:]=[91,92,93,94];bus.cpu_to_spc[:]=[11,12,13,14];bus.spc_to_cpu[:]=[81,82,83,84];bus.dsp_address=250
        for j,t in enumerate(hw.timer):
            t.stage0=rng.randrange(16 if j==2 else 128);t.stage1=rng.randrange(2);t.previous_stage1=1-t.stage1
            t.stage2=rng.randrange(1,256);t.output=rng.randrange(16,256);t.target=rng.randrange(256);t.enabled=bool(old_enables&(1<<j));t.globally_enabled=bool((value>>j)&1)
        expect_hw=Hw.from_buffer_copy(hw);expect_bus=Bus.from_buffer_copy(bus)
        if write_enabled:expect_bus.aram[0xf1]=value
        for j in range(4):
            if value&(0x10 if j<2 else 0x20):expect_hw.staged_cpu_input[j]=expect_bus.cpu_to_spc[j]=0
        for j,t in enumerate(expect_hw.timer):
            if value&(1<<j) and not t.enabled:t.stage2=t.output=0
            t.enabled=bool(value&(1<<j))
        expect_hw.rom_enabled=bool(value&128)
        assert fn(C.byref(hw),C.byref(bus),value)
        assert bytes(hw)==bytes(expect_hw) and bytes(bus)==bytes(expect_bus),(value,old_enables,write_enabled,pending)
        for addr in (0,0xffbf,0xffc0,0xffff):assert visible(C.byref(hw),addr)==bool(value&128 and addr>=0xffc0)
        before=bytes(hw),bytes(bus)
        assert not fn(None,C.byref(bus),value) and not fn(C.byref(hw),None,value) and before==(bytes(hw),bytes(bus))
        count+=1
    result=dict(passed=True,cases=count,dll_sha256=hashlib.sha256(a.dll.read_bytes()).hexdigest(),scope='controlled F1 commit only; no clock advancement or normal reachability')
    (a.output/'report.json').write_text(json.dumps(result,indent=2)+'\n');print(result)
if __name__=='__main__':main()
