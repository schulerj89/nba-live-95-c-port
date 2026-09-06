"""Strictly replay compact native `$86:A5B0-$A628` witnesses in production C."""

import argparse
import json
import struct
import subprocess
from pathlib import Path

from normalize_cpu_mode_ten_vectors import (
    FIELDS as EXPECTED_FIELD_NAMES,
    MUTABLE_FIELDS,
)


EXPECTED_FIELDS = 68
EXPECTED_CALLS = 18
EXPECTED_ROUTINE = "$86:A5B0-$A628 mode-ten receiver"
EXPECTED_ROM_SHA256 = (
    "2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870"
)
EXPECTED_NAMES = (
    "invalid_live_82_preserves", "invalid_live_81_clears",
    "aux_four_below_holds", "aux_five_equal_holds",
    "aux_six_actor_positive",
    "aux_thirteen_actor_positive_normalizes",
    "aux_thirteen_actor_negative_preserves",
    "aux_fourteen_actor_zero_normalizes",
    "aux_fifteen_actor_negative_preserves",
    "aux_eight_controller_positive_normalizes",
    "aux_eight_controller_negative_preserves",
    "cmp_8004_positive_controller_normalizes",
    "cmp_8005_negative_skips_table",
    "timer_one_expires_path_zero_offense",
    "timer_zero_expires_path_nonzero_defense",
    "timer_8002_expires", "timer_8001_wraps_positive",
    "valid_expiry_always_clears_live_82",
)
EXPECTED_EXECUTED = {
    "86a5b0", "86a5b3", "86a5b5", "86a5b8", "86a5ba", "86a5bd",
    "86a5bf", "86a5c1", "86a5c3", "86a5c6", "86a5c7", "86a5c8",
    "86a5cc", "86a5ce", "86a5cf", "86a5d2", "86a5d5", "86a5d7",
    "86a5d9", "86a5db", "86a5de", "86a5e0", "86a5e3", "86a5e5",
    "86a5e8", "86a5e9", "86a5eb", "86a5ee", "86a5f0", "86a5f3",
    "86a5f5", "86a5f9", "86a5fb", "86a5ff", "86a601", "86a605",
    "86a608", "86a60b", "86a60d", "86a610", "86a613", "86a616",
    "86a619", "86a61c", "86a61f", "86a622", "86a625", "86a628",
}
RESTORING = {
    "invalid_live_82_preserves", "invalid_live_81_clears",
    "timer_one_expires_path_zero_offense",
    "timer_zero_expires_path_nonzero_defense", "timer_8002_expires",
    "valid_expiry_always_clears_live_82",
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
        raise ValueError("mode-ten fixture identity/schema/count changed")

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
        unexpected = [
            field for field, initial, final in
            zip(EXPECTED_FIELD_NAMES, inputs, expected)
            if field not in MUTABLE_FIELDS and initial != final
        ]
        if unexpected:
            raise ValueError(
                f"case {number} changes fields outside the native write set: "
                f"{unexpected}")
        executed = call.get("executed")
        if call.get("entry_pc") != "86a5b0" or \
                call.get("exit_pc") != "86a628" or \
                not isinstance(executed, list) or not executed or \
                executed[0] != "86a5b0" or executed[-1] != "86a628":
            raise ValueError(f"case {number} has an invalid native boundary")
        executed_union.update(executed)
        expected_children = {
            "restore_869846": 1 if call["name"] in RESTORING else 0
        }
        if call.get("child_calls") != expected_children:
            raise ValueError(f"case {number} has changed native child counts")
        payload.extend(struct.pack("<" + "H" * EXPECTED_FIELDS, *inputs))
    if executed_union != EXPECTED_EXECUTED:
        raise ValueError(
            f"native instruction-start union changed: "
            f"{len(executed_union)}/48 starts")

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
        f"[CPU MODE TEN] {'PASS' if not mismatches else 'FAIL'}: "
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
            f"mode-ten production scheduler check exited "
            f"{integration.returncode}")
    if "[CPU MODE TEN INTEGRATION] PASS" not in integration.stdout.splitlines():
        raise AssertionError("mode-ten production caller check did not pass")
    print("[CPU MODE TEN INTEGRATION] PASS")


if __name__ == "__main__":
    main()
