"""Project native allocator write events onto an asset-free WRAM fixture."""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
from pathlib import Path

WRAM_SIZE = 0x20000
ROM_SHA = "2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870"
MESEN_SHA = "d2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b"
STACK_POST_PCS = {0x80AB82, 0x80AB85, 0x80AC06, 0x80AC07, 0x80AC0A, 0x80AC0B}
FIXED_WRITE_POST = {
    0x80AB81: 0x05EB, 0x80AB8A: 0x05ED, 0x80AB9D: 0x05EF,
    0x80ABA0: 0x05F1, 0x80ABBD: 0x32EA, 0x80ABC5: 0x05F9,
    0x80ABD2: 0x3363, 0x80ABDD: 0x33C4, 0x80ABE5: 0x05E5,
    0x80ABEB: 0x05F3, 0x80ABF1: 0x05F5, 0x80ABF7: 0x05DF,
    0x80ABFD: 0x05E3, 0x80AC03: 0x05E1, 0x80ACA9: 0x05DF,
    0x80ACAC: 0x05F5, 0x80ACB5: 0x05E3, 0x80ACB8: 0x05E1,
    0x80ACBF: 0x0566,
}


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def rows(path: Path) -> list[dict]:
    return [json.loads(line) for line in path.read_text().splitlines() if line]


def canonical(raw: int) -> int | None:
    bank, low = raw >> 16, raw & 0xFFFF
    if bank in (0x7E, 0x7F):
        return raw - 0x7E0000
    if low < 0x2000 and (bank <= 0x3F or 0x80 <= bank <= 0xBF):
        return low
    return None


def functional(event: dict) -> tuple[int, int] | None:
    address = canonical(event["address"])
    if address is None:
        raise ValueError(f"unmapped CPU-bus write: {event}")
    pc = event["pc"]
    if pc in STACK_POST_PCS:
        if not 0x1FF2 <= address <= 0x1FFC:
            raise ValueError(f"unexpected native call-stack write: {event}")
        return None
    if pc in FIXED_WRITE_POST:
        first = FIXED_WRITE_POST[pc]
        width = 1 if pc in (0x80ABBD, 0x80ABD2, 0x80ABDD, 0x80ACBF) else 2
        if not first <= address < first + width:
            raise ValueError(f"fixed write address changed: {event}")
    elif pc == 0x80ABAA:
        if not 0x3271 <= address <= 0x3370:
            raise ValueError(f"first byte table write changed: {event}")
    elif pc == 0x80ABB1:
        if not 0x32EA <= address <= 0x33E9:
            raise ValueError(f"second byte table write changed: {event}")
    elif pc == 0x80ABD5:
        if not 0x3363 <= address <= 0x3462:
            raise ValueError(f"rounded-X first sentinel changed: {event}")
    elif pc == 0x80ABD8:
        if not 0x33C4 <= address <= 0x34C3:
            raise ValueError(f"rounded-X second sentinel changed: {event}")
    elif pc == 0x80AC16:
        if not 0x2640 <= address <= 0x2E71:
            raise ValueError(f"cache clear write changed: {event}")
    elif pc == 0x80AC9A:
        if address > 0xFFFF:
            raise ValueError(f"AC89 crossed DBR=$7E: {event}")
    else:
        raise ValueError(f"unknown allocator write PC: {event}")
    return address, event["value"]


def fnv(data) -> str:
    value = 14695981039346656037
    for byte in data:
        value ^= byte
        value = value * 1099511628211 & 0xFFFFFFFFFFFFFFFF
    return f"{value:016x}"


def read16(memory: bytearray, address: int) -> int:
    return memory[address] | memory[(address + 1) & 0xFFFF] << 8


def write16(memory: bytearray, address: int, value: int) -> None:
    memory[address] = value & 0xFF
    memory[(address + 1) & 0xFFFF] = value >> 8


