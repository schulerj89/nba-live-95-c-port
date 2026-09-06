"""Strictly replay compact native `$86:F6CD-$F793` mode-two witnesses."""

import argparse
import hashlib
import json
import struct
import subprocess
from pathlib import Path

from normalize_cpu_mode_two_parent_vectors import CASE_NAMES, FIELDS

EXPECTED_ROUTINE = "$86:F6CD-$F793 mode-two parent"
EXPECTED_ROM = "2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870"
EXPECTED_STARTS = "3dcf5ee5a8c6d883acbaf57dd981962a08c51889b7ee52e75095aafb888a23b2"
EXPECTED_CALLS = "7c42e827f1f41df7ebf881fc4bad97243dce9f7ca352b23955e8f98e8beda3c5"
EXPECTED_FIELDS = "ff57aaa7cb3f0f0915ce6d557025d7a4ad8adb74c5789d2c991103408b3b04b4"
EXPECTED_FIXTURE = "6bdd4b38ff593f3ae736682355bf320f93eec657030c86718425c1ba1d10a02e"
EXPECTED_PROVENANCE = (
    "two controlled genuine `$87:9C21` dispatcher-entry Mesen captures with "
    "identical represented projections; documented WRAM inputs only; CPU state, "
    "stack, CPU flags, ROM and RNG were never written; no child result was written "
    "during the observed parent; the native exit was recorded before restoring "
    "original WRAM controls and state")
EXPECTED_DOMAIN = (
    "represented parent entry/exit state plus direct gameplay child outputs; both "
    "native team contexts and assigned opponents are represented across slots 1-4 "
    "and 6-9; natural controller records 0, 4 and $FFFF are covered, while "
    "$7FFF/$8000 are explicit signed-word boundary witnesses rather than retail "
    "controller IDs; these captures hold offense-group $093A at $FFFF, and the "
    "replay adapter supports canonical 0, 5 and $FFFF while mapping the host byte "
    "sentinel back to native $FFFF; paired +$92 covers 0, 3, "
    "$8000, $8002 and $8003; period $0926, difficulty $17AF and both context +$08 "
    "fractions are fixed zero; actor/paired +$00 identities are provenance metadata "
    "rather than replayed outputs; `$092E` has no storage in NbaTipoff and is "
    "excluded because none of these live-state-two paths writes it; lineup words "
    "and pack-backed decision/shot profiles establish roster inputs without "
    "embedding ROM data; DP address/arithmetic scratch and unmodeled temporary "
    "globals are excluded")
EXPECTED_SOURCE = {
    "vectors_sha256": "f700bf22c2e2d7616ca2e064f2b3f3a2437cbcf467e7f650bcc9574a91441e69",
    "repeat_vectors_sha256": "8f7a27063988d301c2579a196c8136c0d3a4ca67faca49d40ad2b5626546bb67",
    "cases_sha256": "582f5a624423d28e6970e2bcb9c7b44b9c73bb9a3614516856653b8cad9cd913",
    "paths_sha256": "84535e6969a2988a66da42a088008f2c47ac3b6b144d76bd1bb1f5085437f46b",
    "capture_provenance_sha256": "024b99ca813b1a1689b362181d1d5dcf5a0bbaf0c444595c5a4188f7a4e1e9b1",
    "capture_lua_sha256": "66f6729591937de705f9c311384271b95be1d5d0e9d02ea3bc1bf63d7c422304",
    "capture_runner_sha256": "8fb04e9797bc69f1387569c0473321b403b6fb851c99e2f63285a1e442c37c42",
    "vector_recorder_sha256": "406652c21ed58e31d9f7adb09feea18a4155d76a074498aca23c53a07f7cc7f7",
    "ghidra_functions_sha256": "c3ce476090ee9a9ec970ce9b9ccb2fdeea42936f8a3ff9184880b609fff3b199",
    "ghidra_instructions_sha256": "f642199de28e016061db579f28a28f60c772de70a6a2a6712ded609e03f7d63c",
}
EXPECTED_AGGREGATE = {
    "repair_86e3cb": 1, "loose_86f0fd": 2,
    "target_mode3_86e6b7": 2, "target_weak_86e7b3": 5,
    "target_normal_86e7dc": 20, "target_role_86e96f": 4,
    "jump_86ec32": 10, "pose_86e3e1": 30,
    "steer_85b3aa": 1, "accelerate_85a82c": 10,
    "finalize_86e5ab": 0, "approach_static_85b3c9": 8,
    "approach_moving_85b402": 1, "pose_animation_87b37c": 1,
}
TOP_KEYS = {"schema", "routine", "rom_sha256", "provenance", "domain", "fields",
            "owned_starts_sha256", "aggregate_child_calls", "source", "calls"}
CALL_KEYS = {"call", "name", "entry_pc", "exit_pc", "native_actor_id",
             "native_paired_id", "executed", "child_calls", "input", "expected"}


def compact_hash(value) -> str:
    return hashlib.sha256(json.dumps(value, separators=(",", ":")).encode()).hexdigest()


