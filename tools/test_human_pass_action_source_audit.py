"""Literal AC50/B00B edge witnesses, independent of captured expected values."""
import argparse
import json
from pathlib import Path
import subprocess


def main():
    p = argparse.ArgumentParser()
    for key in ('probe', 'assets', 'output'):
        p.add_argument('--' + key, type=Path, required=True)
    a = p.parse_args()
    assert not a.output.exists()
    # mode, relative, Z, vertical speed, profile byte, lower, distance,
    # boost, planar X/Y, installed upper, upper lock.
    base = [0, 3, 0, 0, 0x54, 0, 0xf0, 0, 0, 0, 5, 0]
    cases = []
    def add(name, changes, route, installed=None, descriptor_kept=False):
        values = base.copy()
        for slot, value in changes.items(): values[slot] = value
        cases.append(dict(name=name, values=values, route=route,
                          installed=installed, descriptor_kept=descriptor_kept))
    for rel in (0, 1, 2, 6, 7, 8, 0x7fff, 0x8000, 0xffff):
        add('relative offaxis ' + hex(rel), {1:rel}, 0)
    for rel in (3, 4, 5): add('relative grounded ' + str(rel), {1:rel}, 3, 0x2f)
    for field in (2, 3):
        for value in (1, 0x8000, 0xffff):
            add('airborne nonzero ' + str((field,value)), {field:value}, 1)
    for profile in (0, 0x54):
        add('profile forces grounded ' + hex(profile), {4:profile,6:0x119,7:1,8:1}, 3, 0x2c)
    for lower in (9, 11):
        add('lower forces grounded ' + str(lower), {4:0x55,5:lower,6:0x119,7:1,8:1}, 3, 0x2c)
    add('stationary profile threshold', {4:0x55}, 3, 0x2f)
    add('moving profile threshold', {4:0x55,8:1}, 1)
    add('moving negative velocity', {4:0xff,9:0x8000}, 1)
    add('boost below threshold moving', {4:0x55,6:0x118,7:1,8:1}, 1)
    for boost in (1, 0x8000, 0xffff):
        add('boost takes precedence ' + hex(boost), {4:0x55,6:0x119,7:boost,8:1}, 2)
    add('zero boost still stationary', {4:0x55,6:0xffff,7:0}, 3, 0x2c)
    for distance in (0, 0xf0, 0xf1, 0x7fff, 0x8000, 0xffff):
        add('ground fullword distance ' + hex(distance), {0:1,6:distance,8:0xffff,9:0x8000}, 3,
            0x2f if distance < 0xf1 else 0x2c)
    add('same first request leaves lock zero before second', {0:1,6:0xf1,10:0x2c}, 3, 0x2f)
    add('negative existing lock rejects both', {0:1,6:0xf1,11:0x8000}, 3, 5, True)
    result = subprocess.run([str(a.probe.resolve()),str(a.assets.resolve())],
        input='\n'.join(' '.join(f'{v:x}' for v in c['values'])for c in cases)+'\n',
        text=True,capture_output=True,check=True)
    lines=result.stdout.splitlines()
    assert len(lines)==len(cases)+1 and lines[0].startswith('[ASSETS] Loaded asset pack:')
    for c,line in zip(cases,lines[1:]):
        values=[int(x)for x in line.split()]
        assert len(values)==14 and values[0]==c['route'],(c,values)
        if c['route']!=3: assert values[1]==0,(c,values)
        else:
            assert values[1:11]==[1,0x50,5,0x8007,6,0,0,0,0x2f,c['installed']],(c,values)
            if c['descriptor_kept']: assert values[11:]==[0x8000,0x1234,0xabcd],(c,values)
            else: assert values[11]==0xffff and values[13]==0x84,(c,values)
        c['actual']=values
    a.output.write_text(json.dumps(dict(passed=True,cases=cases),indent=2)+'\n')
    print('PASS:',len(cases),'literal source gate/grounded witnesses')


if __name__=='__main__':main()
