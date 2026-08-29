import argparse
import json
import subprocess
from pathlib import Path


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--vectors", required=True)
    parser.add_argument("--probe", required=True)
    args = parser.parse_args()
    calls = json.loads(Path(args.vectors).read_text())["calls"]
    if len(calls) < 4:
        raise AssertionError("foul-bookkeeping native branch census is incomplete")
    payload = "\n".join(" ".join(f"{value & 0xffff:x}"
                                  for value in call["input"])
                        for call in calls) + "\n"
    result = subprocess.run([args.probe], input=payload, text=True,
                            capture_output=True, check=True)
    actual = [[int(value, 16) for value in line.split()]
              for line in result.stdout.splitlines()]
    bad = [(call["call"], call["expected"], got)
           for call, got in zip(calls, actual)
           if call["expected"] != got]
    print(f"[FOUL BOOKKEEPING] {'PASS' if not bad else 'FAIL'}: "
          f"cases={len(calls)} mismatches={len(bad)}")
    for mismatch in bad:
        print("  call=%d rom=%s port=%s" % mismatch)
    if len(actual) != len(calls) or bad:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
