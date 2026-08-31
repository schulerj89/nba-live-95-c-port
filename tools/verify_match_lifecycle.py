"""Replay recorded period/clock/latch projections, never expected C constants.

The native capture seeds period, score and one clock tick. The C adapter starts
after that tick and skips host presentation waits. Only the three declared
terminal words are compared; stamina, frame timing and the full caller are NOT
part of this proof. --native-root independently checks the compact fixture
against the retained native event files before replay.
"""
import argparse
import json
import re
import subprocess
from pathlib import Path

from differential_compare import digest, load, object_without_duplicates

NAMES = {"q1", "halftime", "regulation_tie", "regulation_final"}
OUTPUTS = {"period": "0926", "clock": "0928", "latch": "09b4"}


def uint(value, label, maximum=65535):
    if type(value) is not int or not 0 <= value <= maximum:
        raise ValueError(f"{label}: expected unsigned integer <= {maximum}")
    return value


def read_fixture(path):
    data = json.loads(Path(path).read_text(), object_pairs_hook=object_without_duplicates)
    if type(data) is not dict or type(data.get("schema")) is not int or data.get("schema") != 1:
        raise ValueError("unsupported lifecycle fixture schema")
    source = data.get("source", {})
    if (source.get("system") != "retail ROM under Mesen" or
            source.get("capture_script") != "tools/mesen_match_lifecycle_capture.lua" or
            not source.get("controlled_writes") or
            source.get("clock_setting") != "12-minute regulation / 5-minute overtime (setting index 3)"):
        raise ValueError("missing/unsupported native source or controlled launch declaration")
    cases = data.get("cases")
    if not isinstance(cases, list) or len(cases) != len(NAMES):
        raise ValueError("expected all four recorded cases")
    names = []
    for case in cases:
        if not isinstance(case, dict) or set(case) != {"name", "seed", "ordered_witnesses", "result"}:
            raise ValueError("unexpected/missing case fields")
        names.append(case["name"])
        seed, result = case["seed"], case["result"]
        if type(seed) is not dict or set(seed) != {"period", "left_score", "right_score"}:
            raise ValueError("incomplete seed")
        for key, value in seed.items():
            uint(value, key, 4 if key == "period" else 65535)
        if type(result) is not dict or set(result) != set(OUTPUTS):
            raise ValueError("incomplete native output projection")
        for key, value in result.items():
            uint(value, key)
        witnesses = case["ordered_witnesses"]
        if not isinstance(witnesses, list) or len(witnesses) < 3 or any(
                not isinstance(pc, str) or not re.fullmatch(r"\$[0-9A-F]{2}:[0-9A-F]{4}", pc)
                for pc in witnesses):
            raise ValueError("invalid native witness sequence")
    if len(set(names)) != len(names) or set(names) != NAMES:
        raise ValueError("missing/duplicate/unexpected lifecycle case")
    return cases


