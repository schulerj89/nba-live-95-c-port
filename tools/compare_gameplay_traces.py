"""Compare frame-aligned Mesen ROM and native-port gameplay JSONL traces."""

import argparse
import json
from pathlib import Path

UNKNOWN_VALUES = {65535, -32768}


def load_rows(path):
    rows = {}
    for line_number, line in enumerate(Path(path).read_text().splitlines(), 1):
        if not line.strip():
            continue
        try:
            row = json.loads(line)
        except json.JSONDecodeError as error:
            raise ValueError(f"{path}:{line_number}: {error}") from error
        frame = int(row["scene_frame"])
        if frame in rows:
            raise ValueError(f"{path}: duplicate scene_frame {frame}")
        rows[frame] = row
    if not rows:
        raise ValueError(f"{path}: trace has no rows")
    return rows


def mismatch(items, frame, path, rom_value, port_value):
    items.append({"frame": frame, "path": path,
                  "rom": rom_value, "port": port_value})


def compare_core(rom, port, frame, mismatches):
    if rom.get("phase") != port.get("phase"):
        mismatch(mismatches, frame, "phase", rom.get("phase"), port.get("phase"))
    rom_actors, port_actors = rom.get("actors", []), port.get("actors", [])
    if len(rom_actors) != 10 or len(port_actors) != 10:
        mismatch(mismatches, frame, "actors.length", len(rom_actors), len(port_actors))
        return
    for index, (rom_actor, port_actor) in enumerate(zip(rom_actors, port_actors)):
        for field in ("id", "team", "roster", "visible", "x", "y", "z"):
            if rom_actor.get(field) != port_actor.get(field):
                mismatch(mismatches, frame, f"actors.{index}.{field}",
                         rom_actor.get(field), port_actor.get(field))
        if rom_actor.get("visible") and port_actor.get("visible"):
            for field in ("screen_x", "screen_y", "direction", "animation",
                          "lower_animation"):
                if rom_actor.get(field) != port_actor.get(field):
                    mismatch(mismatches, frame, f"actors.{index}.{field}",
                             rom_actor.get(field), port_actor.get(field))


def flatten(value, prefix=""):
    if isinstance(value, dict):
        for key, child in value.items():
            path = f"{prefix}.{key}" if prefix else key
            yield from flatten(child, path)
    elif isinstance(value, list):
        for index, child in enumerate(value):
            yield from flatten(child, f"{prefix}.{index}")
    else:
        yield prefix, value


def compare_all(rom, port, frame, mismatches):
    ignored = {"source", "frame", "scene_frame", "simulation_tick", "routine_hits"}
    rom_values = dict(flatten(rom))
    port_values = dict(flatten(port))
    paths = sorted(set(rom_values) & set(port_values))
    for path in paths:
        if path.split(".", 1)[0] in ignored:
            continue
        rom_value, port_value = rom_values[path], port_values[path]
        if rom_value in UNKNOWN_VALUES or port_value in UNKNOWN_VALUES:
            continue
        if rom_value != port_value:
            mismatch(mismatches, frame, path, rom_value, port_value)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--rom-trace", required=True)
    parser.add_argument("--port-trace", required=True)
    parser.add_argument("--port-frame-offset", type=int, default=1,
                        help="Port scene frame corresponding to ROM frame 0")
    parser.add_argument("--mode", choices=("core", "all"), default="core")
    parser.add_argument("--max-mismatches", type=int, default=100)
    parser.add_argument("--report")
    parser.add_argument("--report-only", action="store_true")
    args = parser.parse_args()

    rom_rows = load_rows(args.rom_trace)
    port_rows = load_rows(args.port_trace)
    pairs, missing, mismatches = 0, [], []
    compare = compare_core if args.mode == "core" else compare_all
    for rom_frame in sorted(rom_rows):
        port_frame = rom_frame + args.port_frame_offset
        if port_frame not in port_rows:
            missing.append({"rom_frame": rom_frame, "port_frame": port_frame})
            continue
        pairs += 1
        compare(rom_rows[rom_frame], port_rows[port_frame], rom_frame, mismatches)
        if len(mismatches) >= args.max_mismatches:
            break
    report = {"mode": args.mode, "port_frame_offset": args.port_frame_offset,
              "compared_frames": pairs, "missing_frames": missing,
              "mismatch_count": len(mismatches), "mismatches": mismatches}
    if args.report:
        Path(args.report).write_text(json.dumps(report, indent=2) + "\n")
    status = "PASS" if not mismatches and not missing and pairs else "FAIL"
    print(f"[GAMEPLAY TRACE] {status}: mode={args.mode} frames={pairs} "
          f"missing={len(missing)} mismatches={len(mismatches)}")
    for item in mismatches[:10]:
        print(f"  f={item['frame']} {item['path']}: "
              f"ROM={item['rom']} PORT={item['port']}")
    if status == "FAIL" and not args.report_only:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
