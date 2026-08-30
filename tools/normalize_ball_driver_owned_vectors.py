"""Normalize genuine owned `$85:9A37` wrapper/core calls."""

import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path


ROM_SHA256 = "2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870"
SOURCE_SHA256 = "7a845ec070471070176882e51071909a7a9f6f5de06350cda21749a1ef66e79f"
# This one raw capture is retained, not repaired. Its floor bounce traverses
# A582/A585/A588, which OR bit 0, but 13E7 is zero at the captured return.
# The call crosses a frame boundary and 80:8576 -> 82:F8B1 -> 82:FD65 is an
# interrupt-path audio consumer; FD7A/FD7D clear bit 0 before 80:859B RTI.
# No per-write PC was captured, so interleaved consumption remains an inference.
# Keep all other captured words exact, report this as a PARTIAL native case,
# and separately check the instruction-derived isolated producer result.
NONISOLATED_EVENT = {
    "field": "events_13e7",
    "source_call": 181,
    "source_frame": 4241,
    "source_exit_frame": 4242,
    "captured_value": 0,
    "isolated_producer_value": 1,
    "reason": "Cross-frame capture has 13E7=0000 although the A582/A585/A588 "
              "floor-bounce producer sets bit 0. Interrupt audio path "
              "80:8576 -> 82:F8B1 -> 82:FD65 can clear it at FD7A/FD7D "
              "before RTI at 80:859B. The write PC was not captured; "
              "interleaved consumption is inferred, not proven. This field "
              "is excluded from native exactness, not changed in the fixture.",
}
INPUT_FIELDS = [
    "owner_093e", "counter_094a", "response_0970", "attachment_09f6",
    "dead_0968", "rim_0962", "activity_0948", "side_093a",
    "ball_x_fraction", "ball_x", "ball_y_fraction", "ball_y",
    "ball_z_fraction", "ball_z", "ball_vx", "ball_vy", "ball_vz",
    "actor_x_fraction", "actor_x", "actor_y_fraction", "actor_y",
    "actor_z_fraction", "actor_z", "actor_upper_2a", "actor_lower_2c",
    "actor_flags_28", "actor_phase_3a", "actor_mode_5e", "actor_half_a8",
    "actor_direction_4e", "actor_requested_50", "actor_display_52",
] + [f"controller_{i}_16" for i in range(10)] + [
    "impact_13e5", "events_13e7", "live_state_0936",
    "previous_x_0922", "previous_z_0924",
]
OUTPUT_FIELDS = INPUT_FIELDS[:7] + INPUT_FIELDS[8:17] + INPUT_FIELDS[32:47]
GLOBAL_ADDRESSES = [0x093E, 0x094A, 0x0970, 0x09F6, 0x0968, 0x0962, 0x0948]
BALL_ADDRESSES = [0x3EED, 0x3EEF, 0x3EF1, 0x3EF3, 0x3EF5, 0x3EF7,
                  0x3EF9, 0x3EFB, 0x3EFD]
ACTOR_OFFSETS = [2, 4, 6, 8, 10, 12, 0x2A, 0x2C, 0x28, 0x3A, 0x5E,
                 0xA8, 0x4E, 0x50, 0x52]


def memory(snapshot):
    raw = bytearray(0x4B00)
    for base, payload in snapshot["mem"].items():
        start = int(base, 16)
        data = bytes.fromhex(payload)
        raw[start:start + len(data)] = data
    return raw


def word(raw, address):
    return raw[address] | raw[address + 1] << 8


