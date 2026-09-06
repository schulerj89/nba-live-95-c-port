"""Strictly replay compact native `$86:B769-$B978` shooter witnesses."""

import argparse
import hashlib
import json
import struct
import subprocess
from pathlib import Path

from normalize_cpu_mode_twelve_vectors import CASE_NAMES, FIELDS
from normalize_cpu_mode_twelve_ft_vectors import (
    CASE_NAMES as FT_CASE_NAMES, INPUT_FIELDS as FT_INPUT_FIELDS,
    OUTPUT_FIELDS as FT_OUTPUT_FIELDS,
)


EXPECTED_ROUTINE = "$86:B769-$B978 mode-twelve shooter"
EXPECTED_ROM = "2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870"
EXPECTED_STARTS_SHA256 = "8687118903ea44ad124933f7bec71f23e203cef0133072afedb8b71a610502a6"
EXPECTED_CALLS_SHA256 = "7ea9c8c8cf6ba5d66257f35c8501dd970247f2144f293daa8e1ea14a4552e6c0"
EXPECTED_PROVENANCE = ("two controlled genuine-entry Mesen captures with "
    "identical relevant projections; documented WRAM inputs only; no PC, "
    "stack, ROM, RNG, or child-result patching; unrelated volatile bytes in "
    "the widened player-record window are excluded")
EXPECTED_SOURCE = {
    "vectors_sha256": "0156643bfbd1c72cdd623a217dff36d84b63da33128f6d60a692ea7d8f6f2c7b",
    "repeat_vectors_sha256": "99bafc61ad3a1c5d474bc6c406907cf0257f848aad6afafc30c86957f1d8b605",
    "cases_sha256": "9d077f1c3361ecbe54433495f557e4a945d7945faf9108643a6a8dd1cc1f4eeb",
    "paths_sha256": "bf2b05615ccc2692ef60c8847d3cf4697313206fc0900225cb50aad08ccaf5f8",
    "ghidra_functions_sha256": "c3ce476090ee9a9ec970ce9b9ccb2fdeea42936f8a3ff9184880b609fff3b199",
    "ghidra_instructions_sha256": "f642199de28e016061db579f28a28f60c772de70a6a2a6712ded609e03f7d63c",
}
EXPECTED_AGGREGATE = {"attach_87b832": 32, "direction_85f02d": 11,
    "cancel_lower_87b555": 11, "animation_87b4db": 10,
    "restore_869846": 2, "cancel_upper_87b538": 1, "launch_869d6e": 6}
LAUNCH = {28, 30, 31, 32, 33, 36}
DIRECTION = {17, 21, 22, 26, 27, 28, 30, 31, 32, 33, 36}
CANCEL_LOWER = {5, 11, 12, 15, 16, 17, 18, 19, 20, 21, 22}
ANIMATION = {11, 12, 15, 16, 17, 18, 19, 20, 21, 22}
EXITS = {
    **{n: "86b86b" for n in (1, 2)}, **{n: "86b790" for n in (3, 4)},
    **{n: "86b8c8" for n in (5, 28, 30, 31, 32, 33, 36)},
    **{n: "86b8c9" for n in (6, 7, 9, 10)},
    **{n: "86b88f" for n in (8, 23, 24)},
    **{n: "86b866" for n in (11, 12, 15, 16, 17, 18, 19, 20, 21, 22)},
    **{n: "86b978" for n in (13, 14, 25, 26, 27, 29, 34, 35, 37)},
}
FT_ROUTINE = "$87:9F11-$9F75 free-throw mode-twelve caller"
FT_CALLS_SHA256 = "4acdf1bd6cfcbd3e2ad9a2a56a6d8d0fb1409c7e44064e626b202a09b2ee8b0d"
FT_PROVENANCE = ("repeated genuine-entry Mesen captures at $87:9F11 and "
    "$87:9F3E with matching entry/exit stack; the two-frame release includes "
    "native $85:95DB graphics/DMA work, whose volatile/NMI state is excluded "
    "from this caller-owned projection")
FT_SCOPE = ("caller-owned gameplay words only; player record $416B was "
    "outside the captured ranges, so player statistics/stamina are excluded; "
    "parent witnesses cover the child mutation contract for captured player "
    "$44EB, while exact $416B values remain outside")
FT_SOURCE = {
    "prep_sha256": "629e61f39258010c247bdb681ffec79efd59c863d484f8c966aab7e849a61451",
    "prep_repeat_sha256": "629e61f39258010c247bdb681ffec79efd59c863d484f8c966aab7e849a61451",
    "state9_sha256": "0c49d14761a42ea3071f6b4473032dcdf3628ec5be92e8b61bce7c489fcda8dd",
    "state9_repeat_sha256": "0c49d14761a42ea3071f6b4473032dcdf3628ec5be92e8b61bce7c489fcda8dd",
    "final_sha256": "4a65d9ccf4c7a1e1bcec398bdb599dd8a7de7180a06e9d3a87a3272da003a6b0",
    "final_repeat_sha256": "4a65d9ccf4c7a1e1bcec398bdb599dd8a7de7180a06e9d3a87a3272da003a6b0",
}
FT_BOUNDARIES = (
    ("879f11", 4362, 4362, 807),
    ("879f3e", 4362, 4362, 500),
    ("879f3e", 4364, 4366, 109427),
    ("879f3e", 4364, 4366, 109426),
)


