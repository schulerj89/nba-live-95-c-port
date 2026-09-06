"""Strictly replay compact native `$86:B154-$B334` mode-fourteen witnesses."""

import argparse
import hashlib
import json
import struct
import subprocess
from pathlib import Path

from normalize_cpu_mode_fourteen_vectors import CASE_NAMES, FIELDS

EXPECTED_ROUTINE = "$86:B154-$B334 mode-fourteen special receiver"
EXPECTED_ROM = "2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870"
EXPECTED_STARTS = "2ba100f987f03ddc4a7499dec07fcff87578ef71a202ee60ebfe90b726859d5f"
EXPECTED_CALLS = "5c111b890bfed66e82762d5f409cf791f14684bef237409e721635d90b967677"
EXPECTED_FIELDS = "02c38a6da38b04bfb5daed7c91bfd6be388e00cf1f72d799d4ca362d5572d35d"
EXPECTED_FIXTURE = "f26a26da6f6f750b3843caaf2d6ce1d519271b734b81e672784eb76cf5f1aec1"
EXPECTED_PROVENANCE = ("two controlled genuine-entry Mesen captures with identical relevant "
    "projections; documented WRAM inputs only; no PC, stack, ROM, RNG, or child-result "
    "patching; inline $86:A9B2-$A9CF and $86:B440-$B447 bytes excluded")
EXPECTED_DOMAIN = ("represented parent entry/exit state plus direct gameplay child outputs; "
    "actor +$00 is identity-only; native slot/id 5 -> $343F -> player $446B and active "
    "lineup $4779=2 map to host actor[5], context1, roster slot2/persistent index14 "
    "(pack roster address $AC:F636); controller $FFFF only; captured fixed inputs period "
    "$0926=0, difficulty $17AF=0, and both context +$08 basket fractions=0; all ten actor "
    "+$5E words retain the other-owner mode dependency; B649/B66A DP $46/$47 attachment "
    "scratch is retained because the host models it; argument/address/arithmetic DP "
    "$00/$8E/$AA/$B2 and other DP/stack scratch are excluded because the portable state "
    "has no corresponding storage; table bytes remain pack-backed data")
EXPECTED_SOURCE = {
    "vectors_sha256": "215155c4c085caf49af97dc84dd4e5bf915237e6279f9eed61af357fa4b2ba19",
    "repeat_vectors_sha256": "c0d88a401cba7294e6c757c176d85814a2bf4e523e796e0eba49670889f24bfd",
    "cases_sha256": "c58f19f007b91cc0d81b99c2f93f4a410522bc23b5431bfc936bbc74967a5527",
    "paths_sha256": "216e33722763b6ce4941048032ebf6ea1574cf33f5b5d2f5766eb13474881914",
    "ghidra_functions_sha256": "c3ce476090ee9a9ec970ce9b9ccb2fdeea42936f8a3ff9184880b609fff3b199",
    "ghidra_instructions_sha256": "f642199de28e016061db579f28a28f60c772de70a6a2a6712ded609e03f7d63c",
}
EXPECTED_AGGREGATE = {"restore_869846": 15, "cancel_upper_87b538": 7,
    "cancel_lower_87b555": 15, "upper_animation_87b47a": 23,
    "lower_animation_87b4db": 7, "both_animation_87b3bd": 9,
    "pose_87aec3": 10, "attach_xy_87b649": 17, "attach_z_87b66a": 17,
    "launch_869d6e": 8, "finish_86a9d0": 4, "landing_86986d": 4,
    "attach_point_87b832": 19, "cancel_pass_86a613": 7,
    "stats_869cdb": 12, "effect_87a9e3": 2}
EXPECTED_EXITS = {"86b17d": 4, "86b1b9": 7, "86b242": 8,
                  "86b28f": 3, "86b334": 29}
TOP_KEYS = {"schema", "routine", "rom_sha256", "provenance", "domain", "fields",
            "owned_starts_sha256", "aggregate_child_calls", "source", "calls"}
CALL_KEYS = {"call", "name", "entry_pc", "exit_pc", "executed", "child_calls",
             "input", "expected"}


def compact_hash(value) -> str:
    return hashlib.sha256(json.dumps(value, separators=(",", ":")).encode()).hexdigest()