def normalized_rom_hash(path):
    raw = Path(path).read_bytes()
    if len(raw) % 0x8000 == 512:
        raw = raw[512:]
    return hashlib.sha256(raw).hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--vectors", required=True)
    parser.add_argument("--capture-meta", required=True)
    parser.add_argument("--rom", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--replace", action="store_true",
                        help="replace an explicitly named normalized fixture")
    args = parser.parse_args()
    source = Path(args.vectors)
    if hashlib.sha256(source.read_bytes()).hexdigest() != SOURCE_SHA256 or \
            normalized_rom_hash(args.rom) != ROM_SHA256:
        parser.error("source capture or ROM identity mismatch")
    meta = json.loads(Path(args.capture_meta).read_text())
    if meta.get("entry") != "859a24" or meta.get("exits") != ["85a7c7"] or \
            meta.get("max_calls") != 500:
        parser.error("unexpected source capture boundary")
    cases = []
    calls = [json.loads(line) for line in source.read_text().splitlines()
             if line.strip()]
    if len(calls) != 500:
        parser.error("expected the complete 500-call source capture")
    for call in calls:
        entry, after = memory(call["entry"]), memory(call["exit"])
        owner = word(entry, 0x093E)
        if word(entry, 0x09F2) < 0x8000 or owner >= 10:
            continue
        base = 0x34EB + owner * 0x100
        if call.get("exit_pc") != "85a7c7" or word(entry, 0x00C6) != 2:
            parser.error("unexpected selected call return or scheduler quantum")
        controllers = [0x34EB + i * 0x100 + 0x16 for i in range(10)]
        values = [word(entry, a) for a in GLOBAL_ADDRESSES]
        # The source enters before 9A2C. Move the sole prefix counter forward
        # so the durable input begins at the requested 9A37 dispatch boundary.
        if values[1]:
            values[1] = (values[1] + word(entry, 0x00C6)) & 0xFFFF
        values += [word(entry, 0x093A)]
        values += [word(entry, a) for a in BALL_ADDRESSES]
        values += [word(entry, base + offset) for offset in ACTOR_OFFSETS]
        values += [word(entry, a) for a in controllers]
        values += [word(entry, a) for a in
                   (0x13E5, 0x13E7, 0x0936, 0x0922, 0x0924)]
        expected = [word(after, a) for a in
                    GLOBAL_ADDRESSES + BALL_ADDRESSES + controllers +
                    [0x13E5, 0x13E7, 0x0936, 0x0922, 0x0924]]
        if any(values[i] & 0xFF for i in (8, 10, 12, 17, 19, 21)):
            parser.error("fixture needs unsupported low fractional bits")
        if values[23] >= 0xF0:
            kind = "preserve" if values[27] in (15, 17) else "project"
        else:
            if word(entry, 0x0940) != base or values[13] >= 73 or \
                    values[44] not in (0, 0x82):
                parser.error("low-owned call exceeds the captured non-rim scope")
            kind = "low-reset" if values[26] < 3 else "low-fall"
        same = values[:7] + values[8:17] + values[32:47]
        if kind.startswith("low-"):
            preserved = {0, 1, 2, 6, 7, 9, 28, *range(16, 26)}
        else:
            mutable = {8, 10, 12, 29} if kind == "project" else set()
            preserved = set(range(len(expected))) - mutable
        # B654 snapshots the OLD ball X for the high projection wrapper;
        # A5C7 snapshots owner X for the low-owned core. A59D additionally
        # snapshots pre-substep ball Z in the latter; keep its captured word.
        if kind == "project" and expected[29] != values[9] or \
                kind.startswith("low-") and expected[29] != values[18]:
            parser.error(f"native 0922 producer mismatch at call {call['call']}")
        if any(same[i] != expected[i] for i in preserved):
            parser.error(f"native preservation failed at call {call['call']}")
        case = {
            "source_call": call["call"],
            "source_frame": call["entry_frame"],
            "source_exit_frame": call["exit_frame"],
            "kind": kind,
            "input": values,
            "expected": expected,
        }
        if call["call"] == NONISOLATED_EVENT["source_call"]:
            if call["entry_frame"] != NONISOLATED_EVENT["source_frame"] or \
                    call["exit_frame"] != NONISOLATED_EVENT["source_exit_frame"] or \
                    kind != "low-fall" or values[0] != 8 or \
                    values[12:14] != [0x4800, 1] or values[16] != 0xFD50 or \
                    values[23] != 0x008E or values[26:28] != [4, 11] or \
                    values[43] != 0 or expected[11:13] != [0xFE00, 1] or \
                    expected[15] != 0x01FE or expected[26:28] != [0xFD38, 0]:
                parser.error("the narrowly scoped nonisolated event witness changed")
            case["nonisolated_output"] = NONISOLATED_EVENT
        cases.append(case)
    if Counter(case["kind"] for case in cases) != {
            "project": 66, "preserve": 42, "low-reset": 85, "low-fall": 131}:
        parser.error("unexpected owned wrapper/core branch census")
    document = {
        "schema": 2,
        "rom_sha256": ROM_SHA256,
        "source_vectors_sha256": SOURCE_SHA256,
        "source_entry": "859A24",
        "native_entry": "859A37",
        "native_exit": "85A7C7",
        "scope": "108 high-resource wrapper calls and 216 low-resource owned "
                 "core calls with captured Z<73 and live state 0/82; no "
                 "owned-rim classifier coverage is claimed. Native call 181 "
                 "is partial: its cross-frame 13E7 output is retained raw but "
                 "excluded from exact comparison; all other outputs are exact. "
                 "The 31-output projection includes native 0922/0924 snapshots: "
                 "high project saves old ball X, high preserve leaves both, "
                 "and low owned saves actor X and pre-substep ball Z.",
        "input_fields": INPUT_FIELDS,
        "output_fields": OUTPUT_FIELDS,
        "cases": cases,
    }
    target = Path(args.output)
    if target.exists() and not args.replace:
        parser.error("output must be new unless --replace is explicit")
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(json.dumps(document, indent=2) + "\n")
    print(f"[OWNED BALL DRIVER] normalized {len(cases)} native calls to {target}")


if __name__ == "__main__":
    main()