def projection(events: list[tuple[int, int]], inputs: dict | None = None) -> dict:
    memory = bytearray([0x5A]) * WRAM_SIZE
    if inputs:
        for address, value in inputs.items():
            write16(memory, int(address, 16), value)
    before = memory[:]
    stream = bytearray()
    touched = set()
    for address, value in events:
        memory[address] = value
        touched.add(address)
        stream.extend(struct.pack("<I", address)[:3])
        stream.append(value)
    changed = [i for i, pair in enumerate(zip(before, memory)) if pair[0] != pair[1]]
    changed_stream = b"".join(struct.pack("<I", value)[:3] for value in changed)
    return {
        "ok": 1,
        "full_hash": fnv(memory),
        "changed_hash": fnv(changed_stream),
        "changed_count": len(changed),
        "fill_hash": fnv(memory[0x2000:0x2200]),
        "cache_hash": fnv(memory[0x2640:0x2E72]),
        "table_hash": fnv(memory[0x3271:0x33EA]),
        "words": [read16(memory, address) for address in
                  (0x05DF, 0x05E1, 0x05E3, 0x05E5, 0x05EB, 0x05ED,
                   0x05EF, 0x05F1, 0x05F3, 0x05F5, 0x05F9)],
        "bytes": [memory[address] for address in
                  (0x0566, 0x0567, 0x0035, 0x0037, 0x012C, 0x8E10)],
        "native_write_events": len(events),
        "native_unique_written_bytes": len(touched),
        "native_write_stream_sha256": hashlib.sha256(stream).hexdigest(),
    }


def verified_capture(directory: Path) -> tuple[dict, list[dict], list[dict]]:
    manifest = json.loads((directory / "manifest.json").read_text())
    if (manifest["exit_code"] != 0 or
            manifest["sources"]["rom"]["sha256"] != ROM_SHA or
            manifest["sources"]["mesen"]["sha256"] != MESEN_SHA or
            not manifest["isolation"].get("post_settings_verified")):
        raise ValueError(f"bad capture provenance: {directory}")
    for name, identity in manifest["artifacts"].items():
        if sha(directory / name) != identity["sha256"]:
            raise ValueError(f"artifact changed: {directory / name}")
    expected_sources = {
        "capture": directory / "capture.lua",
        "runner": directory / "capture.py",
        "isolation_helper": directory / "mesen_portable.py",
        "mesen": directory / "portable-mesen" / "Mesen.exe",
    }
    for name, path in expected_sources.items():
        if (Path(manifest["sources"][name]["path"]).resolve() != path.resolve() or
                sha(path) != manifest["sources"][name]["sha256"]):
            raise ValueError(f"capture source changed: {directory} {name}")
    return manifest, rows(directory / "boundaries.jsonl"), rows(directory / "writes.jsonl")


def parent_streams(events: list[dict]) -> list[list[tuple[int, int]]]:
    streams, current, stack_writes = [], None, []
    for event in events:
        if current is None and not event.get("parent"):
            continue
        item = functional(event)
        if event["pc"] == 0x80AB81 and item and item[0] == 0x05EB:
            if current is not None or not event.get("parent"):
                raise ValueError("nested or unmarked parent write stream")
            current = []
            stack_writes = []
        if current is not None:
            if not event.get("parent"):
                raise ValueError("parent write stream lost parent marker")
            if item:
                current.append(item)
            else:
                stack_writes.append((event["pc"], canonical(event["address"])))
        if current is not None and event["pc"] == 0x80ACBF and item and item[0] == 0x0566:
            stack_base = stack_writes[0][1] if stack_writes else -1
            expected_stack = [
                (0x80AB82, stack_base),
                (0x80AB85, stack_base - 1), (0x80AB85, stack_base - 2),
                (0x80AC06, stack_base - 1),
                (0x80AC07, stack_base - 2), (0x80AC07, stack_base - 3),
                (0x80AC0A, stack_base - 1),
                (0x80AC0B, stack_base - 2), (0x80AC0B, stack_base - 3),
            ]
            if stack_writes != expected_stack:
                raise ValueError(f"parent native call-stack footprint: {stack_writes}")
            streams.append(current)
            current = None
    if current is not None:
        raise ValueError("unfinished parent write stream")
    return streams


def ac89_streams(events: list[dict], limit: int = 2) -> list[list[tuple[int, int]]]:
    streams, current = [], None
    for event in events:
        if event.get("child") != "ac89" or event.get("parent"):
            continue
        item = functional(event)
        if current is None:
            current = []
        if item:
            current.append(item)
        if event["pc"] == 0x80ACBF and item and item[0] == 0x0566:
            streams.append(current)
            current = None
            if len(streams) == limit:
                break
    if current is not None:
        raise ValueError("unfinished direct AC89 write stream")
    return streams