def validate(document: dict) -> tuple[bytearray, list[dict], list[str]]:
    if type(document) is not dict or set(document) != TOP_KEYS:
        raise ValueError("mode-fourteen fixture shape changed")
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
        raise ValueError("mode-fourteen fixture identity/provenance changed")
    payload = bytearray()
    union: set[str] = set()
    exit_counts: dict[str, int] = {}
    aggregate: dict[str, int] = {}
    for number, call in enumerate(calls, 1):
        if type(call) is not dict or set(call) != CALL_KEYS:
            raise ValueError(f"case {number} shape changed")
        before = call.get("input", [])
        expected = call.get("expected", [])
        if (type(before) is not list or type(expected) is not list or
            len(before) != len(FIELDS) or len(expected) != len(FIELDS) or any(
                type(v) is not int or not 0 <= v <= 0xFFFF for v in before + expected)):
            raise ValueError(f"case {number} has an invalid field vector")
        executed = call.get("executed", [])
        children = call.get("child_calls", {})
        if (call.get("call") != number or call.get("entry_pc") != "86b154" or
            call.get("exit_pc") not in EXPECTED_EXITS or type(executed) is not list or
            not executed or executed[0] != "86b154" or executed[-1] != call["exit_pc"] or
            any(type(pc) is not str or len(pc) != 6 for pc in executed) or
            type(children) is not dict or any(type(k) is not str or type(v) is not int or
                v < 0 for k, v in children.items())):
            raise ValueError(f"case {number} has an invalid native boundary")
        union.update(executed)
        exit_counts[call["exit_pc"]] = exit_counts.get(call["exit_pc"], 0) + 1
        for child, count in children.items():
            aggregate[child] = aggregate.get(child, 0) + count
        payload.extend(struct.pack("<" + "H" * len(FIELDS), *before))
    union_hash = hashlib.sha256("\n".join(sorted(union)).encode()).hexdigest()
    if (len(union) != 185 or union_hash != EXPECTED_STARTS or
        exit_counts != EXPECTED_EXITS or aggregate != EXPECTED_AGGREGATE):
        raise ValueError("mode-fourteen path/child contract changed")
    return payload, calls, fields


def replay(probe: Path, pack: Path, payload: bytes, fields: list[str], calls: list[dict],
           option: str | None = None) -> list[str]:
    command = [str(probe), str(pack)] + ([option] if option else [])
    run = subprocess.run(command, input=payload, capture_output=True)
    if run.returncode:
        raise AssertionError(run.stderr.decode(errors="replace"))
    lines = [line for line in run.stdout.decode().splitlines()
             if len(line.split()) == len(fields)]
    if len(lines) != len(calls):
        raise AssertionError(f"probe returned {len(lines)}/{len(calls)} rows")
    return lines


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--vectors", type=Path, required=True)
    parser.add_argument("--probe", type=Path, required=True)
    parser.add_argument("--pack", type=Path, required=True)
    parser.add_argument("--skip-caller", action="store_true")
    args = parser.parse_args()
    raw = args.vectors.read_bytes()
    if hashlib.sha256(raw).hexdigest() != EXPECTED_FIXTURE:
        raise ValueError("mode-fourteen fixture byte hash changed")
    document = json.loads(raw)
    payload, calls, fields = validate(document)
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
    print(f"[CPU MODE FOURTEEN] {'PASS' if not mismatches else 'FAIL'}: "
          f"calls={len(calls)} fields={len(fields)} mismatches={len(mismatches)}")
    for number, name, differences in mismatches:
        print(f"case {number} {name}: {differences[:16]}")
    if mismatches:
        raise SystemExit(1)
    stale_lines = replay(args.probe, args.pack, payload, fields, calls,
                         "--stale-ball-owner")
    if stale_lines != lines:
        raise AssertionError("authoritative $093E replay depends on stale host ball owner")
    print("[CPU MODE FOURTEEN OWNER] PASS: stale host owner cannot replace authoritative $093E")
    if not args.skip_caller:
        caller = subprocess.run([str(args.probe), str(args.pack), "--self-test"],
                                capture_output=True, text=True)
        if caller.returncode or "[CPU MODE FOURTEEN CALLER] PASS" not in caller.stdout:
            print(caller.stdout, end="")
            print(caller.stderr, end="")
            raise AssertionError("production caller check failed")
        print("[CPU MODE FOURTEEN CALLER] PASS")


if __name__ == "__main__":
    main()
