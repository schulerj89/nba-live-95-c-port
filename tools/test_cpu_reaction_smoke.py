"""Press through setup and capture the first live CPU role-reaction rebuild."""

import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import time

from PIL import Image, ImageDraw

from run_visible_smoke_checkpoints import FRONTEND_INPUT
from test_headless_input import read_trace, verify_inputs


ROOT = Path(__file__).resolve().parents[1]
ROM_SHA256 = "2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870"


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def write_contacts(directory, rows):
    pages = []
    for start in range(0, len(rows), 12):
        group = rows[start:start + 12]
        sheet = Image.new("RGB", (4 * 256, 3 * 244), "#111820")
        draw = ImageDraw.Draw(sheet)
        for index, row in enumerate(group):
            frame = row["frame"]
            image = Image.open(directory / f"frame_{frame:04d}.png").convert("RGB")
            x, y = (index % 4) * 256, (index // 4) * 244
            sheet.paste(image, (x, y))
            marker = "  REBUILD" if row.get("reaction_rebuild") else ""
            draw.text((x + 4, y + 226), f"frame {frame}{marker}", fill="white")
        name = f"contact_{start // 12 + 1:02d}.png"
        sheet.save(directory / name)
        pages.append(name)
    return pages


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--exe", type=Path,
                        default=ROOT / "build" / "nba95_port.exe")
    parser.add_argument("--pack", type=Path,
                        default=ROOT / "build" / "nba95_assets.pak")
    parser.add_argument("--output", type=Path, default=ROOT / "build" /
                        time.strftime("cpu-reaction-smoke-%Y%m%d-%H%M%S"))
    parser.add_argument("--search-frames", type=int, default=1000)
    args = parser.parse_args()
    if sha(args.rom) != ROM_SHA256 or not 800 <= args.search_frames <= 5000:
        raise ValueError("wrong ROM or search duration")

    started = time.monotonic()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    script = output / "buttons.rle"
    script.write_text(FRONTEND_INPUT, encoding="ascii")
    expected_inputs = []
    for line in FRONTEND_INPUT.splitlines():
        duration, word = line.split()
        expected_inputs.extend([int(word, 16)] * int(duration))

    common = [str(args.exe.resolve()), "--headless", "--rom",
              str(args.rom.resolve()), "--assets", str(args.pack.resolve()),
              "--team-only", "--input-script", str(script)]
    commands = []

    def run(name, frames, extra=()):
        trace = output / f"{name}.jsonl"
        inputs = output / f"{name}-inputs.csv"
        command = common + ["--frames", str(frames), "--gameplay-trace",
                            str(trace), "--input-trace", str(inputs),
                            *map(str, extra)]
        result = subprocess.run(
            command, capture_output=True, text=True, timeout=90,
            creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
        (output / f"{name}.log").write_text(
            result.stdout + result.stderr, encoding="utf-8")
        commands.append({"args": command, "exit_code": result.returncode})
        if result.returncode or result.stdout.count(
                "[HEADLESS] Headless execution completed successfully.") != 1:
            raise AssertionError(f"{name} did not complete; see retained log")
        input_rows = read_trace(inputs, frames)
        verify_inputs(input_rows,
                      (expected_inputs + [0] * frames)[:frames])
        rows = [json.loads(line) for line in trace.read_text().splitlines()]
        return rows, input_rows

    search, search_inputs = run("search", args.search_frames)
    rebuild = None
    for before, after in zip(search, search[1:]):
        if before["possession"]["role_rebuild_raw_09d6"] != 0 and \
                after["possession"]["role_rebuild_raw_09d6"] == 0 and \
                after["phase"] >= 2:
            rebuild = before, after
            break
    if rebuild is None:
        raise AssertionError("no natural live role rebuild was reached")

    before, after = rebuild
    if after["frame"] != before["frame"] + 1:
        raise AssertionError("role rebuild telemetry is not consecutive")
    before_rng = before["possession"]["rng_state_raw"]
    after_rng = after["possession"]["rng_state_raw"]
    if before_rng == after_rng:
        raise AssertionError("role rebuild did not advance the shared RNG")
    eligible, skipped, changed = [], [], []
    for old_actor, new_actor in zip(before["actors"], after["actors"]):
        actor = new_actor["id"]
        old = old_actor["raw"]
        new = new_actor["raw"]
        if new["control_mode"] == 10:
            skipped.append(actor)
            if new["reaction_threshold"] != old["reaction_threshold"]:
                raise AssertionError("native mode-10 exclusion reloaded a timer")
            continue
        eligible.append(actor)
        if new["reaction_threshold"] <= 0 or new["behavior_flags"] != 0:
            raise AssertionError("eligible CPU actor lacks a clean reaction reload")
        if new["reaction_threshold"] != old["reaction_threshold"]:
            changed.append(actor)
    if len(eligible) < 9 or len(changed) < 9 or not skipped:
        raise AssertionError("role rebuild did not exercise the expected actor set")

    event_frame = after["frame"]
    first, last = event_frame - 12, event_frame + 24
    frames = output / "frames"
    frames.mkdir()
    replay, replay_inputs = run(
        "capture", last,
        ["--dump-sequence-from", first, "--dump-sequence-dir", frames,
         "--dump-sequence-layers"])
    expected_replay = [row for row in search if row["frame"] <= last]
    if replay != expected_replay:
        raise AssertionError("capture replay diverged from the search run")
    captured = [row for row in replay if first <= row["frame"] <= last]
    if [row["frame"] for row in captured] != list(range(first, last + 1)):
        raise AssertionError("capture trace dropped or reordered frames")
    if len(list(frames.glob("*.bmp"))) != len(captured):
        raise AssertionError("capture dropped rendered frames")

    frame_records = []
    for row in captured:
        frame = row["frame"]
        prefix = frames / f"frame_{frame:04d}"
        with Image.open(prefix.with_suffix(".bmp")) as source:
            image = source.convert("RGB")
        layer_data = prefix.with_suffix(".layers").read_bytes()
        if image.size != (256, 224) or len(layer_data) != 32 + 256 * 224 or \
                layer_data[:8] != b"NBLAYER1":
            raise AssertionError(f"frame {frame} has invalid render evidence")
        if struct.unpack_from("<I", layer_data, 16)[0] != frame or \
                list(struct.unpack_from("<hh", layer_data, 24)) != \
                [row["camera"]["x"], row["camera"]["y"]]:
            raise AssertionError(f"frame {frame} layer header differs from gameplay")
        png = prefix.with_suffix(".png")
        image.save(png)
        record = {
            "frame": frame,
            "file": str(png.relative_to(output)).replace("\\", "/"),
            "sha256": sha(png),
            "camera": [row["camera"]["x"], row["camera"]["y"]],
            "reaction_rebuild": frame == event_frame,
        }
        row["reaction_rebuild"] = frame == event_frame
        frame_records.append(record)
    pages = [f"frames/{name}" for name in write_contacts(frames, captured)]
    Image.open(frames / f"frame_{event_frame:04d}.png").save(
        output / "reaction-rebuild.png")

    report = {
        "status": "PASS",
        "seconds": round(time.monotonic() - started, 3),
        "rom_sha256": ROM_SHA256,
        "exe_sha256": sha(args.exe),
        "pack_sha256": sha(args.pack),
        "state_injection": False,
        "through_menus": True,
        "button_presses": sum(bool(row["pressed"]) for row in search_inputs),
        "deterministic_replay_button_presses": sum(
            bool(row["pressed"]) for row in replay_inputs),
        "event_frame": event_frame,
        "rng": {"before": before_rng, "after": after_rng},
        "eligible_actors": eligible,
        "changed_timers": changed,
        "skipped_mode_10_actors": skipped,
        "timers_before": [a["raw"]["reaction_threshold"]
                          for a in before["actors"]],
        "timers_after": [a["raw"]["reaction_threshold"]
                         for a in after["actors"]],
        "captured_frames": len(captured),
        "contact_pages": pages,
        "commands": commands,
        "frames": frame_records,
    }
    (output / "report.json").write_text(
        json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({key: report[key] for key in
                      ("status", "button_presses", "event_frame",
                       "captured_frames", "changed_timers")}))
    print(output / "reaction-rebuild.png")


if __name__ == "__main__":
    main()
