#!/usr/bin/env python3
"""Replay `$87:9B30-$9B37` native held-input witnesses in production C."""
import argparse
import json
import struct
import subprocess
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
FIXTURE=ROOT/'tests'/'fixtures'/'controller-held-sampler-witnesses.json'

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--probe',required=True,type=Path)
    parser.add_argument('--pack',required=True,type=Path)
    args=parser.parse_args()
    data=json.loads(FIXTURE.read_text())
    assert data['schema']==1 and data['routine']=='87:9B30-9B37'
    assert data['rom_sha256']=='2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
    assert data['capture_sha256']=='178ba0847f0e69b41ee24ad30a2ca9fdbbef80948f37ee5399edbc76f39a6bbd'
    assert data['capture_method']==(
        'One untouched production $87:915D caller, followed by five controlled genuine '
        '$87:9B30 entries. Controlled cases seed only 16-bit A and input words '
        '$0576-$057F; PC, SP, return address, ROM, and output are untouched.')
    cases=data['cases']
    required={'controlled','pad','caller_actor','caller_pad','inputs','entry_sp',
              'exit_sp','exit_x','held'}
    assert len(cases)==6 and all(set(row)==required for row in cases)
    assert [row['controlled'] for row in cases]==[False,True,True,True,True,True]
    assert [row['pad'] for row in cases]==[0,0,1,2,3,4]
    assert [row['caller_actor'] for row in cases]==[0,0,4,4,3,3]
    assert [row['caller_pad'] for row in cases]==[0,0,0,0,0,0]
    assert cases[0]['inputs']==[0,15,15,15,15]
    assert all(row['inputs']==[33059,17767,35243,52719,4951]
               for row in cases[1:])
    assert all(row['entry_sp']==8185 and row['exit_sp']==8185 for row in cases)
    assert all(row['exit_x']==2*row['pad'] for row in cases)
    assert all(row['held']==row['inputs'][row['pad']] for row in cases)
    payload=b''.join(struct.pack('<6H',row['pad'],*row['inputs']) for row in cases)
    replay=subprocess.run([str(args.probe),str(args.pack)],input=payload,
                          stdout=subprocess.PIPE,check=True)
    outputs=list(struct.unpack('<'+('H'*len(cases)),replay.stdout))
    expected=[row['held'] for row in cases]
    assert outputs==expected,(outputs,expected)
    subprocess.run([str(args.probe),str(args.pack),'--caller-test'],check=True)
    print('[CONTROLLER HELD SAMPLER] PASS: 6 native calls, pads 0..4, invalid boundary, production caller')

if __name__=='__main__':
    main()