def audit_native(cases, root):
    evidence = []
    for case in cases:
        path = Path(root) / case["name"] / "events.jsonl"
        rows = load(path)
        if not rows or any(r.get("case") != case["name"] for r in rows):
            raise ValueError(f"{path}: empty or mixed-case native events")
        previous_frame = -1
        for row in rows:
            frame = uint(row.get("frame"), "native frame", 0x7fffffff)
            if frame < previous_frame:
                raise ValueError(f"{path}: native frames out of order")
            previous_frame = frame
            if not isinstance(row.get("pc"), str) or not re.fullmatch(r"[0-9a-f]{6}", row["pc"]):
                raise ValueError(f"{path}: invalid native PC")
            state = row.get("state")
            if not isinstance(state, dict) or not set(OUTPUTS.values()) <= set(state):
                raise ValueError(f"{path}: incomplete native state")
            for address, value in state.items():
                if not re.fullmatch(r"[0-9a-f]{4}", address):
                    raise ValueError(f"{path}: invalid native word address")
                uint(value, f"native {address}")
        seeds = [r for r in rows if r.get("event") == "controlled_seed_before_native_clock_writer"]
        if len(seeds) != 1 or seeds[0].get("pc") != "85edc6":
            raise ValueError(f"{path}: missing/ambiguous native seed boundary")
        native_seed = seeds[0]["state"]
        for key, address in {"period": "0926", "left_score": "4711", "right_score": "4791"}.items():
            if native_seed.get(address) != case["seed"][key]:
                raise ValueError(f"{path}: recorded seed differs: {key}")
        if native_seed.get("0928") != 1:
            raise ValueError(f"{path}: expected one native clock-writer tick")
        index = 0
        for witness in case["ordered_witnesses"]:
            pc = witness.replace("$", "").replace(":", "").lower()
            while index < len(rows) and rows[index].get("pc") != pc:
                index += 1
            if index == len(rows):
                raise ValueError(f"{path}: missing ordered witness {witness}")
            index += 1
        terminal = rows[-1]
        terminal_pcs = {"next_period_clock_ready": "86dd47",
                        "final_postgame_handoff": "8797a0",
                        "postgame_exhibition_return_boundary": "87985c"}
        terminal_event = terminal.get("event")
        if terminal_event not in terminal_pcs or terminal["pc"] != terminal_pcs[terminal_event]:
            raise ValueError(f"{path}: missing terminal capture boundary")
        if (case["name"] == "regulation_final") == (terminal_event == "next_period_clock_ready"):
            raise ValueError(f"{path}: terminal boundary belongs to a different branch")
        for key, address in OUTPUTS.items():
            if terminal["state"].get(address) != case["result"][key]:
                raise ValueError(f"{path}: recorded output differs: {key}")
        evidence.append(dict(case=case["name"], path=str(path), sha256=digest(path),
                             terminal_pc=terminal["pc"], events=len(rows)))
    return evidence


def replay(cases, probe):
    inputs = "".join(f"{c['seed']['period']} {c['seed']['left_score']} "
                     f"{c['seed']['right_score']} 3\n" for c in cases)
    result = subprocess.run([str(probe), "--project"], input=inputs,
                            capture_output=True, text=True, timeout=60)
    if result.returncode:
        raise ValueError(f"probe failed ({result.returncode}): {result.stderr.strip()}")
    rows = result.stdout.splitlines()
    if len(rows) != len(cases):
        raise ValueError(f"expected {len(cases)} output rows, received {len(rows)}")
    for case, line in zip(cases, rows):
        fields = line.split()
        if len(fields) != len(OUTPUTS) or any(not re.fullmatch(r"[0-9]+", x) for x in fields):
            raise ValueError(f"{case['name']}: malformed/extra probe words: {line}")
        actual = dict(zip(OUTPUTS, (uint(int(x), "probe output") for x in fields)))
        if actual != case["result"]:
            raise ValueError(f"{case['name']}: native {case['result']} != C {actual}")
    return len(cases)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fixture", required=True)
    parser.add_argument("--probe", required=True)
    parser.add_argument("--native-root")
    parser.add_argument("--report")
    args = parser.parse_args()
    report = {"scope": "three terminal words; post-clock adapter; presentation waits skipped",
              "natural_journey": False, "whole_call_parity": False}
    try:
        cases = read_fixture(args.fixture)
        report["fixture_sha256"] = digest(args.fixture)
        report["probe_sha256"] = digest(args.probe)
        if args.native_root:
            report["native_events"] = audit_native(cases, args.native_root)
        report["cases"] = replay(cases, args.probe)
        report["status"] = "PROJECTION_MATCH"
        code = 0
    except (OSError, ValueError, KeyError, TypeError, subprocess.SubprocessError) as error:
        report.update(status="FAIL", error=str(error))
        code = 1
    if args.report:
        Path(args.report).write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))
    return code


if __name__ == "__main__":
    raise SystemExit(main())