def expected_children(number):
    return {"attach_87b832": int(number >= 6),
        "direction_85f02d": int(number in DIRECTION),
        "cancel_lower_87b555": int(number in CANCEL_LOWER),
        "animation_87b4db": int(number in ANIMATION),
        "restore_869846": int(number in (1, 2)),
        "cancel_upper_87b538": int(number == 5),
        "launch_869d6e": int(number in LAUNCH)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--vectors", type=Path, required=True)
    parser.add_argument("--caller-vectors", type=Path, required=True)
    parser.add_argument("--probe", type=Path, required=True)
    parser.add_argument("--pack", type=Path, required=True)
    args = parser.parse_args()
    document = json.loads(args.vectors.read_text())
    calls = document.get("calls", [])
    if document.get("schema") != 1 or document.get("routine") != EXPECTED_ROUTINE or \
            document.get("rom_sha256") != EXPECTED_ROM or tuple(document.get("fields", ())) != FIELDS or \
            tuple(call.get("name") for call in calls) != CASE_NAMES or len(calls) != 37 or \
            document.get("owned_starts_sha256") != EXPECTED_STARTS_SHA256 or \
            document.get("aggregate_child_calls") != EXPECTED_AGGREGATE or \
            document.get("provenance") != EXPECTED_PROVENANCE or \
            document.get("source") != EXPECTED_SOURCE:
        raise ValueError("mode-twelve fixture identity/schema/count changed")
    calls_hash = hashlib.sha256(json.dumps(
        calls, separators=(",", ":"), ensure_ascii=True).encode()).hexdigest()
    if calls_hash != EXPECTED_CALLS_SHA256:
        raise ValueError("mode-twelve normalized call payload changed")
    payload = bytearray()
    executed_union = set()
    for number, call in enumerate(calls, 1):
        before, after = call.get("input", []), call.get("expected", [])
        if call.get("call") != number or len(before) != len(FIELDS) or \
                len(after) != len(FIELDS) or any(
                    type(v) is not int or not 0 <= v <= 0xffff
                    for v in before + after):
            raise ValueError(f"case {number} has an invalid field vector")
        executed = call.get("executed", [])
        if call.get("entry_pc") != "86b769" or call.get("exit_pc") != EXITS[number] or \
                not executed or executed[0] != "86b769" or executed[-1] != EXITS[number] or \
                call.get("child_calls") != expected_children(number):
            raise ValueError(f"case {number} changed its native boundary or child calls")
        executed_union.update(executed)
        payload.extend(struct.pack("<" + "H" * len(FIELDS), *before))
    starts_hash = hashlib.sha256("\n".join(sorted(executed_union)).encode()).hexdigest()
    if len(executed_union) != 213 or starts_hash != EXPECTED_STARTS_SHA256:
        raise ValueError("native instruction-start set changed")
    run = subprocess.run([str(args.probe), str(args.pack)], input=payload,
                         capture_output=True, check=True)
    lines = [line for line in run.stdout.decode().splitlines()
             if line.strip() and not line.startswith("[")]
    if len(lines) != 37:
        raise AssertionError(f"probe returned {len(lines)} rows, expected 37")
    mismatches = []
    for number, (call, line) in enumerate(zip(calls, lines), 1):
        words = line.split()
        if len(words) != len(FIELDS):
            raise AssertionError(f"case {number} returned {len(words)} fields")
        actual = [int(word, 16) for word in words]
        differences = [(FIELDS[index], wanted, got) for index, (wanted, got)
                       in enumerate(zip(call["expected"], actual)) if wanted != got]
        if differences:
            mismatches.append((number, call["name"], differences))
    print(f"[CPU MODE TWELVE] {'PASS' if not mismatches else 'FAIL'}: "
          f"calls=37 fields={len(FIELDS)} starts=213 exits=7 mismatches={len(mismatches)}")
    for number, name, differences in mismatches:
        print(f"case {number} {name}: {differences[:12]}")
    if mismatches:
        raise SystemExit(1)

    caller_document = json.loads(args.caller_vectors.read_text())
    caller_calls = caller_document.get("calls", [])
    if caller_document.get("schema") != 1 or \
            caller_document.get("routine") != FT_ROUTINE or \
            caller_document.get("rom_sha256") != EXPECTED_ROM or \
            tuple(caller_document.get("input_fields", ())) != FT_INPUT_FIELDS or \
            tuple(caller_document.get("output_fields", ())) != FT_OUTPUT_FIELDS or \
            tuple(call.get("name") for call in caller_calls) != FT_CASE_NAMES or \
            caller_document.get("provenance") != FT_PROVENANCE or \
            caller_document.get("scope") != FT_SCOPE or \
            caller_document.get("source") != FT_SOURCE or \
            caller_document.get("calls_sha256") != FT_CALLS_SHA256:
        raise ValueError("mode-twelve free-throw caller fixture identity changed")
    caller_hash = hashlib.sha256(json.dumps(
        caller_calls, separators=(",", ":"), ensure_ascii=True).encode()).hexdigest()
    if caller_hash != FT_CALLS_SHA256:
        raise ValueError("mode-twelve free-throw caller payload changed")
    caller_mismatches = []
    for number, (call, boundary) in enumerate(
            zip(caller_calls, FT_BOUNDARIES), 1):
        before, after = call.get("input", []), call.get("expected", [])
        entry_pc, entry_frame, exit_frame, cycles = boundary
        if call.get("call") != number or len(before) != len(FT_INPUT_FIELDS) or \
                len(after) != len(FT_OUTPUT_FIELDS) or any(
                    type(value) is not int or not 0 <= value <= 0xffff
                    for value in before + after) or \
                call.get("entry_pc") != entry_pc or \
                call.get("exit_pc") != "87a017" or \
                call.get("entry_frame") != entry_frame or \
                call.get("exit_frame") != exit_frame or \
                call.get("cycle_count") != cycles or \
                call.get("stack_pointer") != 8185:
            raise ValueError(f"free-throw caller case {number} changed shape/boundary")
        mode = "--ft-prep" if entry_pc == "879f11" else "--ft-state9"
        packed = struct.pack("<" + "H" * len(before), *before)
        replay = subprocess.run([str(args.probe), str(args.pack), mode],
                                input=packed, capture_output=True, check=True)
        replay_lines = [line for line in replay.stdout.decode().splitlines()
                        if line.strip() and not line.startswith("[")]
        if len(replay_lines) != 1:
            raise AssertionError(f"caller case {number} returned an invalid row count")
        actual = [int(word, 16) for word in replay_lines[0].split()]
        if len(actual) != len(FT_OUTPUT_FIELDS):
            raise AssertionError(f"caller case {number} returned {len(actual)} fields")
        differences = [(FT_OUTPUT_FIELDS[index], wanted, got)
                       for index, (wanted, got) in enumerate(zip(after, actual))
                       if wanted != got]
        if differences:
            caller_mismatches.append((number, call["name"], differences))
    print(f"[CPU MODE TWELVE FT] {'PASS' if not caller_mismatches else 'FAIL'}: "
          f"calls=4 inputs={len(FT_INPUT_FIELDS)} outputs={len(FT_OUTPUT_FIELDS)} "
          f"mismatches={len(caller_mismatches)}")
    for number, name, differences in caller_mismatches:
        print(f"caller case {number} {name}: {differences[:12]}")
    if caller_mismatches:
        raise SystemExit(1)
    stale = calls[27]
    stale_payload = struct.pack("<" + "H" * len(FIELDS), *stale["input"])
    stale_run = subprocess.run(
        [str(args.probe), str(args.pack), "--stale-mirror"],
        input=stale_payload, capture_output=True, check=True)
    stale_lines = [line for line in stale_run.stdout.decode().splitlines()
                   if line.strip() and not line.startswith("[")]
    if len(stale_lines) != 1:
        raise AssertionError("stale-mirror integration returned an invalid row count")
    stale_words = stale_lines[0].split()
    stale_actual = [int(word, 16) for word in stale_words]
    if stale_actual != stale["expected"]:
        raise AssertionError("stale actor mirror overrode the authoritative roster record")
    stale_owner_run = subprocess.run(
        [str(args.probe), str(args.pack), "--stale-ball-owner"],
        input=stale_payload, capture_output=True, check=True)
    stale_owner_lines = [line for line in stale_owner_run.stdout.decode().splitlines()
                         if line.strip() and not line.startswith("[")]
    if len(stale_owner_lines) != 1 or [int(word, 16) for word in
            stale_owner_lines[0].split()] != stale["expected"]:
        raise AssertionError("stale host ball owner overrode raw possession publication")
    print("[CPU MODE TWELVE INTEGRATION] PASS: authoritative roster stats and "
          "raw possession win over stale host mirrors")
    caller = subprocess.run([str(args.probe), str(args.pack), "--self-test"],
                            capture_output=True, text=True)
    print(caller.stdout, end="")
    if caller.returncode != 0 or "[CPU MODE TWELVE CALLER] PASS" not in caller.stdout:
        print(caller.stderr, end="")
        raise AssertionError("mode-twelve free-throw caller integration failed")


if __name__ == "__main__":
    main()
