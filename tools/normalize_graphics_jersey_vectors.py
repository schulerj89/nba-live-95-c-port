"""Normalize repeated native `$80:AD2B-$AD88` writes into behavioral cases."""
from __future__ import annotations

import argparse
import hashlib
import json
from collections import defaultdict
from pathlib import Path

ROM_SHA = "2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870"
MESEN_SHA = "d2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b"
CAPTURE_NAMES = (
    "native-natural-v1", "native-natural-v2",
    "native-wrap-v1", "native-wrap-v2",
    "native-hit-v1", "native-hit-v2",
    "native-carry-v1", "native-carry-v2",
)
ENTRY_PC, EXIT_PC = 0x80AD2B, 0x80AD88
VALID_SOURCES = {0x86F0, 0x8710, 0x8690, 0x86D0, 0x8730, 0x86B0}
WRITE_PCS = {0x80AD35, 0x80AD45, 0x80AD47, 0x80AD57, 0x80AD5F,
             0x80AD64, 0x80AD6A, 0x80AD70, 0x80AD7C, 0x80AD86}
# Two natural calls straddle the periodic interrupt.  These exact stack writes
# repeat in both captures; pin them so only the positively identified interrupt
# frame is removed from the routine's functional WRAM projection.
INTERRUPT_STACK_WRITES = {
    1168: [
        (0x80AD38, 0x1FEC, 0x80), (0x80AD38, 0x1FEB, 0xAD),
        (0x80AD38, 0x1FEA, 0x38), (0x80AD38, 0x1FE9, 0x00),
        (0x008607, 0x1FE8, 0x00), (0x008607, 0x1FE7, 0x00),
        (0x008608, 0x1FE6, 0x7E), (0x008609, 0x1FE5, 0x00),
        (0x00860B, 0x1FE5, 0x00), (0x00860B, 0x1FE4, 0x04),
        (0x00860C, 0x1FE3, 0x00), (0x00860C, 0x1FE2, 0x6C),
        (0x00860D, 0x1FE1, 0x00), (0x00860D, 0x1FE0, 0x04),
        (0x008619, 0x1FDF, 0x80), (0x00861C, 0x1FDE, 0x86),
        (0x00861C, 0x1FDD, 0x1E), (0x85EF15, 0x1FDC, 0x94),
        (0x85EF18, 0x1FDB, 0x00), (0x85EF18, 0x1FDA, 0x00),
    ],
    1191: [
        (0x80AD35, 0x1FEC, 0x80), (0x80AD35, 0x1FEB, 0xAD),
        (0x80AD35, 0x1FEA, 0x35), (0x80AD35, 0x1FE9, 0x00),
        (0x008607, 0x1FE8, 0x00), (0x008607, 0x1FE7, 0x00),
        (0x008608, 0x1FE6, 0x7E), (0x008609, 0x1FE5, 0x00),
        (0x00860B, 0x1FE5, 0x01), (0x00860B, 0x1FE4, 0xE8),
        (0x00860C, 0x1FE3, 0x00), (0x00860C, 0x1FE2, 0x30),
        (0x00860D, 0x1FE1, 0x00), (0x00860D, 0x1FE0, 0x10),
        (0x008619, 0x1FDF, 0x80), (0x00861C, 0x1FDE, 0x86),
        (0x00861C, 0x1FDD, 0x1E), (0x85EF15, 0x1FDC, 0x94),
        (0x85EF18, 0x1FDB, 0x00), (0x85EF18, 0x1FDA, 0x00),
    ],
}
FNV_OFFSET, FNV_PRIME = 14695981039346656037, 1099511628211
WRAM_SIZE = 0x20000


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read_jsonl(path: Path) -> list[dict]:
    return [json.loads(line) for line in path.read_text().splitlines() if line]


def word(memory: bytearray, address: int) -> int:
    return memory[address] | memory[(address + 1) & 0xFFFF] << 8


def set_word(memory: bytearray, address: int, value: int) -> None:
    memory[address] = value & 0xFF
    memory[(address + 1) & 0xFFFF] = value >> 8


