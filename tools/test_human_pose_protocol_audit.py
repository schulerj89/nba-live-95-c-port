"""Independent pose output/register-domain checks; original capture bytes stay unchanged."""
import argparse, copy, importlib.util, json, subprocess, sys
from pathlib import Path
from unittest.mock import patch

def main():
    parser = argparse.ArgumentParser()
    for key in ('verifier', 'capture', 'probe', 'rom', 'output'):
        parser.add_argument('--' + key, type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(exist_ok=False)
    sys.path.insert(0, str(args.verifier.resolve().parent))
    spec = importlib.util.spec_from_file_location('pose_audit_verifier', args.verifier)
    verifier = importlib.util.module_from_spec(spec); spec.loader.exec_module(verifier)
    capture = args.capture.resolve()
    call = lambda name: verifier.verify(capture, args.probe.resolve(), args.rom.resolve(), args.output / (name + '.json'))
    baseline = call('baseline'); assert baseline['passed']
    stdout = (args.output / 'baseline.probe-stdout.txt').read_text()
    stderr = (args.output / 'baseline.probe-stderr.txt').read_text()
    lines = stdout.splitlines(); first = json.loads(lines[0]); checks = []
    def run(name, context):
        try:
            with context:
                report = call('case-%02d' % len(checks))
            rejected = not report['passed']; reason = 'C mismatch' if rejected else 'accepted'
        except (ValueError, TypeError, KeyError, AssertionError) as error:
            rejected = True; reason = str(error)
        checks.append(dict(name=name, rejected=rejected, reason=reason))
    changes = [('missing DP', lambda r: r.pop('dp_words')),
               ('short actor vector', lambda r: r['actor_words'].pop()),
               ('extra field', lambda r: r.update(extra=0)),
               ('invalid route', lambda r: r.update(route=65536)),
               ('bool scratch', lambda r: r['dp_words'].__setitem__(0, False)),
               ('high global', lambda r: r['global_words'].__setitem__(0, 65536))]
    for name, alter in changes:
        row = copy.deepcopy(first); alter(row)
        text = '\n'.join([json.dumps(row), *lines[1:]]) + '\n'
        run(name, patch.object(verifier.subprocess, 'run', return_value=subprocess.CompletedProcess([], 0, text, stderr)))
    for name, text in [('duplicate route', stdout.replace('"route":', '"route":0,"route":', 1))]:
        run(name, patch.object(verifier.subprocess, 'run', return_value=subprocess.CompletedProcess([], 0, text, stderr)))
    for name, text in [('extra failure diagnostic', stderr + 'ERROR\n'), ('missing diagnostic', ''), ('forged diagnostic', 'Loaded different assets\n')]:
        run(name, patch.object(verifier.subprocess, 'run', return_value=subprocess.CompletedProcess([], 0, stdout, text)))
    original_read = Path.read_text
    native_rows = [json.loads(line) for line in (capture / 'boundaries.jsonl').read_text().splitlines()]
    for tag in ('pose.entry', 'pose.offset.entry', 'pose.commit'):
        rows = copy.deepcopy(native_rows)
        row = next(row for row in rows if row['tag'] == tag)
        row['cpu_ps'] |= 8
        text = '\n'.join(json.dumps(row) for row in rows) + '\n'
        def read(path, *a, **kw):
            return text if path == capture / 'boundaries.jsonl' else original_read(path, *a, **kw)
        run('decimal mode ' + tag, patch.object(Path, 'read_text', read))
    report = dict(passed=all(row['rejected'] for row in checks), verifier_sha256=verifier.sha(args.verifier), checks=checks)
    (args.output / 'report.json').write_text(json.dumps(report, indent=2))
    print(json.dumps(report))
    return 0 if report['passed'] else 1

if __name__ == '__main__':
    raise SystemExit(main())
