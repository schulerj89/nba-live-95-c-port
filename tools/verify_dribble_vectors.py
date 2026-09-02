"""Replay fresh Mesen dribbles with an independently stale action-phase counter."""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import subprocess

from normalize_ball_driver_owned_vectors import INPUT_FIELDS, OUTPUT_FIELDS, ROM_SHA256

FIXTURE_SHA256 = "5e6e875a772d226e2058dd954cc1d457cd3bccf77853e37a0f526b3f9ea3c4b7"
DRAW_FIXTURE_SHA256 = "2f7ea273165b0dcf57361c56aebf6a3a7e111ca1f0c2c7d1586777632179a51d"


def verify_draw(vectors, probe, assets, rom):
    raw = Path(vectors).read_bytes()
    if hashlib.sha256(raw).hexdigest() != DRAW_FIXTURE_SHA256 or \
            hashlib.sha256(Path(rom).read_bytes()).hexdigest() != ROM_SHA256:
        raise ValueError("native draw fixture/ROM identity changed")
    fixture = json.loads(raw)
    if fixture["schema"] != 1 or fixture["state_injection"] is not False or \
            fixture["native_entry"] != "80AF1E" or fixture["native_exit"] != "80B0AB":
        raise ValueError("invalid native draw boundary")
    cases = fixture["cases"]
    if len(cases) != 67 or {c["input"][12] for c in cases} != {0, 1}:
        raise ValueError("missing front/back depth branches")
    for index, case in enumerate(cases, 1):
        if case["call"] != index or len(case["input"]) != 18 or \
                case["input"][13] >= 0x082c or len(case["parts"]) not in (4, 5) or \
                any(len(part) != 4 for part in case["parts"]) or \
                sum(part[0] == 0x081d for part in case["parts"]) != 1:
            raise ValueError("invalid native owner draw")
    payload = "".join(" ".join(f"{v:04x}" for v in c["input"]) + "\n" for c in cases)
    run = subprocess.run([str(Path(probe).resolve()), str(Path(assets).resolve())],
                         input=payload, text=True, capture_output=True, timeout=30)
    if run.returncode:
        raise AssertionError(f"draw probe failed: {run.returncode}: {run.stderr}")
    lines = [line for line in run.stdout.splitlines() if line and
             not line.startswith("[ASSETS] Loaded asset pack:")]
    if len(lines) != len(cases):
        raise AssertionError("draw probe dropped or added calls")
    words = 0
    for case, line in zip(cases, lines):
        actual = [int(v, 16) for v in line.split()]
        expected = [len(case["parts"])] + [v for part in case["parts"] for v in part]
        if actual != expected:
            raise AssertionError(f"draw call {case['call']}: C={actual} Mesen={expected}")
        words += len(expected)
    return {"passed": True, "fixture_sha256": DRAW_FIXTURE_SHA256, "native_calls": len(cases),
            "compared_words": words, "mismatches": 0}


def verify(vectors, probe, assets, rom):
    raw = Path(vectors).read_bytes()
    if hashlib.sha256(raw).hexdigest() != FIXTURE_SHA256:
        raise ValueError("dribble fixture identity changed")
    if hashlib.sha256(Path(rom).read_bytes()).hexdigest() != ROM_SHA256:
        raise ValueError("expected the verified USA ROM")
    fixture = json.loads(raw)
    for key, value in {"schema": 1, "rom_sha256": ROM_SHA256, "state_injection": False,
                       "native_entry": "859A37", "native_exit": "85A7C7",
                       "input_fields": INPUT_FIELDS, "output_fields": OUTPUT_FIELDS}.items():
        if fixture.get(key) != value:
            raise ValueError(f"invalid fixture {key}")
    cases = fixture["cases"]
    if len(cases) != 67 or {c["input"][26] for c in cases} != set(range(8)):
        raise ValueError("missing native calls or dribble phases")
    for index, case in enumerate(cases, 1):
        values, expected = case["input"], case["expected"]
        if case["call"] != index or len(values) != 47 or len(expected) != 31 or any(
                type(v) is not int or not 0 <= v <= 65535 for v in values + expected):
            raise ValueError("malformed native call")
        if values[0] >= 10 or values[23] >= 0xf0 or values[27] != 11 or \
                case["base"] not in (9, 11) or case["subject"] != 0x34eb + values[0] * 256:
            raise ValueError("call exceeds the captured low-resource dribble scope")
    payload = "".join(" ".join(f"{v:04x}" for v in c["input"] +
                               [4, c["input"][0], c["input"][0]]) + "\n" for c in cases)
    reports = []
    for stale in (False, True):
        command = [str(Path(probe).resolve()), str(Path(assets).resolve())]
        if stale:
            command.append("--stale-compat-phase")
        run = subprocess.run(command, input=payload, text=True, capture_output=True, timeout=30)
        if run.returncode:
            raise AssertionError(f"dribble probe failed: {run.returncode}: {run.stderr}")
        lines = [line for line in run.stdout.splitlines() if line and
                 not line.startswith("[ASSETS] Loaded asset pack:")]
        if len(lines) != len(cases):
            raise AssertionError("probe dropped or added native calls")
        for case, line in zip(cases, lines):
            actual = [int(v, 16) for v in line.split()]
            if len(actual) != 34 or any(not 0 <= v <= 65535 for v in actual):
                raise AssertionError("invalid probe output")
            # All 31 captured words, including fractions, velocity, history,
            # attachment state and events. No native-field exclusions.
            for field, want, got in zip(OUTPUT_FIELDS, case["expected"], actual):
                if want != got:
                    raise AssertionError(f"call {case['call']} stale_phase={stale} "
                                         f"{field}: C={got:04x} Mesen={want:04x}")
        reports.append({"stale_compatibility_phase": stale, "calls": len(cases),
                        "compared_native_words": len(cases) * 31, "mismatches": 0})
    return {"passed": True, "fixture_sha256": FIXTURE_SHA256,
            "native_calls": len(cases), "replays": len(cases) * 2,
            "phases": dict(sorted(Counter(c["input"][26] for c in cases).items())),
            "variants": reports}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("vectors", "probe", "assets", "rom"):
        parser.add_argument("--" + name, type=Path, required=True)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    report = verify(args.vectors, args.probe, args.assets, args.rom)
    if args.output:
        args.output.write_text(json.dumps(report, indent=2) + "\n")
    print(f"[DRIBBLE NATIVE] PASS: calls={report['native_calls']} "
          f"replays={report['replays']} phases=0..7 mismatches=0")


if __name__ == "__main__":
    main()