def fnv(data) -> str:
    digest = FNV_OFFSET
    for value in data:
        digest = ((digest ^ value) * FNV_PRIME) & 0xFFFFFFFFFFFFFFFF
    return f"{digest:016x}"


def changed_hash(before: bytes, after: bytearray) -> tuple[str, int]:
    addresses = [index for index, (left, right) in
                 enumerate(zip(before, after)) if left != right]
    encoded = bytearray()
    for address in addresses:
        encoded.extend(address.to_bytes(3, "little"))
    return fnv(encoded), len(addresses)


def canonical_address(raw: int) -> int:
    if 0 <= raw <= 0x1FFF:
        return raw
    if 0x7E0000 <= raw <= 0x7FFFFF:
        return raw - 0x7E0000
    raise ValueError(f"unmapped native write address {raw:06x}")


def native_owned_starts(listing: Path) -> set[int]:
    owned = set()
    for line in listing.read_text().splitlines():
        fields = line.split("\t", 2)
        if len(fields) != 3:
            continue
        address = int(fields[0], 16)
        if ENTRY_PC <= address <= EXIT_PC:
            owned.add(address)
    if len(owned) != 49 or ENTRY_PC not in owned or EXIT_PC not in owned:
        raise ValueError("native listing ownership changed")
    return owned


def verify_manifest(run: Path) -> dict:
    manifest = json.loads((run / "manifest.json").read_text())
    isolation = manifest.get("isolation", {})
    sources = manifest.get("sources", {})
    if (manifest.get("exit_code") != 0 or
            isolation.get("post_settings_verified") is not True or
            sources.get("rom", {}).get("sha256") != ROM_SHA or
            sources.get("mesen", {}).get("sha256") != MESEN_SHA):
        raise ValueError(f"unverified capture manifest: {run.name}")
    for name, identity in sources.items():
        path = Path(identity["path"])
        if not path.is_file() or path.stat().st_size != identity["bytes"] or \
                sha(path) != identity["sha256"]:
            raise ValueError(f"capture source identity changed: {run.name}/{name}")
    artifacts = manifest.get("artifacts", {})
    for name, identity in artifacts.items():
        path = run / name
        if not path.is_file() or path.stat().st_size != identity["bytes"] or \
                sha(path) != identity["sha256"]:
            raise ValueError(f"capture artifact identity changed: {run.name}/{name}")
    return manifest


def validate_boundaries(rows: list[dict], seeded: bool) -> dict[int, tuple[dict, dict]]:
    active = None
    calls = {}
    before_seed = None
    for row in rows:
        tag, pc = row.get("tag"), row.get("pc")
        if tag == "parent.entry.before_seed":
            if not seeded or active is not None or before_seed is not None or pc != ENTRY_PC:
                raise ValueError("invalid before-seed boundary")
            before_seed = row
            continue
        if tag == "parent.entry":
            if active is not None or pc != ENTRY_PC or row.get("call") in calls:
                raise ValueError("nested or duplicate parent entry")
            if seeded and before_seed is None:
                raise ValueError("controlled entry lacks before-seed boundary")
            if before_seed is not None and before_seed["cpu"] != row["cpu"]:
                raise ValueError("controlled seed changed CPU context")
            active = row
            continue
        if tag == "parent.exit":
            if active is None or pc != EXIT_PC or row.get("call") != active.get("call"):
                raise ValueError("orphan or mismatched parent exit")
            entry_cpu, exit_cpu = active["cpu"], row["cpu"]
            if (entry_cpu["k"], entry_cpu["pc"], entry_cpu["d"], entry_cpu["dbr"]) != \
                    (0x80, 0xAD2B, 0, 0x7E) or entry_cpu["ps"] & 0x30 or \
                    (exit_cpu["k"], exit_cpu["pc"], exit_cpu["d"], exit_cpu["dbr"]) != \
                    (0x80, 0xAD88, 0, 0x7E) or exit_cpu["ps"] & 0x30 or \
                    entry_cpu["sp"] != exit_cpu["sp"]:
                raise ValueError("parent CPU entry/exit context changed")
            state = active["state"]
            if (state["be"] & 1 or state["be"] > 18 or state["c2"] >= 8 or
                    state["source"] not in VALID_SOURCES):
                raise ValueError("natural parent domain changed")
            calls[active["call"]] = (active, row)
            active = None
            continue
        raise ValueError(f"unknown boundary tag {tag}")
    if active is not None or (seeded and (before_seed is None or len(calls) != 1)):
        raise ValueError("incomplete parent boundaries")
    if sorted(calls) != list(range(1, len(calls) + 1)):
        raise ValueError("noncontiguous call identifiers")
    return calls