def ac0d_streams(events: list[dict]) -> list[list[tuple[int, int]]]:
    streams, current = [], []
    for event in events:
        if event.get("child") == "ac0d":
            item = functional(event)
            if item is None:
                raise ValueError("AC0D emitted a stack write")
            current.append(item)
        elif current:
            streams.append(current)
            current = []
    if current:
        streams.append(current)
    expected_addresses = []
    for address in range(0x2E70, 0x263F, -2):
        expected_addresses.extend((address, address + 1))
    for stream in streams:
        if (len(stream) != 2098 or [address for address, _ in stream] != expected_addresses or
                {value for _, value in stream} != {0xFF}):
            raise ValueError("AC0D exact write footprint changed")
    return streams


def validate_boundaries(boundaries: list[dict], seeded: bool) -> None:
    expected_pc = {"caller.entry": 0x858B6C, "caller.post": 0x858B79,
                   "parent.entry.before_seed": 0x80AB7E,
                   "parent.entry": 0x80AB7E, "parent.exit": 0x80AC0C,
                   "ac0d.entry": 0x80AC0D, "ac0d.exit": 0x80AC1A,
                   "ac89.entry": 0x80AC89, "ac89.exit": 0x80ACC1}
    if any(row["tag"] not in expected_pc or row["pc"] != expected_pc[row["tag"]]
           for row in boundaries):
        raise ValueError("boundary tag/PC changed")
    counts = {tag: sum(row["tag"] == tag for row in boundaries) for tag in
              ("caller.entry", "caller.post", "parent.entry.before_seed",
               "parent.entry", "parent.exit", "ac0d.entry", "ac0d.exit",
               "ac89.entry", "ac89.exit")}
    if seeded:
        expected = {"caller.entry": 1, "caller.post": 0,
                    "parent.entry.before_seed": 1, "parent.entry": 1,
                    "parent.exit": 1, "ac0d.entry": 1, "ac0d.exit": 1,
                    "ac89.entry": 1, "ac89.exit": 1}
    else:
        expected = {"caller.entry": 1, "caller.post": 1,
                    "parent.entry.before_seed": 0, "parent.entry": 3,
                    "parent.exit": 3, "ac0d.entry": 3, "ac0d.exit": 3,
                    "ac89.entry": 49, "ac89.exit": 49}
    if counts != expected:
        raise ValueError(f"boundary pairing changed: {counts}")
    active_parent = active_clear = active_swap = None
    caller = None
    before_seed = None
    for row in boundaries:
        tag, cpu = row["tag"], row["cpu"]
        if tag == "caller.entry":
            if caller is not None:
                raise ValueError("nested caller boundary")
            caller = cpu
        elif tag == "caller.post":
            if caller is None or cpu["sp"] != caller["sp"] or cpu["dbr"] != caller["dbr"]:
                raise ValueError("caller return context changed")
            caller = None
        elif tag == "parent.entry.before_seed":
            if before_seed is not None or active_parent is not None:
                raise ValueError("misordered seed boundary")
            before_seed = cpu
        elif tag == "parent.entry":
            if active_parent is not None or active_clear is not None or active_swap is not None:
                raise ValueError("nested parent boundary")
            if before_seed is not None:
                for key in ("d", "dbr", "k", "pc", "ps", "sp"):
                    if before_seed[key] != cpu[key]:
                        raise ValueError(f"seed changed {key}")
                before_seed = None
            active_parent = cpu
        elif tag == "parent.exit":
            if active_parent is None or active_clear is not None or active_swap is not None:
                raise ValueError("orphan parent exit")
            if cpu["sp"] != active_parent["sp"] or cpu["dbr"] != active_parent["dbr"] or \
                    (cpu["ps"] & 0x30) != (active_parent["ps"] & 0x30):
                raise ValueError("parent return context changed")
            active_parent = None
        elif tag == "ac0d.entry":
            if active_clear is not None or active_swap is not None:
                raise ValueError("nested AC0D boundary")
            active_clear = cpu
        elif tag == "ac0d.exit":
            if active_clear is None or cpu["sp"] != active_clear["sp"] or cpu["dbr"] != active_clear["dbr"]:
                raise ValueError("orphan AC0D exit")
            active_clear = None
        elif tag == "ac89.entry":
            if active_swap is not None or active_clear is not None:
                raise ValueError("nested AC89 boundary")
            active_swap = cpu
        elif tag == "ac89.exit":
            if active_swap is None or cpu["sp"] != active_swap["sp"] or cpu["dbr"] != active_swap["dbr"]:
                raise ValueError("orphan AC89 exit")
            active_swap = None
    unfinished = (active_parent, active_clear, active_swap, before_seed)
    if any(value is not None for value in unfinished) or (caller is not None and not seeded):
        raise ValueError("unfinished boundary nesting")


