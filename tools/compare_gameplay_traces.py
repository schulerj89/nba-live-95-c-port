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


def logical_pass_rows(rows, path):
    """Coalesce ROM NMI-split actor slices into completed 0..9 passes.

    `$87:8EFB-$8F92` is atomic game logic, but an SNES NMI can interrupt it
    between actors and make one pass appear in two rendered-frame JSON rows.
    Native C emits the whole logical pass in one row.
    """
    completed, pending = [], []
    for frame in sorted(rows):
        row = rows[frame]
        order = row.get("scheduler", {}).get("actor_pass_order_raw", [])
        if not order:
            continue
        pending.extend(int(actor) for actor in order)
        if pending == list(range(10)):
            completed.append(row)
            pending = []
        elif pending != list(range(len(pending))):
            raise ValueError(
                f"{path}: invalid logical actor pass at scene_frame {frame}: {pending}")
    if pending:
        raise ValueError(f"{path}: incomplete trailing logical actor pass: {pending}")
    if not completed:
        raise ValueError(f"{path}: no completed logical actor passes")
    return completed


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
    parser.add_argument("--logical-passes", action="store_true",
                        help="compare completed $87:8EFB actor passes, coalescing ROM NMI slices")
    parser.add_argument("--pass-offset", type=int, default=0,
                        help="Port logical pass corresponding to ROM pass zero")
    parser.add_argument("--max-mismatches", type=int, default=100)
    parser.add_argument("--report")
    parser.add_argument("--report-only", action="store_true")
    args = parser.parse_args()

    rom_rows = load_rows(args.rom_trace)
    port_rows = load_rows(args.port_trace)
    pairs, missing, mismatches = 0, [], []
    compare = compare_core if args.mode == "core" else compare_all
    if args.logical_passes:
        rom_passes = logical_pass_rows(rom_rows, args.rom_trace)
        port_passes = logical_pass_rows(port_rows, args.port_trace)
        for rom_index, rom_row in enumerate(rom_passes):
            port_index = rom_index + args.pass_offset
            if port_index < 0 or port_index >= len(port_passes):
                missing.append({"rom_pass": rom_index, "port_pass": port_index})
                continue
            pairs += 1
            compare(rom_row, port_passes[port_index], rom_index, mismatches)
            if len(mismatches) >= args.max_mismatches:
                break
    else:
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
              "logical_passes": args.logical_passes,
              "pass_offset": args.pass_offset,
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