def expected_write_events(entry: dict) -> list[tuple[int, int, int]]:
    state = entry["state"]
    actor = state["be"] >> 1
    index = (actor + state["extent"]) & 0xFFFF
    source = state["source"]
    result = [
        (0x80AD35, 0x0004, index & 0xFF),
        (0x80AD35, 0x0005, index >> 8),
    ]
    if state["cache"] == source:
        return result
    descriptor_source = (source + actor * 0xC0) & 0xFFFF
    destination = ((index << 4) + state["vram_base"] +
                   (1 if index & 0x1000 else 0)) & 0xFFFF
    tail = state["tail"]
    words = ((0x80AD45, 0x8E10 + state["be"], source),
             (0x80AD47, 0x0000, source),
             (0x80AD57, 0x0000, descriptor_source),
             (0x80AD5F, 0x0100 + tail, 1),
             (0x80AD64, 0x0101 + tail, descriptor_source),
             (0x80AD6A, 0x0103 + tail, 0x007E),
             (0x80AD70, 0x0104 + tail, 0x0020),
             (0x80AD7C, 0x0106 + tail, destination),
             (0x80AD86, 0x0037, (tail + 8) & 0x01FF))
    for pc, address, value in words:
        result.extend(((pc, address, value & 0xFF),
                       (pc, (address + 1) & 0xFFFF, value >> 8)))
    return result


def validate_writes(calls: dict[int, tuple[dict, dict]], rows: list[dict]) -> dict[int, list[dict]]:
    grouped = defaultdict(list)
    stack_grouped = defaultdict(list)
    for row in rows:
        if row.get("call") not in calls:
            raise ValueError("unknown or orphan write event")
        if 0x1FDA <= row.get("address", -1) <= 0x1FEC:
            stack_grouped[row["call"]].append(
                (row.get("pc"), row["address"], row.get("value")))
            continue
        if row.get("pc") not in WRITE_PCS:
            raise ValueError("unknown non-stack write event")
        canonical = canonical_address(row["address"])
        grouped[row["call"]].append({"pc": row["pc"], "address": canonical,
                                      "value": row["value"]})
    expected_stack = INTERRUPT_STACK_WRITES if len(calls) > 1 else {}
    if dict(stack_grouped) != expected_stack:
        raise ValueError("interrupt stack write sequence changed")
    for call, (entry, exit_row) in calls.items():
        expected = expected_write_events(entry)
        actual = [(row["pc"], row["address"], row["value"])
                  for row in grouped[call]]
        if actual != expected:
            raise ValueError(f"native write order changed at call {call}")
        state, after = entry["state"], exit_row["state"]
        actor = state["be"] >> 1
        index = (actor + state["extent"]) & 0xFFFF
        hit = state["cache"] == state["source"]
        expected_tail = state["tail"] if hit else (state["tail"] + 8) & 0x01FF
        expected_x = state["c2"] * 2 if hit else state["tail"]
        if (after["dp4"] != index or after["tail"] != expected_tail or
                after["cache"] != state["source"] or exit_row["cpu"]["a"] != index or
                exit_row["cpu"]["x"] != expected_x or
                exit_row["cpu"]["y"] != state["be"]):
            raise ValueError(f"native exit state changed at call {call}")
    return grouped