def inventory_coverage(root: Path, capture_dirs: list[Path]) -> dict:
    inventory = json.loads((root / "manager-native-inventory.json").read_text())
    observed = set()
    for directory in capture_dirs:
        observed.update(row["pc"] for row in rows(directory / "paths.jsonl"))
        observed.update(row["pc"] for row in rows(directory / "boundaries.jsonl")
                        if row["tag"].endswith("exit"))
    keys = (("parent", "parent_starts"), ("cache_clear", "clear_starts"),
            ("oam_swap", "swap_starts"))
    result = {}
    for (native_name, output_name), native in zip(keys, inventory["ranges"]):
        if native["name"] != native_name:
            raise ValueError("native inventory order changed")
        expected = {int(item["address"], 16) for item in native["instructions"]}
        actual = expected & observed
        if actual != expected:
            missing = ",".join(f"{pc:06x}" for pc in sorted(expected - actual))
            raise ValueError(f"owned path coverage incomplete: {native_name} {missing}")
        result[output_name] = len(actual)
    return result


def run_identity(directory: Path, manifest: dict) -> dict:
    return {
        "name": directory.name,
        "capture_sha256": manifest["sources"]["capture"]["sha256"],
        "runner_sha256": manifest["sources"]["runner"]["sha256"],
        "boundaries_sha256": manifest["artifacts"]["boundaries.jsonl"]["sha256"],
        "writes_sha256": manifest["artifacts"]["writes.jsonl"]["sha256"],
        "paths_sha256": manifest["artifacts"].get("paths.jsonl", {}).get("sha256"),
    }


