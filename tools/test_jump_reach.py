"""Native regression, oracle-tamper checks, and independent ROM asset checks."""
import argparse,copy,json,struct,subprocess
from pathlib import Path
from differential_compare import load
from extract_assets import load_verified_rom,lorom_offset,build_jump_gameplay_asset
from verify_jump_reach import verify

ROOT=Path(__file__).resolve().parents[1]
def main():
    p=argparse.ArgumentParser()
    for name in ('pack','rom','probe'):p.add_argument('--'+name,required=True)
    a=p.parse_args();raw=Path(a.pack).read_bytes();rom=load_verified_rom(a.rom)
    count=struct.unpack_from('<I',raw,12)[0]
    entries={e[0]:e for i in range(count) for e in [struct.unpack_from('<6I',raw,16+i*24)]}
    e=entries[280]
    assert e[2:]==(184,72,16,0x86ee76)
    assert raw[e[1]:e[1]+e[2]]==build_jump_gameplay_asset(rom)
    # Independent literal ranges prevent an extractor/consumer shared mistake.
    assert raw[e[1]+8:e[1]+152]==rom[lorom_offset(0x86ee76):lorom_offset(0x86ef06)]
    assert raw[e[1]+152:e[1]+184]==rom[lorom_offset(0x85f16f):lorom_offset(0x85f18f)]
    e=entries[251]
    for i in range(348):
        start=e[1]+24+i*64;address=struct.unpack_from('<I',raw,start)[0]
        assert raw[start+29:start+31]==rom[lorom_offset(address)+0x3c:lorom_offset(address)+0x3e]
    fixture=ROOT/'tests/fixtures/jump-reach-witnesses.jsonl'
    rows=load(fixture);pcs=verify(rows,a.probe,a.pack,True)
    census=json.loads((ROOT/'tests/fixtures/jump-reach-instructions.json').read_text())
    assert pcs==census['pcs'] and len(pcs)==239
    assert any(not r['controlled'] for r in rows) and any(r['controlled'] for r in rows)
    sample=next(r for r in rows if r['calls'] and r['calls'][0][0]>=0x870000)
    bad=[]
    for i in range(4):
        r=copy.deepcopy(sample);r['output'][i]^=1;bad.append(r)
    r=copy.deepcopy(sample);r['calls'][0][1]^=1;bad.append(r)
    r=copy.deepcopy(sample);r['channels_out'][2]^=1;bad.append(r)
    r=copy.deepcopy(sample);r['input'].pop();bad.append(r)
    r=copy.deepcopy(sample);r['abi'][2]|=0x20;bad.append(r)
    for r in bad:
        try:verify([r],a.probe,a.pack,True)
        except (ValueError,KeyError):pass
        else:raise AssertionError('tampered oracle accepted')
    subprocess.run([a.probe,a.pack,'--guards'],check=True)
    print(f'[JUMP REACH] {len(rows)} native witnesses,239 starts; decision/channel projection;8 oracle-tamper guards;348 ROM rating pairs')
    print('[JUMP REACH] Parent and 82:F02F/F13D scratch producer are runtime-wired; far EAA8 child remains pending.')
if __name__=='__main__':main()