def validate(document: dict) -> tuple[bytearray, list[dict], list[str]]:
    if type(document) is not dict or set(document) != TOP_KEYS:
        raise ValueError("mode-two parent fixture shape changed")
    calls = document.get("calls", [])
    fields = document.get("fields", [])
    if (type(document.get("schema")) is not int or document.get("schema") != 1 or
            document.get("routine") != EXPECTED_ROUTINE or
            document.get("rom_sha256") != EXPECTED_ROM or
            document.get("provenance") != EXPECTED_PROVENANCE or
            document.get("domain") != EXPECTED_DOMAIN or
            document.get("owned_starts_sha256") != EXPECTED_STARTS or
            document.get("aggregate_child_calls") != EXPECTED_AGGREGATE or
            document.get("source") != EXPECTED_SOURCE or tuple(fields) != FIELDS or
            compact_hash(fields) != EXPECTED_FIELDS or
            tuple(c.get("name") for c in calls) != CASE_NAMES or
            compact_hash(calls) != EXPECTED_CALLS):
        raise ValueError("mode-two parent fixture identity/provenance changed")
    payload = bytearray()
    union: set[str] = set()
    aggregate: dict[str, int] = {}
    for number, call in enumerate(calls, 1):
        if type(call) is not dict or set(call) != CALL_KEYS:
            raise ValueError(f"case {number} shape changed")
        before = call.get("input", [])
        expected = call.get("expected", [])
        identities = (call.get("native_actor_id"), call.get("native_paired_id"))
        if (type(before) is not list or type(expected) is not list or
                len(before) != len(FIELDS) or len(expected) != len(FIELDS) or any(
                    type(v) is not int or not 0 <= v <= 0xFFFF
                    for v in before + expected + list(identities))):
            raise ValueError(f"case {number} has an invalid field vector")
        executed = call.get("executed", [])
        children = call.get("child_calls", {})
        if (call.get("call") != number or call.get("entry_pc") != "86f6cd" or
                call.get("exit_pc") != "86f793" or type(executed) is not list or
                not executed or executed[0] != "86f6cd" or executed[-1] != "86f793" or
                any(type(pc) is not str or len(pc) != 6 for pc in executed) or
                type(children) is not dict or set(children) != set(EXPECTED_AGGREGATE) or
                any(type(v) is not int or v < 0 for v in children.values())):
            raise ValueError(f"case {number} has an invalid native boundary")
        union.update(executed)
        for child, count in children.items():
            aggregate[child] = aggregate.get(child, 0) + count
        payload.extend(struct.pack("<" + "H" * len(FIELDS), *before))
    union_hash = hashlib.sha256("\n".join(sorted(union)).encode()).hexdigest()
    if (len(calls) != 43 or len(fields) != 197 or len(union) != 81 or
            union_hash != EXPECTED_STARTS or aggregate != EXPECTED_AGGREGATE):
        raise ValueError("mode-two parent path/child contract changed")
    return payload, calls, fields


def replay(probe: Path, pack: Path, payload: bytes, fields: list[str],
           calls: list[dict]) -> list[str]:
    run = subprocess.run([str(probe), str(pack)], input=payload, capture_output=True)
    if run.returncode:
        raise AssertionError(run.stderr.decode(errors="replace"))
    lines = []
    for line in run.stdout.decode().splitlines():
        if not line:
            continue
        if line.startswith(("[ASSETS] ", "[TIPOFF] ")):
            continue
        if len(line.split()) != len(fields):
            raise AssertionError("probe emitted malformed or unexpected output")
        lines.append(line)
    if len(lines) != len(calls):
        raise AssertionError(f"probe returned {len(lines)}/{len(calls)} rows")
    return lines


def reject_partial_record(probe: Path, pack: Path) -> None:
    run = subprocess.run([str(probe), str(pack)], input=b"\0", capture_output=True)
    if run.returncode == 0:
        raise AssertionError("probe accepted a trailing partial input record")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--vectors", type=Path, required=True)
    parser.add_argument("--probe", type=Path, required=True)
    parser.add_argument("--pack", type=Path, required=True)
    parser.add_argument("--skip-caller", action="store_true")
    args = parser.parse_args()
    raw = args.vectors.read_bytes()
    if hashlib.sha256(raw).hexdigest() != EXPECTED_FIXTURE:
        raise ValueError("mode-two parent fixture byte hash changed")
    payload, calls, fields = validate(json.loads(raw))
    reject_partial_record(args.probe, args.pack)
    lines = replay(args.probe, args.pack, payload, fields, calls)
    mismatches = []
    for number, (call, line) in enumerate(zip(calls, lines), 1):
        try:
            actual = [int(word, 16) for word in line.split()]
        except ValueError as error:
            raise AssertionError(f"case {number} emitted a non-hex word") from error
        if any(not 0 <= value <= 0xFFFF for value in actual):
            raise AssertionError(f"case {number} emitted an out-of-range word")
        differences = [(fields[i], want, got) for i, (want, got) in
                       enumerate(zip(call["expected"], actual)) if want != got]
        if differences:
            mismatches.append((number, call["name"], differences))
    print(f"[CPU MODE TWO PARENT] {'PASS' if not mismatches else 'FAIL'}: "
          f"calls={len(calls)} fields={len(fields)} mismatches={len(mismatches)}")
    for number, name, differences in mismatches:
        print(f"case {number} {name}: {differences[:24]}")
    if mismatches:
        raise SystemExit(1)
    if not args.skip_caller:
        caller = subprocess.run([str(args.probe), str(args.pack), "--self-test"],
                                capture_output=True, text=True)
        if caller.returncode or "[CPU MODE TWO PARENT CALLER] PASS" not in caller.stdout:
            print(caller.stdout, end="")
            print(caller.stderr, end="")
            raise AssertionError("production caller check failed")
        print("[CPU MODE TWO PARENT CALLER] PASS")


if __name__ == "__main__":
    main()
