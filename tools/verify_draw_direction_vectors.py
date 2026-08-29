"""Replay native draw-facing calls through compiled production C."""

import argparse
import json
import subprocess
from collections import Counter
from pathlib import Path


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--vectors", required=True)
    parser.add_argument("--probe", required=True)
    args = parser.parse_args()
    fixture = json.loads(Path(args.vectors).read_text())
    calls = fixture["calls"]
    if fixture["raw_calls"] != 2000 or len(calls) != 2000:
        raise AssertionError("native draw-direction fixture population changed")
    payload = "\n".join(" ".join(f"{value & 0xffff:x}" for value in call["input"])
                          for call in calls) + "\n"
    result = subprocess.run([args.probe], input=payload, text=True,
                            capture_output=True, check=True)
    actual = [int(line, 16) for line in result.stdout.splitlines() if line]
    mismatches = [(call["call"], call["input"], call["expected"], value)
                  for call, value in zip(calls, actual)
                  if call["expected"] != value]
    modes = Counter(call["input"][1] for call in calls)
    candidates = sum(call["input"][5] != 0 for call in calls)
    changed = sum(call["input"][0] != call["expected"] for call in calls)
    if len(modes) < 5 or candidates < 60 or changed < 25:
        raise AssertionError("native draw selector lost branch diversity")
    print(f"[DRAW DIRECTION] {'PASS' if not mismatches else 'FAIL'}: "
          f"calls={len(calls)} mismatches={len(mismatches)} modes={dict(modes)} "
          f"candidates={candidates} changed={changed}")
    for mismatch in mismatches[:10]:
        print("  call=%d input=%s rom=%s port=%s" % mismatch)
    if len(actual) != len(calls) or mismatches:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
