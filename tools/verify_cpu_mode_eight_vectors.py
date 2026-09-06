"""Strictly replay compact native `$86:C6AD-$C758` witnesses in production C."""

import argparse
import json
import struct
import subprocess
from pathlib import Path

from normalize_cpu_mode_eight_vectors import FIELDS as EXPECTED_FIELD_NAMES


EXPECTED_FIELDS = 27
EXPECTED_CALLS = 19
EXPECTED_ROUTINE = "$86:C6AD-$C758 mode-eight knockdown recovery"
EXPECTED_ROM_SHA256 = (
    "2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870"
)
EXPECTED_NAMES = (
    "selector_negative", "vertical_nonzero", "airborne_integer",
    "fractional_z_lands", "bounce_signed_halves", "settle_negative_marker",
    "timer_two_holds", "timer_one_owner_restores",
    "timer_zero_group_match_restores",
    "timer_8002_group_mismatch_restores", "timer_8001_wraps_positive",
    "phase_nine_clears", "phase_ten_sets_10", "phase_nineteen_sets_10",
    "phase_twenty_sets_08", "phase_twentynine_sets_08",
    "phase_thirty_sets_10", "phase_thirtynine_sets_10",
    "phase_forty_clears",
)
EXPECTED_EXECUTED = {
    "86c6ad", "86c6af", "86c6b2", "86c6b5", "86c6b8", "86c6bb",
    "86c6bd", "86c6c0", "86c6c2", "86c6c5", "86c6c7", "86c6ca",
    "86c6cc", "86c6cf", "86c6d2", "86c6d5", "86c6d8", "86c6db",
    "86c6de", "86c6e1", "86c6e2", "86c6e5", "86c6e8", "86c6eb",
    "86c6ec", "86c6ef", "86c6f1", "86c6f4", "86c6f7", "86c6fa",
    "86c6fd", "86c700", "86c701", "86c703", "86c706", "86c707",
    "86c70a", "86c70c", "86c70d", "86c710", "86c712", "86c715",
    "86c718", "86c71a", "86c71d", "86c71f", "86c722", "86c724",
    "86c727", "86c72a", "86c72c", "86c72f", "86c732", "86c734",
    "86c737", "86c73a", "86c73d", "86c740", "86c742", "86c746",
    "86c748", "86c74b", "86c74d", "86c750", "86c752", "86c755",
    "86c758",
}
RESTORE_NAMES = {
    "timer_one_owner_restores", "timer_zero_group_match_restores",
    "timer_8002_group_mismatch_restores",
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
        raise ValueError("mode-eight fixture identity/schema/count changed")

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
        executed = call.get("executed")
        if call.get("entry_pc") != "86c6ad" or \
                call.get("exit_pc") != "86c758" or \
                not isinstance(executed, list) or not executed or \
                executed[0] != "86c6ad" or executed[-1] != "86c758":
            raise ValueError(f"case {number} has an invalid native boundary")
        executed_union.update(executed)
        expected_children = {
            "restore_869846": 1 if call["name"] in RESTORE_NAMES else 0,
        }
        if call.get("child_calls") != expected_children:
            raise ValueError(f"case {number} has changed native child counts")
        for field in ("scratch_raw_0046", "scratch_raw_0047",
                      "x_fraction_raw_02", "x_integer_raw_04",
                      "y_fraction_raw_06", "y_integer_raw_08",
                      "selector_raw_56",
                      "recovery_inhibit_raw_7a", "alternate_lower_raw_a8"):
            index = fields.index(field)
            if expected[index] != inputs[index]:
                raise ValueError(f"case {number} unexpectedly changes {field}")
        payload.extend(struct.pack("<" + "H" * EXPECTED_FIELDS, *inputs))
    if executed_union != EXPECTED_EXECUTED:
        raise ValueError(
            f"native instruction-start union changed: "
            f"{len(executed_union)}/67 starts")

    fractional = calls[EXPECTED_NAMES.index("fractional_z_lands")]
    if fractional["input"][fields.index("z_fraction_raw_0a")] == 0 or not (
            fractional["expected"][fields.index("velocity_z_raw_12")] == 0x00F0 and
            fractional["expected"][fields.index("landing_marker_raw_66")] == 0xFFFF):
        raise ValueError("fractional-Z landing witness changed")

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
        f"[CPU MODE EIGHT] {'PASS' if not mismatches else 'FAIL'}: "
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
            f"mode-eight production scheduler check exited "
            f"{integration.returncode}")
    if "[CPU MODE EIGHT INTEGRATION] PASS" not in integration.stdout.splitlines():
        raise AssertionError("mode-eight production caller check did not pass")
    print("[CPU MODE EIGHT INTEGRATION] PASS")


if __name__ == "__main__":
    main()
