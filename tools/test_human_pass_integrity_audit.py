"""Independent pass evidence mutations; original files remain unchanged."""
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

    def events(name, change):
        rows = copy.deepcopy(original_rows)
        change(rows)
        text = '\n'.join(json.dumps(row) for row in rows) + '\n'
        # Only the parsed event view is altered, after original on-disk hash checks.
        # This tests row/schema consistency without rewriting capture artifacts.
        def altered(path, *args, **kwargs):
            return text if path == a.capture / 'boundaries.jsonl' else actual_text(path, *args, **kwargs)
        run(name, patch.object(Path, 'read_text', altered))

    index = next(i for i, r in enumerate(original_rows) if r['tag'] == 'pass.entry')
    for field, value in [('actor', True), ('direction', 1.5), ('owner', 0x10000), ('live', 0x10000), ('score', 0x10000), ('candidate', 0x10000), ('actor', 0x10000), ('direction', 0x10000)]:
        events(f'entry {field}={value!r}', lambda rows, k=field, x=value: rows[index].update({k: x}))
    for field in ('actor', 'owner', 'live', 'offense', 'candidate', 'score', 'direction'):
        events('entry raw disagreement ' + field, lambda rows, k=field: rows[index].update({k: rows[index][k] ^ 1}))
    events('all court clocks exceed completion', lambda rows: [r.update(court=r['court'] + 100000) for r in rows])
    events('all frame clocks exceed runner stop bound', lambda rows: [r.update(frame=r['frame'] + 100000) for r in rows])
    events('wrong native PC', lambda rows: rows[index].update(pc=rows[index]['pc'] + 1))
    events('nonzero native DP', lambda rows: rows[index].update(cpu_d=1))
    events('duplicated native index', lambda rows: rows[index].update(index=1))

    lines = stdout.splitlines()
    pass_index = next(i for i, line in enumerate(lines) if 'route' in json.loads(line))
    first = json.loads(lines[pass_index])
    outputs = [('missing C row', '\n'.join(lines[:-1]) + '\n'), ('extra C row', stdout + lines[0] + '\n'), ('arbitrary C noise', 'noise\n' + stdout)]
    for field in ('route', 'actor_words', 'controller_words', 'context_words', 'global_words', 'handoff_words'):
        altered = copy.deepcopy(first)
        if isinstance(altered[field], list):
            altered[field][0] = float(altered[field][0])
        else:
            altered[field] = float(altered[field])
        outputs.append(('float C ' + field, '\n'.join([*lines[:pass_index], json.dumps(altered), *lines[pass_index + 1:]]) + '\n'))
    for name, text in outputs:
        run(name, patch.object(v.subprocess, 'run', return_value=subprocess.CompletedProcess([str(a.probe)], 0, text, '')))
    report = dict(passed=all(c['rejected'] for c in checks), verifier_sha256=v.sha(a.verifier), probe_sha256=v.sha(a.probe), manifest_sha256=v.sha(a.capture / 'manifest.json'), checks=checks)
    (a.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
