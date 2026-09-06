"""Strictly replay compact native `$86:994C-$99C3` witnesses in production C."""

import argparse
import json
import struct
import subprocess
from pathlib import Path

from normalize_cpu_mode_seven_vectors import FIELDS as EXPECTED_FIELD_NAMES


EXPECTED_FIELDS = 50
EXPECTED_CALLS = 19
EXPECTED_ROUTINE = "$86:994C-$99C3 mode-seven dead-ball hold"
EXPECTED_ROM_SHA256 = (
    "2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870"
)
EXPECTED_NAMES = (
    "live_state_restore", "high_byte_live_state_restore",
    "integer_height_hold", "negative_integer_height_hold",
    "timer_underflow_restore", "timer_8002_restores",
    "timer_8001_wraps_positive", "fractional_height_stationary",
    "stationary_human", "moving_human_first", "moving_human_y_only",
    "moving_human_repeat", "moving_human_boost_preserved",
    "cpu_origin_damps", "cpu_origin_damps_to_zero",
    "cpu_off_origin_steers", "stationary_matching_state",
    "stationary_negative_upper_lock", "restore_boost_remaps_state_three",
)
EXPECTED_EXECUTED = {
    "86994c", "86994f", "869952", "869954", "869957", "869959",
    "86995d", "869961", "869962", "869964", "869967", "869969",
    "86996c", "86996d", "86996f", "869972", "869974", "869977",
    "869979", "86997b", "86997d", "869981", "869983", "869986",
    "869989", "86998b", "86998e", "869990", "869994", "869996",
    "869999", "86999c", "86999f", "8699a1", "8699a4", "8699a6",
    "8699aa", "8699ac", "8699af", "8699b1", "8699b4", "8699b7",
    "8699ba", "8699bd", "8699c0", "8699c3",
}
RESTORE_NAMES = {
    "live_state_restore", "high_byte_live_state_restore",
    "timer_underflow_restore", "timer_8002_restores",
    "restore_boost_remaps_state_three",
}
HEIGHT_NAMES = {"integer_height_hold", "negative_integer_height_hold"}
CPU_NAMES = {
    "cpu_origin_damps", "cpu_origin_damps_to_zero", "cpu_off_origin_steers"
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
        raise ValueError("mode-seven fixture identity/schema/count changed")

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
        expected_exit = "869961" if name in RESTORE_NAMES else "8699c3"
        executed = call.get("executed")
        if call.get("entry_pc") != "86994c" or \
                call.get("exit_pc") != expected_exit or \
                not isinstance(executed, list) or not executed or \
                executed[0] != "86994c" or executed[-1] != expected_exit:
            raise ValueError(f"case {number} has an invalid native boundary")
        executed_union.update(executed)
        expected_children = {
            "animation_87b3bd": 0 if name in HEIGHT_NAMES else 1,
            "restore_869846": 1 if name in RESTORE_NAMES else 0,
            "steering_85b3aa": 1 if name in CPU_NAMES else 0,
            "velocity_85a82c": 1 if name in CPU_NAMES else 0,
        }
        if call.get("child_calls") != expected_children:
            raise ValueError(f"case {number} has changed native child counts")
        payload.extend(struct.pack("<" + "H" * EXPECTED_FIELDS, *inputs))
    if executed_union != EXPECTED_EXECUTED:
        raise ValueError(
            f"native instruction-start union changed: "
            f"{len(executed_union)}/46 starts")

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
        expected = call["expected"]
        differences = [
            (fields[index], wanted, got)
            for index, (wanted, got) in enumerate(zip(expected, actual))
            if wanted != got
        ]
        if differences:
            mismatches.append((number, call["name"], differences))

    print(
        f"[CPU MODE SEVEN] {'PASS' if not mismatches else 'FAIL'}: "
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
            f"mode-seven production scheduler check exited "
            f"{integration.returncode}")
    integration_lines = integration.stdout.splitlines()
    if "[CPU MODE SEVEN INTEGRATION] PASS" not in integration_lines:
        raise AssertionError("mode-seven production caller check did not pass")
    print("[CPU MODE SEVEN INTEGRATION] PASS")


if __name__ == "__main__":
    main()
