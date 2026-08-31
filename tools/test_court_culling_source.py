"""Culling result against bounded original instructions, not native reachability.

The test-only executor starts at the already-selected player culling branch.
It stops before the human indicator or the CPU sentinel store. It verifies the
helper's Boolean result, not OAM, the indicator, CPU registers or elapsed time.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess

ROM_SHA = '2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'


class OriginalGate:
    def __init__(self, rom):
        assert hashlib.sha256(rom).hexdigest() == ROM_SHA
        self.rom = rom
        self.pcs = set()
        self.stops = {0xa405:0, 0xa42f:0, 0xa435:1}

    def byte(self, pc):
        assert 0xa3ee <= pc < 0xa42f
        return self.rom[7*0x8000+(pc&0x7fff)]

    def run(self, sx, depth, z, human):
        a, pc, carry, negative = sx, 0xa3ee if human else 0xa40c, False, False
        for _ in range(24):
            if pc in self.stops:
                return self.stops[pc]
            self.pcs.add(pc)
            op = self.byte(pc)
            if op in (0xc9, 0xbd, 0xfd):
                value = self.byte(pc+1) | self.byte(pc+2)<<8
                pc += 3
                if op == 0xc9:
                    negative = bool((a-value)&0x8000)
                    carry = a >= value
                elif op == 0xbd:
                    assert value == 0x68
                    a = depth
                    negative = bool(a&0x8000)
                else:
                    assert value == 0x0c
                    result = a-z-int(not carry)
                    a, carry, negative = result&0xffff, result>=0, bool(result&0x8000)
            elif op == 0x38:
                carry = True
                pc += 1
            elif op in (0x10, 0x30):
                offset = self.byte(pc+1)
                pc += 2
                if (not negative) if op == 0x10 else negative:
                    pc += offset-256 if offset&128 else offset
            else:
                raise AssertionError((hex(pc), hex(op)))
        raise AssertionError('Original bounded culling gate did not terminate')


def cases():
    for human in (0, 1):
        for sx in range(65536):
            yield sx, 100, 0, human
    for z in (0, 1, 20, 32767, 32768, 65535):
        for depth in range(65536):
            yield 100, depth, z, 0
    for depth in range(65536):
        yield 100, depth, 0, 1
    for depth in (65515, 65516, 287, 288, 32767, 32768):
        for z in range(65536):
            yield 100, depth, z, 0


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom', required=True, type=Path)
    parser.add_argument('--exe', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    original = OriginalGate(args.rom.read_bytes())
    expected, inputs = bytearray(), bytearray()
    for case in cases():
        inputs.extend(struct.pack('<4H', *case))
        expected.append(original.run(*case))
    (out/'input.bin').write_bytes(inputs)
    (out/'expected.bin').write_bytes(expected)
    command = [str(args.exe.resolve())]
    env = {k:v for k,v in os.environ.items() if not k.startswith('NBA95')}
    run = subprocess.run(command, input=inputs, capture_output=True, env=env,
                         creationflags=subprocess.CREATE_NO_WINDOW)
    (out/'actual.bin').write_bytes(run.stdout)
    (out/'stderr.txt').write_bytes(run.stderr)
    assert run.returncode == 0 and run.stderr == b''
    assert len(run.stdout) == len(expected) == 983040
    mismatches = []
    count = 0
    for i, (actual, wanted) in enumerate(zip(run.stdout, expected)):
        assert actual in (0, 1)
        if actual != wanted:
            count += 1
            if len(mismatches) < 12:
                mismatches.append({'case':list(struct.unpack_from('<4H', inputs, i*8)),
                                   'actual':actual, 'original':wanted, 'index':i})
    protocol = []
    for data, code in [(b'\x00',2), (bytes(7),2), (struct.pack('<4H',0,0,0,2),3)]:
        bad = subprocess.run(command, input=data, capture_output=True, env=env,
                             creationflags=subprocess.CREATE_NO_WINDOW)
        assert bad.returncode == code and bad.stdout == b'' and bad.stderr == b''
        protocol.append({'input_hex':data.hex(),'exit_code':code})
    report = {'scope':'source-only culling Boolean; no natural edge reachability, indicator/OAM/CPU/timing parity',
              'command':command,'exe_sha256':sha(args.exe),'rom_sha256':sha(args.rom),
              'cases':len(expected),'mismatches':count,'first_mismatches':mismatches,
              'original_pcs':[f'87:{pc:04X}' for pc in sorted(original.pcs)],
              'protocol_rejections':protocol,'passed':count==0,
              'artifacts':{p.name:sha(p) for p in out.iterdir() if p.is_file()}}
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))
    assert count == 0


if __name__ == '__main__':
    main()
