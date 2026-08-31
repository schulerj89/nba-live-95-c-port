"""Aligned-pass v2 evidence and diagnostic mutation tests, derived from the independent pass audit tool."""
import argparse, copy, importlib.util, json, subprocess, sys
from pathlib import Path
from unittest.mock import patch


def main():
    p = argparse.ArgumentParser()
    for key in ('verifier', 'capture', 'probe', 'rom', 'output'):
        p.add_argument('--' + key, type=Path, required=True)
    a = p.parse_args()
    for key in vars(a):
        setattr(a, key, getattr(a, key).resolve())
    a.output.mkdir(parents=True, exist_ok=False)
    sys.path.insert(0, str(a.verifier.parent))
    spec = importlib.util.spec_from_file_location('pass_verifier_audit', a.verifier)
    v = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(v)
    baseline = v.verify(a.capture, a.probe, a.rom, a.output / 'baseline.json')
    assert baseline['passed'] and baseline['calls'] > 0
    (a.output / 'baseline.json').write_text(json.dumps(baseline, indent=2) + '\n')
    manifest = v.read_json(a.capture / 'manifest.json')
    actual_read = v.read_json
    actual_text = Path.read_text
    original_rows = [json.loads(s) for s in actual_text(a.capture / 'boundaries.jsonl').splitlines()]
    stdout = (a.output / 'baseline.probe-stdout.txt').read_text()
    stderr = (a.output / 'baseline.probe-stderr.txt').read_text()
    checks = []

    def run(name, context):
        reason = ''
        try:
            with context:
                report = v.verify(a.capture, a.probe, a.rom, a.output / f'case-{len(checks):02d}.json')
            rejected = not report['passed']
            if rejected:
                reason = 'C mismatch'
        except (ValueError, KeyError, TypeError, IndexError) as error:
            rejected = True
            reason = str(error)
        checks.append(dict(name=name, rejected=rejected, reason=reason))

    def metadata(name, change):
        altered = copy.deepcopy(manifest)
        change(altered)
        run(name, patch.object(v, 'read_json', side_effect=lambda path: altered if Path(path) == a.capture / 'manifest.json' else actual_read(path)))

    for field, value in [('requested_frames', True), ('requested_frames', 399), ('selection', 1), ('schema', 1.0)]:
        metadata(f'{field}={value!r}', lambda m, k=field, x=value: m.update({k: x}))
    metadata('extra source key', lambda m: m['sources'].update(extra=m['sources']['capture']))
    metadata('wrong command ROM', lambda m: m['arguments'].__setitem__(3, str(a.output / 'wrong.sfc')))
    metadata('extra environment key', lambda m: m['environment'].update(NBA95_UNATTESTED='1'))
    metadata('boolean artifact size', lambda m: m['artifacts']['stderr.log'].update(bytes=False))
    metadata('missing raw artifact', lambda m: m['artifacts'].pop(next(k for k in m['artifacts'] if k.startswith('raw_'))))
    metadata('forged persisted settings hash', lambda m: m['isolation'].update(post_settings_sha256='0' * 64))

    metadata('omitted capture source', lambda m: m['sources'].pop('capture'))
    metadata('omitted runner source', lambda m: m['sources'].pop('runner'))
    metadata('omitted helper source', lambda m: m['sources'].pop('isolation_helper'))
    metadata('omitted boundary artifact', lambda m: m['artifacts'].pop('boundaries.jsonl'))
    metadata('wrong route environment', lambda m: m['environment'].update(NBA95_PASS_ALIGNED_SELECTION='2'))
    metadata('float process result', lambda m: m.update(exit_code=0.0))
    metadata('integer injection flag', lambda m: m.update(state_injection=0))
    metadata('integer ROM patch flag', lambda m: m.update(rom_patch=0))
    metadata('changed recorded final saves', lambda m: m['isolation'].update(final_saves={'not-native.srm':'0'*64}))

    def events(name, change):
        rows = copy.deepcopy(original_rows)
        change(rows)
        text = '\n'.join(json.dumps(row) for row in rows) + '\n'
        # Only the parsed event view is altered, after original on-disk hash checks.
        # This tests row/schema consistency without rewriting capture artifacts.
        def altered(path, *args, **kwargs):
            return text if path == a.capture / 'boundaries.jsonl' else actual_text(path, *args, **kwargs)
        run(name, patch.object(Path, 'read_text', altered))

    index = next(i for i, r in enumerate(original_rows) if r['tag'] == 'aligned.entry')
    for field, value in [('actor', True), ('direction', 1.5), ('owner', 0x10000), ('live', 0x10000), ('score', 0x10000), ('candidate', 0x10000), ('actor', 0x10000), ('direction', 0x10000)]:
        events(f'entry {field}={value!r}', lambda rows, k=field, x=value: rows[index].update({k: x}))
    for field in ('actor', 'owner', 'live', 'offense', 'candidate', 'score', 'direction'):
        events('entry raw disagreement ' + field, lambda rows, k=field: rows[index].update({k: rows[index][k] ^ 1}))
    events('all court clocks exceed completion', lambda rows: [r.update(court=r['court'] + 100000) for r in rows])
    events('all frame clocks exceed runner stop bound', lambda rows: [r.update(frame=r['frame'] + 100000) for r in rows])
    events('uniform one-frame shift', lambda rows: [r.update(frame=r['frame'] + 1) for r in rows])
    events('uniform one-court shift', lambda rows: [r.update(court=r['court'] + 1) for r in rows])
    events('missing aligned entry', lambda rows: rows.pop(next(i for i,r in enumerate(rows) if r['tag']=='aligned.entry')))
    events('missing lane stage', lambda rows: rows.pop(next(i for i,r in enumerate(rows) if r['tag']=='aligned.lane.entry')))
    events('wrong native PC', lambda rows: rows[index].update(pc=rows[index]['pc'] + 1))
    events('nonzero native DP', lambda rows: rows[index].update(cpu_d=1))
    events('duplicated native index', lambda rows: rows[index].update(index=1))

    lines = stdout.splitlines()
    pass_index = next(i for i, line in enumerate(lines) if 'route' in json.loads(line))
    first = json.loads(lines[pass_index])
    outputs = [('missing C row', '\n'.join(lines[:-1]) + '\n'), ('extra C row', stdout + lines[0] + '\n'), ('arbitrary C noise', 'noise\n' + stdout)]
    for field in ('route', 'actor_words', 'controller_words', 'context_words', 'global_words', 'dp_words', 'profile_words', 'profile_byte', 'selection_words', 'order_words'):
        altered = copy.deepcopy(first)
        if isinstance(altered[field], list):
            altered[field][0] = float(altered[field][0])
        else:
            altered[field] = float(altered[field])
        outputs.append(('float C ' + field, '\n'.join([*lines[:pass_index], json.dumps(altered), *lines[pass_index + 1:]]) + '\n'))
    lane_index = next(i for i,line in enumerate(lines) if 'obstructed' in json.loads(line))
    lane = json.loads(lines[lane_index])
    for field in ('obstructed','saved_words'):
        altered=copy.deepcopy(lane)
        if isinstance(altered[field],list):altered[field][0]=float(altered[field][0])
        else:altered[field]=float(altered[field])
        outputs.append(('float C '+field,'\n'.join([*lines[:lane_index],json.dumps(altered),*lines[lane_index+1:]])+'\n'))
    altered=copy.deepcopy(lane);altered['undeclared_output']=0
    outputs.append(('extra C field','\n'.join([*lines[:lane_index],json.dumps(altered),*lines[lane_index+1:]])+'\n'))
    for name, text in outputs:
        run(name, patch.object(v.subprocess, 'run', return_value=subprocess.CompletedProcess([str(a.probe)], 0, text, stderr)))
    diagnostic_cases = [
        ('extra error', stderr+'ERROR: unexpected native input\n'),
        ('missing loader', ''),
        ('different argument path', stderr.replace(str(v.DEFAULT_ASSETS),'other-pack.pak')),
        ('wrong byte count', stderr.replace('89438786 bytes','89438785 bytes')),
        ('wrong asset count', stderr.replace('263 assets','264 assets')),
        ('missing newline', stderr.rstrip('\n')),
        ('duplicate loader', stderr+stderr),
        ('extra blank line', stderr+'\n')]
    for name,text in diagnostic_cases:
        run(name,patch.object(v.subprocess,'run',return_value=subprocess.CompletedProcess([str(a.probe)],0,stdout,text)))
    for code in (False,0.0):
        run('noninteger process code '+repr(code),patch.object(v.subprocess,'run',return_value=subprocess.CompletedProcess([str(a.probe)],code,stdout,stderr)))
    try:
        v.verify(a.capture,a.probe,a.rom,a.output/'wrong-assets.json',assets=a.rom)
        checks.append(dict(name='wrong immutable assets',rejected=False,reason='accepted'))
    except ValueError as error:
        checks.append(dict(name='wrong immutable assets',rejected=True,reason=str(error)))
    report = dict(test_source_sha256=v.sha(__file__),passed=all(c['rejected'] for c in checks), verifier_sha256=v.sha(a.verifier), probe_sha256=v.sha(a.probe), manifest_sha256=v.sha(a.capture / 'manifest.json'), checks=checks)
    (a.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
