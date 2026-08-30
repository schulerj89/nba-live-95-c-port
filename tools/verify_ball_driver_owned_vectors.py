"""Check native owned wrapper/core calls and high-wrapper host bindings."""

import argparse
from collections import Counter
import json
from pathlib import Path
import subprocess

from normalize_ball_driver_owned_vectors import (
    INPUT_FIELDS, NONISOLATED_EVENT, OUTPUT_FIELDS, ROM_SHA256, SOURCE_SHA256,
    normalized_rom_hash,
)


def fail(message):
    raise SystemExit(f"[OWNED BALL DRIVER] FAIL: {message}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--vectors", required=True)
    parser.add_argument("--probe", required=True)
    parser.add_argument("--assets", required=True)
    parser.add_argument("--rom", required=True)
    args = parser.parse_args()
    fixture = json.loads(Path(args.vectors).read_text())
    if normalized_rom_hash(args.rom) != ROM_SHA256 or any(
            fixture.get(key) != value for key, value in {
                "schema": 2, "rom_sha256": ROM_SHA256,
                "source_vectors_sha256": SOURCE_SHA256,
                "source_entry": "859A24", "native_entry": "859A37",
                "native_exit": "85A7C7", "input_fields": INPUT_FIELDS,
                "output_fields": OUTPUT_FIELDS,
            }.items()):
        fail("fixture/ROM/boundary identity mismatch")
    cases = fixture.get("cases", [])
    if len(cases) != 324 or Counter(case.get("kind") for case in cases) != \
            {"project": 66, "preserve": 42, "low-reset": 85, "low-fall": 131}:
        fail("unexpected owned wrapper/core branch census")
    calls = set()
    replays = []
    partial_cases = 0
    for case in cases:
        values, expected = case.get("input"), case.get("expected")
        call = case.get("source_call")
        if not isinstance(call, int) or isinstance(call, bool) or call <= 0 or \
                call in calls or not isinstance(values, list) or len(values) != 47 or \
                not isinstance(expected, list) or len(expected) != 31 or \
                any(not isinstance(v, int) or isinstance(v, bool) or
                    not 0 <= v <= 0xFFFF for v in values + expected):
            fail("malformed or duplicated native case")
        calls.add(call)
        kind = ("preserve" if values[27] in (15, 17) else "project") \
            if values[23] >= 0xF0 else \
            ("low-reset" if values[26] < 3 else "low-fall")
        if values[0] >= 10 or case["kind"] != kind:
            fail(f"call {call}: invalid ownership/resource/mode branch")
        exclusion = case.get("nonisolated_output")
        if call == NONISOLATED_EVENT["source_call"]:
            if exclusion != NONISOLATED_EVENT or \
                    case.get("source_frame") != NONISOLATED_EVENT["source_frame"] or \
                    case.get("source_exit_frame") != NONISOLATED_EVENT["source_exit_frame"] or \
                    kind != "low-fall" or values[0] != 8 or \
                    values[12:14] != [0x4800, 1] or values[16] != 0xFD50 or \
                    values[23] != 0x008E or values[26:28] != [4, 11] or \
                    values[43] != 0 or expected[11:13] != [0xFE00, 1] or \
                    expected[15] != 0x01FE or expected[26:28] != [0xFD38, 0]:
                fail("the narrowly scoped nonisolated event witness changed")
            partial_cases += 1
        elif exclusion is not None:
            fail(f"call {call}: unauthorized native-output exclusion")
        same = values[:7] + values[8:17] + values[32:47]
        if kind.startswith("low-"):
            preserved = {0, 1, 2, 6, 7, 9, 28, *range(16, 26)}
            if values[13] >= 73 or values[44] not in (0, 0x82) or \
                    any(expected[i] for i in (5, 13, 14)):
                fail(f"call {call}: invalid low-owned native scope/tail")
        else:
            mutable = {8, 10, 12, 29} if kind == "project" else set()
            preserved = set(range(len(expected))) - mutable
        if kind == "project" and expected[29] != values[9] or \
                kind.startswith("low-") and expected[29] != values[18]:
            fail(f"call {call}: fixture violates native 0922 source contract")
        if any(same[i] != expected[i] for i in preserved):
            fail(f"call {call}: fixture violates native preservation contract")
        if kind.startswith("low-"):
            # Low-resource cases compare captured native words only. Their
            # ordinary host caches are initialized without claiming an oracle
            # for host-only labels or cache normalization.
            bindings = [4, values[0], values[0]]
            replays.append((call, 4, values + bindings, expected, exclusion))
            continue
        # These labels/caches have no native WRAM counterparts. Vary them to
        # prove production selection depends on 093E/resource/mode only, and
        # that a coordinate-only native wrapper does not mutate host ownership.
        handler = (values[0] + 5) % 10
        for host_mode, logical_owner in ((4, 0xFFFF), (5, values[0])):
            bindings = [host_mode, logical_owner, handler]
            binding_input, binding_expected = list(values), list(expected)
            if host_mode == 5:
                # Negative controls for the statically/native-established
                # snapshot contract, not additional native captures. The
                # project branch must ignore poisoned old0922 and replace it
                # with old BALL X; preserve must retain it. Both high paths
                # must retain poisoned0924. The durable oracle stays raw.
                binding_input[45] ^= 0xA55A
                binding_input[46] ^= 0x5AA5
                if kind == "preserve":
                    binding_expected[29] = binding_input[45]
                binding_expected[30] = binding_input[46]
            replays.append((call, host_mode, binding_input + bindings,
                            binding_expected + bindings, None))
    if partial_cases != 1:
        fail("expected exactly one explicitly documented partial native case")
    payload = "".join(" ".join(f"{v:04x}" for v in row[2]) + "\n"
                      for row in replays)
    run = subprocess.run([args.probe, args.assets], input=payload, text=True,
                         capture_output=True)
    if run.returncode:
        fail(f"probe exit {run.returncode}: {run.stderr}")
    lines = [line for line in run.stdout.splitlines() if line.strip() and
             not line.startswith("[ASSETS] Loaded asset pack:")]
    if len(lines) != len(replays):
        fail(f"probe returned {len(lines)} lines, expected {len(replays)}")
    names = OUTPUT_FIELDS + ["host_ball_mode", "host_logical_owner", "host_handler"]
    mismatches = []
    for (call, mode, _, expected, exclusion), line in zip(replays, lines):
        try:
            actual = [int(v, 16) for v in line.split()]
        except ValueError:
            fail(f"call {call}: non-hex probe output {line!r}")
        if len(actual) != 34:
            fail(f"call {call}: output width {len(actual)}, expected 34")
        actual = actual[:len(expected)]
        if exclusion is not None:
            # Do not substitute a derived value into the raw native expected
            # row. This separate assertion is static producer evidence only.
            index = names.index(exclusion["field"])
            if actual[index] != exclusion["isolated_producer_value"]:
                fail(f"call {call}: isolated event producer is {actual[index]:04x}, "
                     f"expected instruction-derived {exclusion['isolated_producer_value']:04x}")
            print(f"[OWNED BALL DRIVER] PARTIAL: native-call={call} "
                  f"excluded={exclusion['field']} captured={expected[index]:04x} "
                  f"C={actual[index]:04x}; interrupt consumption is inferred, not proven")
        else:
            index = -1
        changes = [(names[i], f"{want:04x}", f"{got:04x}")
                   for i, (want, got) in enumerate(zip(expected, actual))
                   if want != got and i != index]
        if changes:
            mismatches.append((call, mode, changes))
    if mismatches:
        for call, mode, changes in mismatches[:12]:
            print(f"native-call={call} host-mode={mode}: {changes}")
        fail(f"replays={len(replays)} mismatches={len(mismatches)}")
    print(f"[OWNED BALL DRIVER] PASS: complete_native_cases={len(cases) - partial_cases} "
          f"partial_native_cases={partial_cases} complete_replays={len(replays) - partial_cases} "
          f"partial_replays={partial_cases} high_binding_replays=216 "
          "snapshot_negative_controls=108 "
          "unexpected_mismatches=0; one cross-frame 13E7 timing caveat remains")


if __name__ == "__main__":
    main()
