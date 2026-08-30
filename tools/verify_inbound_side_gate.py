"""Replay controlled Mesen F61F branch witnesses through the portable gate."""
import argparse
import itertools
import json
import subprocess
from pathlib import Path


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--vectors', required=True)
    parser.add_argument('--probe', required=True)
    args = parser.parse_args()
    fixture = json.loads(Path(args.vectors).read_text(encoding='utf-8-sig'))
    calls = fixture['calls']
    assert fixture['controlled'] is True
    expected_grid = set(itertools.product((0, 5), (-336, 336),
                                         (-21, -20, -19, 19, 20), (-1, 0)))
    actual_grid = {(c['group'], *c['input']) for c in calls}
    assert len(calls) == 40 and actual_grid == expected_grid
    for call in calls:
        assert call['controlled'] is True and call['entry_pc'] == 0x86F61F
        assert call['exit_pc'] in (0x86F648, 0x86F653)
        assert call['allowed'] == (call['exit_pc'] == 0x86F648)
        assert call['executed'][0] == 0x86F61F
        assert call['executed'][-1] == call['exit_pc']
    payload = '\n'.join(' '.join(f'{v & 0xffff:x}' for v in c['input'])
                        for c in calls) + '\n'
    result = subprocess.run([args.probe], input=payload, text=True,
                            capture_output=True, check=True)
    actual = [int(line) for line in result.stdout.splitlines()]
    assert len(actual) == len(calls)
    bad = [(c['case'], int(c['allowed']), value)
           for c, value in zip(calls, actual) if int(c['allowed']) != value]
    print(f'[INBOUND SIDE GATE] calls={len(calls)} mismatches={len(bad)} '
          f'controlled=true groups=2 anchor_signs=2')
    for row in bad:
        print('  case=%d rom=%d port=%d' % row)
    if bad:
        raise SystemExit(1)


if __name__ == '__main__':
    main()
