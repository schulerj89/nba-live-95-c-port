"""Strictly replay compact native `$86:F0B7-$F0FC` witnesses in production C."""

import argparse
import json
import struct
import subprocess
from pathlib import Path

from normalize_cpu_mode_nine_vectors import FIELDS as EXPECTED_FIELD_NAMES


EXPECTED_FIELDS = 52
EXPECTED_CALLS = 16
EXPECTED_ROUTINE = "$86:F0B7-$F0FC mode-nine timed target override"
EXPECTED_ROM_SHA256 = (
    "2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870"
)
EXPECTED_NAMES = (
    "inhibit_restores", "inhibit_high_restores_mode_six_alt_lower",
    "timer_zero_restores",
    "timer_one_restores", "timer_8002_restores", "timer_8001_steers",
    "target_x_steers", "target_y_steers", "target_equal_damps",
    "final_zero_accelerates", "final_nine_accelerates",
    "final_direction_damps", "final_boosted", "final_live_blocked",
    "final_airborne", "restore_locked_animation",
)
EXPECTED_EXECUTED = {
    "86f0b7", "86f0b9", "86f0bc", "86f0be", "86f0c1", "86f0c2",
    "86f0c4", "86f0c7", "86f0c9", "86f0cc", "86f0ce", "86f0d1",
    "86f0d3", "86f0d6", "86f0d8", "86f0dc", "86f0dd", "86f0e0",
    "86f0e3", "86f0e6", "86f0e9", "86f0ec", "86f0ee", "86f0f2",
    "86f0f3", "86f0f6", "86f0f8", "86f0fc",
}
RESTORE_NAMES = {
    "inhibit_restores", "inhibit_high_restores_mode_six_alt_lower",
    "timer_zero_restores",
    "timer_one_restores", "timer_8002_restores",
    "restore_locked_animation",
}
TARGET_NAMES = {
    "timer_8001_steers", "target_x_steers", "target_y_steers",
    "target_equal_damps",
}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--vectors", type=Path, required=True)
    parser.add_argument("--probe", type=Path, required=True)
    parser.add_argument("--pack", type=Path, required=True)
    args = parser.parse_args()
    document = json.loads(args.vectors.read_text())
    calls = document.get("calls", [])
    fields = document.get("fields", [])
    if document.get("schema") != 1 or \
            document.get("routine") != EXPECTED_ROUTINE or \
            document.get("rom_sha256") != EXPECTED_ROM_SHA256 or \
            tuple(call.get("name") for call in calls) != EXPECTED_NAMES or \
            len(calls) != EXPECTED_CALLS or \
            tuple(fields) != EXPECTED_FIELD_NAMES:
        raise ValueError("mode-nine fixture identity/schema/count changed")

    payload = bytearray()
    executed_union = set()
    for number, call in enumerate(calls, 1):
        inputs = call.get("input", [])
        expected = call.get("expected", [])
        if len(inputs) != EXPECTED_FIELDS or len(expected) != EXPECTED_FIELDS:
            raise ValueError(f"case {number} has a truncated field vector")
        if any(not isinstance(value, int) or not 0 <= value <= 0xFFFF
               for value in inputs + expected):
            raise ValueError(f"case {number} contains a non-word field")
        name = call["name"]
        expected_exit = "86f0f2" if name in RESTORE_NAMES else \
            "86f0dc" if name in TARGET_NAMES else "86f0fc"
        executed = call.get("executed")
        if call.get("entry_pc") != "86f0b7" or \
                call.get("exit_pc") != expected_exit or \
                not isinstance(executed, list) or not executed or \
                executed[0] != "86f0b7" or executed[-1] != expected_exit:
            raise ValueError(f"case {number} has an invalid native boundary")
        executed_union.update(executed)
        expected_children = {
            "animation_87b3bd": 1 if name in RESTORE_NAMES else 0,
            "steering_85b3aa": 1 if name in TARGET_NAMES else 0,
            "velocity_85a82c": 0 if name in RESTORE_NAMES else 1,
        }
        if call.get("child_calls") != expected_children:
            raise ValueError(f"case {number} has changed native child counts")
        payload.extend(struct.pack("<" + "H" * EXPECTED_FIELDS, *inputs))
    if executed_union != EXPECTED_EXECUTED:
        raise ValueError(
            f"native instruction-start union changed: "
            f"{len(executed_union)}/28 starts")

    run = subprocess.run(
        [str(args.probe), str(args.pack)], input=payload,
        capture_output=True, check=True)
    lines = [line for line in run.stdout.decode().splitlines()
             if line.strip() and not line.startswith("[")]
    if len(lines) != EXPECTED_CALLS:
        raise AssertionError(
            f"probe returned {len(lines)} rows, expected {EXPECTED_CALLS}")

    mismatches = []
    for number, (call, line) in enumerate(zip(calls, lines), 1):
        words = line.split()
        if len(words) != EXPECTED_FIELDS:
            raise AssertionError(
                f"case {number} returned {len(words)} fields, expected "
                f"{EXPECTED_FIELDS}")
        actual = [int(word, 16) for word in words]
        differences = [
            (fields[index], wanted, got)
            for index, (wanted, got) in enumerate(zip(call["expected"], actual))
            if wanted != got
        ]
        if differences:
            mismatches.append((number, call["name"], differences))

    print(
        f"[CPU MODE NINE] {'PASS' if not mismatches else 'FAIL'}: "
        f"calls={len(calls)} fields={len(fields)} mismatches={len(mismatches)}"
    )
    for number, name, differences in mismatches:
        print(f"case {number} {name}: {differences[:12]}")
    if mismatches:
        raise SystemExit(1)

    integration = subprocess.run(
        [str(args.probe), str(args.pack), "--integration"],
        capture_output=True, text=True)
    if integration.returncode != 0:
        print(integration.stdout, end="")
        print(integration.stderr, end="")
        raise AssertionError(
            f"mode-nine production scheduler check exited "
            f"{integration.returncode}")
    if "[CPU MODE NINE INTEGRATION] PASS" not in integration.stdout.splitlines():
        raise AssertionError("mode-nine production caller check did not pass")
    print("[CPU MODE NINE INTEGRATION] PASS")


if __name__ == "__main__":
    main()
