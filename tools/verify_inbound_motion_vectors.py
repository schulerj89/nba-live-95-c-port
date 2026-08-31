"""Replay normalized native inbound motion through compiled production C."""

import argparse
import json
import re
import subprocess
from collections import Counter
from pathlib import Path


def parse_output(stdout, count):
    """The probe emits four words; never silently truncate additional output.

    Historical retained witnesses independently own only velocity X/Y and boost.
    Direction is parsed but cannot receive comparison credit from this corpus;
    verify_inbound_internal.py owns its separately captured native boundary.
    """
    lines = stdout.splitlines()
    if len(lines) != count:
        raise ValueError(f'expected {count} output rows, got {len(lines)}')
    rows = []
    for line in lines:
        tokens = line.split()
        if len(tokens) != 4 or any(not re.fullmatch(r'[0-9a-fA-F]{4}', token)
                                   for token in tokens):
            raise ValueError(f'malformed complete motion-probe row: {line!r}')
        rows.append([int(token, 16) for token in tokens])
    return rows


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--vectors", required=True)
    parser.add_argument("--probe", required=True)
    args = parser.parse_args()
    fixture = json.loads(Path(args.vectors).read_text())
    calls = fixture["calls"]
    if fixture["raw_calls"] != 500 or len(calls) != 111 or \
            fixture.get("motion_calls") != len(calls):
        raise AssertionError("native F43A witness population shrank")
    for call in calls:
        if len(call['input']) != 11 or len(call['expected']) != 3 or \
                any(type(value) is not int or not 0 <= value <= 0xffff
                    for value in call['input'] + call['expected']):
            raise ValueError('invalid retained motion witness schema')
    stdin = "\n".join(" ".join(f"{value & 0xFFFF:04x}" for value in call["input"])
                       for call in calls) + "\n"
    result = subprocess.run([args.probe], input=stdin, capture_output=True,
                            text=True, check=True)
    produced = parse_output(result.stdout, len(calls))
    mismatches = [(call["call"], call["input"], call["expected"], actual)
                  for call, actual in zip(calls, produced)
                  if call["expected"] != actual[:3]]
    signs = Counter(("neg" if value & 0x8000 else "nonnegative")
                    for call in calls for value in call["input"][4:6])
    profiles = {call["input"][7] for call in calls}
    if signs["neg"] == 0 or signs["nonnegative"] == 0 or len(profiles) < 2:
        raise AssertionError("native motion fixture lost signed/profile diversity")
    print(f"[INBOUND MOTION] {'PASS' if not mismatches else 'FAIL'}: "
          f"calls={len(calls)} mismatches={len(mismatches)} "
          f"profiles={len(profiles)} signs={dict(signs)} "
          "compared=vx,vy,boost direction=not-owned-by-this-fixture")
    for mismatch in mismatches[:10]:
        print("  call=%d input=%s rom=%s port=%s" % mismatch)
    if len(produced) != len(calls) or mismatches:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
