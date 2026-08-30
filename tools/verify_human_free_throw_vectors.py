import argparse
from collections import Counter
import hashlib
import json
import pathlib
import subprocess

ROM_SHA256 = "2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870"


def normalized_rom_hash(path):
    data = pathlib.Path(path).read_bytes()
    if len(data) % 0x8000 == 512:
        data = data[512:]
    return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--vectors", required=True)
    parser.add_argument("--probe", required=True)
    parser.add_argument("--rom", required=True)
    args = parser.parse_args()
    document = json.loads(pathlib.Path(args.vectors).read_text())
    if normalized_rom_hash(args.rom) != ROM_SHA256 or \
            document.get("schema") != 1 or document.get("rom_sha256") != ROM_SHA256:
        raise SystemExit("[HUMAN FREE THROW] FAIL: fixture identity mismatch")
    if document.get("native_entry") != "879CBF" or \
            document.get("native_oscillator") != "87A018-87A045":
        raise SystemExit("[HUMAN FREE THROW] FAIL: native boundary mismatch")
    cases = document.get("cases", [])
    if len(cases) != 7:
        raise SystemExit("[HUMAN FREE THROW] FAIL: expected seven native cases")
    names = set()
    source_calls = set()
    expected_result_codes = {
        (3, 3, 0): 0,
        (3, 4, 1): 1,
        (4, 4, 1): 0,
        (4, 5, 0): 2,
        (5, 5, 0): 0,
        (5, 9, 1): 3,
    }
    for case in cases:
        name = case.get("name")
        source_call = case.get("source_call")
        values = case.get("input")
        expected = case.get("expected")
        if not isinstance(name, str) or name in names or \
                isinstance(source_call, bool) or \
                not isinstance(source_call, int) or source_call <= 0 or \
                source_call in source_calls or \
                not isinstance(values, list) or len(values) != 8 or \
                not isinstance(expected, list) or len(expected) != 6 or \
                any(not isinstance(value, int) or not 0 <= value <= 0xffff
                    for value in values + expected):
            raise SystemExit("[HUMAN FREE THROW] FAIL: malformed fixture case")
        names.add(name)
        source_calls.add(source_call)
        signature = (values[0], expected[1], values[7])
        if values[5] != 0 or values[6] == 0 or \
                expected_result_codes.get(signature) != expected[0]:
            raise SystemExit(
                "[HUMAN FREE THROW] FAIL: invalid human transition oracle")
    wrap_cases = [case for case in cases
                  if (case["input"][0], case["expected"][1],
                      case["input"][7], case["expected"][0]) == (3, 3, 0, 0)
                  and 108 <= case["input"][1] < 112 and
                  case["expected"][2] < case["input"][1]]
    if len(wrap_cases) != 1:
        raise SystemExit("[HUMAN FREE THROW] FAIL: missing cursor-wrap oracle")
    wrap_source_call = wrap_cases[0]["source_call"]
    transition_signatures = Counter(
        (case["input"][0], case["expected"][1], case["input"][7])
        for case in cases if case["source_call"] != wrap_source_call)
    required_signatures = Counter(expected_result_codes.keys())
    if transition_signatures != required_signatures:
        raise SystemExit(
            "[HUMAN FREE THROW] FAIL: incomplete state/input signatures")
    payload = "".join(
        " ".join(f"{value & 0xffff:x}" for value in case["input"]) + "\n"
        for case in cases)
    run = subprocess.run([args.probe], input=payload, text=True,
                         capture_output=True)
    if run.returncode:
        raise SystemExit(
            f"[HUMAN FREE THROW] FAIL: probe exit {run.returncode}: "
            f"{run.stderr}")
    output = [line for line in run.stdout.splitlines() if line.strip()]
    failures = []
    for case, line in zip(cases, output):
        actual = [int(value, 16) for value in line.split()]
        if actual != case["expected"]:
            failures.append((case["name"], case["expected"], actual))
    if len(output) != len(cases):
        failures.append(("line-count", len(cases), len(output)))
    if failures:
        for name, expected, actual in failures:
            print(f"{name}: expected={expected} actual={actual}")
        raise SystemExit(
            f"[HUMAN FREE THROW] FAIL: mismatches={len(failures)}")
    print(f"[HUMAN FREE THROW] PASS: cases={len(cases)} mismatches=0")


if __name__ == "__main__":
    main()
