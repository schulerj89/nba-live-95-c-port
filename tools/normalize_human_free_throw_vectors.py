"""Reduce genuine `$87:9CBF` Mesen calls to durable human-aim vectors."""

import argparse
import hashlib
import json
from pathlib import Path


def memory_byte(call, phase, address):
    for base_text, payload in call[phase]["mem"].items():
        base = int(base_text, 16)
        size = len(payload) // 2
        if base <= address < base + size:
            offset = (address - base) * 2
            return int(payload[offset:offset + 2], 16)
    raise KeyError(f"${address:04X} missing from {phase} snapshot")


def memory_word(call, phase, address):
    return memory_byte(call, phase, address) | (
        memory_byte(call, phase, address + 1) << 8)


def signed_word(value):
    return value - 0x10000 if value & 0x8000 else value


def normalized_rom_hash(path):
    data = Path(path).read_bytes()
    if len(data) % 0x8000 == 512:
        data = data[512:]
    return hashlib.sha256(data).hexdigest()


def transition_result(before, after):
    if before == 3 and after == 4:
        return 1
    if before == 4 and after == 5:
        return 2
    if before == 5 and after == 9:
        return 3
    if before == 4 and after == 3:
        return 4
    return 0


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--vectors", required=True)
    parser.add_argument("--capture-meta", required=True)
    parser.add_argument("--rom", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    vectors_path = Path(args.vectors)
    meta = json.loads(Path(args.capture_meta).read_text(encoding="utf-8-sig"))
    raw_rom_hash = hashlib.sha256(Path(args.rom).read_bytes()).hexdigest()
    if meta.get("rom_file_sha256") != raw_rom_hash or \
            meta.get("vectors_sha256") != hashlib.sha256(
                vectors_path.read_bytes()).hexdigest() or \
            meta.get("entry") != "879CBF,87A018" or \
            sorted(meta.get("exits", [])) != ["87a017", "87a045"] or \
            meta.get("max_calls") != 2000 or \
            meta.get("first_press_delay") != 60:
        raise SystemExit("capture metadata/ROM/vector identity mismatch")
    calls = [json.loads(line) for line in vectors_path.read_text().splitlines()
             if line.strip()]
    candidates = []
    for call in calls:
        if call.get("entry_pc", "").lower() != "879cbf":
            continue
        actor = memory_word(call, "entry", 0x00C2)
        shooter = memory_word(call, "entry", 0x492F)
        state_before = memory_word(call, "entry", 0x0978)
        if actor != shooter or actor >= 10 or state_before not in (3, 4, 5):
            continue
        actor_base = 0x34EB + actor * 0x100
        controller_word = memory_word(call, "entry", actor_base + 0x16)
        controller = signed_word(controller_word)
        context = memory_word(call, "entry", 0x009E)
        human_context = memory_word(call, "entry", context + 0x3B)
        if controller_word != 0 or human_context == 0:
            continue
        held = 0
        if 0 <= controller < 5:
            held = memory_word(call, "entry", 0x47EB + controller * 0x40 + 8)
        state_after = memory_word(call, "exit", 0x0978)
        input_values = [
            state_before,
            memory_word(call, "entry", 0x0980),
            memory_word(call, "entry", 0x0982),
            memory_word(call, "entry", 0x0984),
            memory_word(call, "entry", 0x0986),
            controller_word,
            human_context,
            1 if held & 0xC0C0 else 0,
        ]
        expected = [
            transition_result(state_before, state_after),
            state_after,
            memory_word(call, "exit", 0x0980),
            memory_word(call, "exit", 0x0982),
            memory_word(call, "exit", 0x0984),
            memory_word(call, "exit", 0x0986),
        ]
        signature = (state_before, state_after, input_values[-1])
        candidates.append((signature, call["call"], input_values, expected))

    wanted = [(3, 3, 0), (3, 4, 1), (4, 4, 1),
              (4, 5, 0), (5, 5, 0), (5, 9, 1)]
    selected = []
    for signature in wanted:
        match = next((row for row in candidates if row[0] == signature), None)
        if match is None:
            raise SystemExit(f"missing native transition {signature}")
        _, call_number, input_values, expected = match
        selected.append({
            "name": f"native-call-{call_number}-{signature[0]}-to-{signature[1]}",
            "source_call": call_number,
            "input": input_values,
            "expected": expected,
        })
    wrap = next((row for row in candidates
                 if row[0] == (3, 3, 0) and row[2][1] >= 108 and
                 row[3][2] < row[2][1]), None)
    if wrap is None:
        raise SystemExit("missing native oscillator wrap transition")
    signature, call_number, input_values, expected = wrap
    selected.append({
        "name": f"native-call-{call_number}-oscillator-wrap",
        "source_call": call_number,
        "input": input_values,
        "expected": expected,
    })
    rom_hash = normalized_rom_hash(args.rom)
    document = {
        "schema": 1,
        "rom_sha256": rom_hash,
        "native_entry": "879CBF",
        "native_oscillator": "87A018-87A045",
        "cases": selected,
    }
    output = Path(args.output)
    if output.exists():
        parser.error("output must be a new file")
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(document, indent=2) + "\n")
    print(f"[HUMAN FREE THROW] normalized {len(selected)} cases to {output}")


if __name__ == "__main__":
    main()
