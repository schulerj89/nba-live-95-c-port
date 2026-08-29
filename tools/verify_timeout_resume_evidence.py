import argparse, hashlib, json
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser()
p.add_argument('--rom',default=r'F:\Games\SNES\NBA Live 95 (USA).sfc')
a=p.parse_args()
e=json.loads((ROOT/'tests/fixtures/timeout-resume-evidence.json').read_text())
rom=Path(a.rom).read_bytes()
assert hashlib.sha256(rom).hexdigest()==e['rom_sha256']
def off(pc): return 0x30000+(int(pc,16)-0x8000)
for pc,hexbytes in e['critical_bytes'].items():
    raw=bytes.fromhex(hexbytes);assert rom[off(pc):off(pc)+len(raw)]==raw,pc
assert hashlib.sha256(rom[off('8300'):off('849d')+1]).hexdigest()==e['timeout_path']['sha256']
assert hashlib.sha256(rom[off('849d'):off('857b')+1]).hexdigest()==e['resume_continuation']['sha256']
rows=json.loads((ROOT/'tests/fixtures/shot-state-witnesses.json').read_text())
fixed=[x for x in rows if x.get('kind')=='fixed_grant']
assert len(fixed)==1 and fixed[0]['provenance'].startswith('controlled-ROM:')
x=fixed[0]
for before,after in zip(x['input'][39:63],x['expected'][39:63]):
    assert after==min((before+0x1000)&0xffff,0x7fff)
for row in e['controlled_expectations'][:2]:
    if row['side_08d2']==0:
        assert row['left_after']==row['left_4715_before']-1 and row['right_after']==row['right_4795_before']
    else:
        assert row['right_after']==row['right_4795_before']-1 and row['left_after']==row['left_4715_before']
print('[TIMEOUT/RESUME EVIDENCE] PASS: ROM path hashes, side counters, +$1000 grant and clamp')