def validate_paths(path_rows: list[dict], calls: dict, owned: set[int]) -> set[int]:
    grouped = defaultdict(list)
    observed = set()
    for row in path_rows:
        if row.get("call") not in calls or row.get("pc") not in owned:
            raise ValueError("unknown path row")
        grouped[row["call"]].append(row["pc"])
        observed.add(row["pc"])
    for call in calls:
        path = grouped[call]
        # Mesen invokes the dedicated AD88 exit hook before the range hook can
        # append that same PC.  The boundary stream proves AD88; the path must
        # reach the immediately preceding LDA at AD86.
        if not path or path[0] != ENTRY_PC or path[-1] != 0x80AD86:
            raise ValueError(f"incomplete path for call {call}")
    observed.add(EXIT_PC)
    return observed


def validate_callers(rows: list[dict]) -> None:
    if not rows or len(rows) & 1:
        raise ValueError("caller rows missing")
    for index in range(0, len(rows), 2):
        before, after = rows[index:index + 2]
        if before.get("tag") != "a64d" or after.get("tag") != "a656" or \
                before["cpu"]["pc"] != 0xA64D or after["cpu"]["pc"] != 0xA656 or \
                before["cpu"]["x"] != before["actor_record"] or \
                not 0x34EB <= before["actor_record"] <= 0x3DEB or \
                (before["actor_record"] - 0x34EB) % 0x100 or \
                after["be"] != ((before["actor_record"] - 0x34EB) // 0x100) * 2 or \
                after["direction"] >= 8:
            raise ValueError("renderer caller contract changed")


def project(entry: dict, writes: list[dict], name: str, kind: str) -> dict:
    state = entry["state"]
    memory = bytearray([0x5A]) * WRAM_SIZE
    set_word(memory, 0x00BE, state["be"])
    set_word(memory, 0x00C2, state["c2"])
    set_word(memory, 0x05EF, state["extent"])
    set_word(memory, 0x05EB, state["vram_base"])
    set_word(memory, 0x0037, state["tail"])
    set_word(memory, 0x8E10 + state["be"], state["cache"])
    before = bytes(memory)
    for write in writes:
        memory[write["address"]] = write["value"]
    actor = state["be"] >> 1
    index = (actor + state["extent"]) & 0xFFFF
    appended = state["cache"] != state["source"]
    result = [index, 0, 0, 0]
    if appended:
        result = [index,
                  ((index << 4) + state["vram_base"] +
                   (1 if index & 0x1000 else 0)) & 0xFFFF,
                  (state["source"] + actor * 0xC0) & 0xFFFF, 1]
    footprint_hash, footprint_count = changed_hash(before, memory)
    tail = state["tail"]
    record = [(memory[(0x0100 + tail + offset) & 0xFFFF]) for offset in range(8)]
    return {
        "name": name, "kind": kind,
        "operation": [0x5A, state["be"], state["c2"], state["extent"],
                      state["vram_base"], state["tail"], state["cache"]],
        "entry": {key: state[key] for key in
                  ("be", "c2", "extent", "vram_base", "tail", "cache", "source")},
        "expected": {
            "ok": 1, "result": result,
            "full_hash": fnv(memory), "changed_hash": footprint_hash,
            "changed_count": footprint_count,
            "words": [word(memory, address) for address in
                      (0x0000, 0x0004, 0x0037, 0x05EB, 0x05EF,
                       0x8E10 + state["be"])],
            "record": record,
            "sentinels": [memory[address] for address in
                          (0x0035, 0x0039, 0x012C, 0x0566, 0x8E24, 0x1FFFF)],
            "native_write_events": len(writes),
            "native_unique_written_bytes": len({row["address"] for row in writes}),
        },
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--capture-root", type=Path, required=True)
    parser.add_argument("--listing", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    root = args.capture_root.resolve()
    owned = native_owned_starts(args.listing)
    manifests, loaded = {}, {}
    for name in CAPTURE_NAMES:
        run = root / name
        manifests[name] = verify_manifest(run)
        rows = read_jsonl(run / "boundaries.jsonl")
        calls = validate_boundaries(rows, manifests[name]["state_injection"])
        writes = validate_writes(calls, read_jsonl(run / "writes.jsonl"))
        paths = validate_paths(read_jsonl(run / "paths.jsonl"), calls, owned)
        if name.startswith("native-natural"):
            validate_callers(read_jsonl(run / "callers.jsonl"))
        loaded[name] = (rows, calls, writes, paths)

    for first, second in (("native-natural-v1", "native-natural-v2"),
                          ("native-wrap-v1", "native-wrap-v2"),
                          ("native-hit-v1", "native-hit-v2"),
                          ("native-carry-v1", "native-carry-v2")):
        for filename in ("boundaries.jsonl", "writes.jsonl", "paths.jsonl"):
            if sha(root / first / filename) != sha(root / second / filename):
                raise ValueError(f"repeat capture differs: {first}/{filename}")
    natural_calls = loaded["native-natural-v1"][1]
    natural_writes = loaded["native-natural-v1"][2]
    natural_paths = loaded["native-natural-v1"][3]
    if (len(natural_calls) != 1288 or natural_paths != owned or
            sum(a[0]["state"]["cache"] == a[0]["state"]["source"]
                for a in natural_calls.values()) != 1244 or
            {pair[0]["state"]["source"] for pair in natural_calls.values()} != VALID_SOURCES):
        raise ValueError("natural coverage contract changed")

    selected = []
    used = set()
    for direction in (0, 2, 3, 4, 6, 7):
        for hit in (False, True):
            call = next(call for call, pair in natural_calls.items()
                        if pair[0]["state"]["c2"] == direction and
                        (pair[0]["state"]["cache"] == pair[0]["state"]["source"]) == hit)
            if call not in used:
                selected.append(project(natural_calls[call][0], natural_writes[call],
                    f"natural_direction_{direction}_{'hit' if hit else 'miss'}", "natural"))
                used.add(call)
    rollover = next(call for call, pair in natural_calls.items()
                    if pair[0]["state"]["tail"] == 0x1F8 and
                    pair[1]["state"]["tail"] == 0)
    if rollover not in used:
        selected.append(project(natural_calls[rollover][0], natural_writes[rollover],
                                "natural_tail_rollover", "natural"))
    for run_name, case_name in (("native-wrap-v1", "controlled_arithmetic_wrap"),
                                ("native-hit-v1", "controlled_hit_high_index"),
                                ("native-carry-v1", "controlled_adc_carry")):
        calls, writes = loaded[run_name][1], loaded[run_name][2]
        selected.append(project(calls[1][0], writes[1], case_name, "controlled"))

    fixture = {
        "schema": 1,
        "routine": "80ad2b-80ad88",
        "projection": "ordered native functional WRAM writes over byte 0x5a",
        "sources": {
            "rom_sha256": ROM_SHA, "mesen_sha256": MESEN_SHA,
            "listing_sha256": sha(args.listing),
            "runs": [{"name": name,
                      "manifest_sha256": sha(root / name / "manifest.json"),
                      "boundaries_sha256": sha(root / name / "boundaries.jsonl"),
                      "writes_sha256": sha(root / name / "writes.jsonl"),
                      "paths_sha256": sha(root / name / "paths.jsonl")}
                     for name in CAPTURE_NAMES],
        },
        "coverage": {"owned_starts": len(owned), "natural_calls": len(natural_calls),
                     "natural_hits": 1244, "natural_misses": 44,
                     "natural_valid_sources": 6, "natural_rollovers": 1,
                     "interrupt_stack_sequences": len(INTERRUPT_STACK_WRITES)},
        "owned_start_addresses": [f"{address:06x}" for address in sorted(owned)],
        "natural_boundary_contract": list(natural_calls[1]),
        "calls": selected,
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(fixture, indent=2) + "\n")
    print(f"[JERSEY NORMALIZE] calls={len(selected)} owned_starts={len(owned)}")


if __name__ == "__main__":
    main()
