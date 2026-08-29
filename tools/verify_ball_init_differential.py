"""Same-entry native/C ball-prefix differential test, separate from full gameplay.

The C probe imports the explicit helper inputs. All 128 KiB of exit WRAM must
match, including bytes outside the 20-word projection (unexpected-write guard).
"""
import argparse
import json
import re
import subprocess
import tempfile
from pathlib import Path
from differential_compare import compare, digest

FIELDS = Path(__file__).with_name('ball_init_fields.def')
PLAN = ['ball.init.entry', 'ball.init.exit']


def fields(cursor):
    result = {}
    for expression, name in re.findall(r'^WORD\((.+), ([a-z0-9_]+)\)$', FIELDS.read_text(), re.M):
        address = cursor if expression == 'cursor' else cursor + 2 if expression == '(uint16_t)(cursor+2u)' else int(expression, 16)
        key = f'{address:04x}'
        if key in result:
            raise ValueError('aliased initializer fields')
        result[key] = name
    if len(result) != 20:
        raise ValueError('initializer field schema must contain exactly 20 words')
    return result


def state(raw, layout):
    if len(raw) != 0x20000:
        raise ValueError('WRAM snapshot is not 128 KiB')
    return {key: int.from_bytes(raw[int(key, 16):int(key, 16) + 2], 'little') for key in layout}


def row(raw, layout, index, frame):
    return dict(sequence=index, checkpoint=PLAN[index], outer_frame=frame,
                inputs=[0]*5, state=state(raw, layout), writers={})


def main():
    parser = argparse.ArgumentParser()
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument('--capture')
    source.add_argument('--fixture')
    parser.add_argument('--probe', required=True)
    parser.add_argument('--report', required=True)
    parser.add_argument('--export')
    args = parser.parse_args()
    layout = fields(0x34e7)
    if args.capture:
        root = Path(args.capture)
        entry = (root/'ball-init-entry.wram').read_bytes()
        expected = (root/'ball-init-exit.wram').read_bytes()
        pcs = json.loads((root/'ball-init-pcs.json').read_text())
        meta = json.loads((root/'ball-init-meta.json').read_text())
        # A different CPU addressing/operand mode invalidates this adapter.
        if meta['d'] != 0 or meta['dbr'] != 0x7e or meta['ps'] & 0x30:
            raise ValueError(f'unsupported native entry context: {meta}')
        fixture = dict(schema_sha256=digest(FIELDS), fields=layout,
                       entry=state(entry, layout), exit=state(expected, layout), executed=pcs,
                       meta=meta, source=str(root),
                       entry_sha256=digest(root/'ball-init-entry.wram'),
                       exit_sha256=digest(root/'ball-init-exit.wram'))
    else:
        fixture = json.loads(Path(args.fixture).read_text())
        if fixture['fields'] != layout or fixture['schema_sha256'] != digest(FIELDS):
            raise ValueError('fixture schema mismatch')
        pcs, meta = fixture['executed'], fixture['meta']
        # Nonzero unrepresented bytes exercise preservation; represented
        # expected outputs still come exclusively from the native witness.
        entry = bytearray((i*37+91)&255 for i in range(0x20000))
        for key, value in fixture['entry'].items():
            p=int(key,16);entry[p:p+2]=value.to_bytes(2,'little')
        expected=bytearray(entry)
        for key, value in fixture['exit'].items():
            p=int(key,16);expected[p:p+2]=value.to_bytes(2,'little')
    if len(pcs) != 30 or len(set(pcs)) != 30 or pcs[0] != 0x86e056 or pcs[-1] != 0x86e0a9:
        raise ValueError('incomplete native instruction witness')
    with tempfile.TemporaryDirectory() as temp:
        root=Path(temp);input_path=root/'entry.wram';output_path=root/'port-exit.wram'
        input_path.write_bytes(entry)
        subprocess.run([args.probe,str(input_path),str(output_path)],check=True)
        actual=output_path.read_bytes()
    rom=[row(entry,layout,0,0),row(expected,layout,1,meta['frames'])]
    port=[row(entry,layout,0,0),row(actual,layout,1,0)]
    report=compare(rom,port,0,fields=layout,checkpoint_plan=PLAN)
    mismatches=[i for i,(a,b) in enumerate(zip(expected,actual)) if a!=b]
    report.update(scope='bounded E056-E0AB same-entry helper replay, not whole-game equivalence',
                  probe_sha256=digest(args.probe), schema_sha256=digest(FIELDS),
                  native_entry_sha256=fixture['entry_sha256'], native_exit_sha256=fixture['exit_sha256'],
                  native_instruction_starts=30, full_wram_mismatches=len(mismatches),
                  first_raw_mismatch=None if not mismatches else f'{0x7e0000+mismatches[0]:06X}')
    if mismatches and report['status']=='PROJECTION_MATCH':
        report['status']='UNEXPECTED_WRITE_DIVERGENCE'
    if len(actual)!=len(expected):
        raise ValueError('probe returned an incomplete snapshot')
    Path(args.report).write_text(json.dumps(report,indent=2)+'\n')
    if args.export and report['status']=='PROJECTION_MATCH' and not mismatches:
        Path(args.export).write_text(json.dumps(fixture,indent=2)+'\n')
    print(json.dumps(report,indent=2))
    return 0 if report['status']=='PROJECTION_MATCH' and not mismatches else 1


if __name__ == '__main__':
    raise SystemExit(main())