def word_from_snapshot(snapshot: dict, address: int) -> int:
    for label, text in snapshot.items():
        first = int(label.split("-")[0], 16)
        data = bytes.fromhex(text)
        if first <= address and address + 1 < first + len(data):
            offset = address - first
            return data[offset] | data[offset + 1] << 8
    raise KeyError(hex(address))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--capture-root", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    root = args.capture_root
    natural_manifest, natural_boundaries, natural_writes = verified_capture(root / "native-natural-v2")
    repeat_manifest, repeat_boundaries, _ = verified_capture(root / "native-natural-v1")
    validate_boundaries(natural_boundaries, False)
    validate_boundaries(repeat_boundaries, False)
    if natural_boundaries != repeat_boundaries:
        raise ValueError("natural boundary replay changed")

    entries = [row for row in natural_boundaries if row["tag"] == "parent.entry"]
    streams = parent_streams(natural_writes)
    if len(entries) != 3 or len(streams) != 3:
        raise ValueError("natural parent matrix")
    calls = []
    for index, (entry, stream) in enumerate(zip(entries, streams)):
        cpu = entry["cpu"]
        calls.append({
            "name": ("natural_boot_clamp_zero", "natural_boot_clamp_01a0",
                     "natural_court_caller")[index],
            "kind": "parent", "operation": [0, 0x5A, cpu["a"], cpu["x"], cpu["y"], 0, 0],
            "entry": {"pc": "80ab7e", "a": cpu["a"], "x": cpu["x"], "y": cpu["y"],
                      "dbr": cpu["dbr"], "ps": cpu["ps"], "sp": cpu["sp"]},
            "expected": projection(stream),
        })

    run_identities = [run_identity(root / "native-natural-v1", repeat_manifest),
                      run_identity(root / "native-natural-v2", natural_manifest)]
    coverage_dirs = [root / "native-natural-v2"]
    for label in ("y01bf", "y01c0", "y01e1", "y0201", "yffe1"):
        directory = root / f"native-seed-{label}-v2"
        manifest, boundaries, writes = verified_capture(directory)
        repeat_manifest_seed, repeat, repeat_writes = verified_capture(root / f"native-seed-{label}-v1")
        validate_boundaries(boundaries, True)
        validate_boundaries(repeat, True)
        if boundaries != repeat or sha(directory / "writes.jsonl") != sha(root / f"native-seed-{label}-v1" / "writes.jsonl"):
            raise ValueError(f"seeded replay changed: {label}")
        run_identities.extend((run_identity(root / f"native-seed-{label}-v1",
                                            repeat_manifest_seed),
                               run_identity(directory, manifest)))
        coverage_dirs.append(directory)
        before_seed = next(row for row in boundaries if row["tag"] == "parent.entry.before_seed")
        entry = next(row for row in boundaries if row["tag"] == "parent.entry")
        for key in ("d", "dbr", "k", "pc", "ps", "sp"):
            if before_seed["cpu"][key] != entry["cpu"][key]:
                raise ValueError(f"seed changed {key}: {label}")
        stream = parent_streams(writes)
        if len(stream) != 1:
            raise ValueError(f"seed parent stream: {label}")
        cpu = entry["cpu"]
        calls.append({
            "name": f"controlled_{label}", "kind": "parent",
            "operation": [0, 0x5A, cpu["a"], cpu["x"], cpu["y"], 0, 0],
            "entry": {"pc": "80ab7e", "a": cpu["a"], "x": cpu["x"], "y": cpu["y"],
                      "dbr": cpu["dbr"], "ps": cpu["ps"], "sp": cpu["sp"]},
            "controlled_registers": ["a", "x", "y"],
            "expected": projection(stream[0]),
        })

    ac_entries = [row for row in natural_boundaries if row["tag"] == "ac89.entry"]
    direct_stream = ac89_streams(natural_writes)
    direct_entries = []
    for index, row in enumerate(natural_boundaries):
        if row["tag"] == "ac89.entry" and (index == 0 or
                natural_boundaries[index - 1]["tag"] != "ac0d.exit"):
            direct_entries.append(row)
            if len(direct_entries) == len(direct_stream):
                break
    if len(direct_entries) != len(direct_stream):
        raise ValueError("AC89 entry/write pairing")
    selected = {}
    for entry, stream in zip(direct_entries, direct_stream):
        values = {f"{address:04x}": word_from_snapshot(entry["mem"], address)
                  for address in (0x05F3, 0x05F5, 0x05DF, 0x05E1, 0x05E3)}
        head, tail = values["05f3"], values["05f5"]
        if "empty" not in selected and head >= tail:
            selected["empty"] = (entry, stream, values)
        if "nonempty" not in selected and head < tail:
            selected["nonempty"] = (entry, stream, values)
    for name in ("empty", "nonempty"):
        entry, stream, values = selected[name]
        operation = [2, 0x5A, values["05f3"], values["05f5"], values["05df"],
                     values["05e1"], values["05e3"]]
        calls.append({"name": f"natural_ac89_{name}", "kind": "ac89",
                      "operation": operation,
                      "entry": {"pc": "80ac89", **values},
                      "expected": projection(stream, values)})

    clear_stream = ac0d_streams(natural_writes)
    if len(clear_stream) != 3:
        raise ValueError("natural AC0D call count")
    calls.append({"name": "natural_ac0d_first", "kind": "ac0d",
                  "operation": [1, 0x5A, 0, 0, 0, 0, 0],
                  "entry": {"pc": "80ac0d"},
                  "expected": projection(clear_stream[0])})

    coverage = inventory_coverage(root, coverage_dirs)
    coverage["natural_ac89_calls"] = sum(
        row["tag"] == "ac89.entry" for row in natural_boundaries)

    fixture = {
        "schema": 1,
        "routine": "80ab7e-80ac0c",
        "children": ["80ac0d-80ac1a", "80ac89-80acc1"],
        "projection": "ordered native functional WRAM writes over byte 0x5a; native call-stack writes excluded",
        "sources": {
            "rom_sha256": ROM_SHA,
            "inventory_sha256": sha(root / "manager-native-inventory.json"),
            "mesen_sha256": MESEN_SHA,
            "runs": run_identities,
        },
        "coverage": coverage,
        "natural_boundary_contract": [
            {"tag": row["tag"], "pc": row["pc"],
             "cpu": {key: row["cpu"][key] for key in
                     ("a", "x", "y", "d", "dbr", "k", "pc", "ps", "sp")}}
            for row in natural_boundaries
        ],
        "calls": calls,
    }
    args.output.write_text(json.dumps(fixture, indent=2) + "\n")
    print(f"wrote {len(calls)} native-backed calls to {args.output}")


if __name__ == "__main__":
    main()
