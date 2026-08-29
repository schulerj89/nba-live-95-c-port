import argparse
import json
import subprocess
from pathlib import Path


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--vectors", required=True)
    parser.add_argument("--probe", required=True)
    args = parser.parse_args()
    document = json.loads(Path(args.vectors).read_text())
    calls = document["calls"]
    payload = "\n".join(
        " ".join(f"{value:x}" for value in call["input"]) for call in calls
    ) + "\n"
    result = subprocess.run(
        [args.probe], input=payload, text=True, capture_output=True, check=True
    )
    actual = [int(line, 16) for line in result.stdout.splitlines()]
    mismatches = [
        (call["call"], call["expected"], value)
        for call, value in zip(calls, actual)
        if call["expected"] != value
    ]
    nibbles = {
        (call["input"][2] >> 4) & 0x0F
        for call in calls
        if call["input"][0] < 0x80 and call["input"][1] != 0
    }
    preserved = sum(
        1 for call in calls if call["input"][0] >= 0x80 or call["input"][1] == 0
    )
    passed = (
        len(actual) == len(calls)
        and not mismatches
        and nibbles == set(range(16))
        and preserved >= 2
    )
    print(
        f"[HUMAN INBOUND] {'PASS' if passed else 'FAIL'}: "
        f"cases={len(calls)} direction_nibbles={len(nibbles)}/16 "
        f"preservation_cases={preserved} mismatches={len(mismatches)}"
    )
    if not passed:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
