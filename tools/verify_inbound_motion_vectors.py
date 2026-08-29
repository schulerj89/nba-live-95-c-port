"""Replay normalized native inbound motion through compiled production C."""

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
    if fixture["raw_calls"] != 500 or len(calls) < 100:
        raise AssertionError("native F43A witness population shrank")
    stdin = "\n".join(" ".join(f"{value & 0xFFFF:04x}" for value in call["input"])
                       for call in calls) + "\n"
    result = subprocess.run([args.probe], input=stdin, capture_output=True,
                            text=True, check=True)
    produced = [[int(value, 16) for value in line.split()[:3]]
                for line in result.stdout.splitlines() if line.strip()]
    mismatches = [(call["call"], call["input"], call["expected"], actual)
                  for call, actual in zip(calls, produced)
                  if call["expected"] != actual]
    signs = Counter(("neg" if value & 0x8000 else "nonnegative")
                    for call in calls for value in call["input"][4:6])
    profiles = {call["input"][7] for call in calls}
    if signs["neg"] == 0 or signs["nonnegative"] == 0 or len(profiles) < 2:
        raise AssertionError("native motion fixture lost signed/profile diversity")
    print(f"[INBOUND MOTION] {'PASS' if not mismatches else 'FAIL'}: "
          f"calls={len(calls)} mismatches={len(mismatches)} "
          f"profiles={len(profiles)} signs={dict(signs)}")
    for mismatch in mismatches[:10]:
        print("  call=%d input=%s rom=%s port=%s" % mismatch)
    if len(produced) != len(calls) or mismatches:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
