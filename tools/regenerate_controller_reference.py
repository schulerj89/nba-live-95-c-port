"""Fresh original-byte, bounded recomp and Ghidra controller references."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys


def main():
    p=argparse.ArgumentParser()
    p.add_argument('--rom',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--recompiler',type=Path,required=True)
    p.add_argument('--ghidra',type=Path,required=True)
    p.add_argument('--jdk',type=Path,required=True)
    a=p.parse_args(); out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
    rom=a.rom.read_bytes()
    sha=lambda b:hashlib.sha256(b).hexdigest()
    if sha(rom)!='2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870':
        raise ValueError('unexpected ROM')
    sys.path.insert(0,str(a.recompiler.resolve()))
    from v2.emit_bank import BankEntry,emit_bank
    ranges={0x84:[('HumanAction',0xe2ac,0xe3ea)],
            0x85:[('PublishControllerInput',0xef3a,0xefed)],
            0x86:[('InitializeControllers',0xe208,0xe24c),('AllocateControllers',0xe24c,0xe38a),
                  ('TransferController',0xbc9b,0xbd1f),('AcquireControllerPrefix',0xd25a,0xd34a)],
            0x87:[('ResetControllerSweepSlice',0x9075,0x9093)]}
    manifest=dict(rom_sha256=sha(rom),ranges={},commands=[],artifacts={})
    script=Path(__file__).parent/'ghidra'/'DumpControllerContract.java'
    shutil.copyfile(Path(__file__),out/Path(__file__).name)
    shutil.copyfile(script,out/script.name)
    environment=os.environ.copy();environment['JAVA_HOME']=str(a.jdk)
    project=out/'project';project.mkdir()
    for bank in [0x81,0x84,0x85,0x86,0x87]:
        raw=rom[(bank&0x7f)*0x8000:((bank&0x7f)+1)*0x8000]
        binary=out/f'bank{bank:02x}.bin';binary.write_bytes(raw)
        if bank in ranges:
            entries=[BankEntry(name,first,last,0,0) for name,first,last in ranges[bank]]
            (out/f'controller_bank{bank:02x}.c').write_text(emit_bank(rom,bank,entries))
            for name,first,last in ranges[bank]:
                data=raw[first-0x8000:last-0x8000]
                manifest['ranges'][name]=dict(first=f'{bank:02X}:{first:04X}',last_exclusive=f'{bank:02X}:{last:04X}',
                                               bytes=data.hex(),sha256=sha(data))
        command=[str(a.ghidra),str(project),'controller', '-import',str(binary),'-processor','65816:LE:16:default',
                 '-loader','BinaryLoader','-loader-baseAddr','0x8000','-noanalysis','-scriptPath',str(out),
                 '-postScript',script.name,str(out),f'{bank:02x}']
        with (out/f'ghidra{bank:02x}.log').open('w') as log:
            run=subprocess.run(command,env=environment,stdout=log,stderr=subprocess.STDOUT,timeout=120)
        manifest['commands'].append(dict(command=command,exit_code=run.returncode))
        if run.returncode or not (out/f'controller_contract_bank{bank:02x}.txt').is_file():
            raise RuntimeError('Ghidra reference failed; preserve output')
    for f in out.iterdir():
        if f.is_file(): manifest['artifacts'][f.name]=dict(bytes=f.stat().st_size,sha256=sha(f.read_bytes()))
    (out/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print(out)


if __name__=='__main__':main()
