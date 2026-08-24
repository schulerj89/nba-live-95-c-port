"""Regression checks for the F8 Gameplay Lab and JSONL telemetry contract."""

import argparse
import hashlib
import json
import subprocess
import sys
import tempfile
from pathlib import Path

from PIL import Image


EXPECTED_LAB_RGB = "e0d0a016bd6f2488b0d99143f714950a5d528d2aed42aa9db4f8e90faff6a537"


def run(command, label):
    result = subprocess.run(command, capture_output=True, text=True, check=False)
    if result.returncode:
        raise AssertionError(f"{label} failed\n{result.stdout}{result.stderr}")
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--pack", required=True)
    parser.add_argument("--exe", required=True)
    parser.add_argument("--rom", required=True)
    args = parser.parse_args()
    base = [args.exe, "--headless", "--rom", args.rom, "--assets", args.pack,
            "--tipoff-only"]

    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        trace = root / "gameplay.jsonl"
        frame = root / "gameplay_lab.bmp"
        result = run(base + ["--frames", "170", "--gameplay-trace", str(trace),
                             "--gameplay-lab", "--gameplay-actor", "7",
                             "--gameplay-page", "2", "--dump-frame", str(frame)],
                     "Gameplay Lab trace/render")
        if "Wrote 170 gameplay JSONL rows" not in result.stdout:
            raise AssertionError("Gameplay trace row diagnostic missing")
        rows = [json.loads(line) for line in trace.read_text().splitlines()]
        if len(rows) != 170:
            raise AssertionError(f"expected 170 gameplay rows, got {len(rows)}")
        sample = rows[-1]
        if len(sample["actors"]) != 10 or [a["id"] for a in sample["actors"]] != list(range(10)):
            raise AssertionError("telemetry did not preserve all ten actor slots")
        if sum(bool(a["visible"]) for a in sample["actors"]) != 8:
            raise AssertionError("settled tip-off visibility telemetry changed")
        if sample["control"] != {
                "actor": 255, "side_raw": -1, "initial_slot_raw": 0,
                "selected_slot_raw": -1, "actor_pointer_raw": 0}:
            raise AssertionError(f"CPU-only control mapping changed: {sample['control']}")
        if any(actor["control"] != 0 for actor in sample["actors"]):
            raise AssertionError("tip-off introduced a human-controlled actor")
        required = {"base", "action", "flags", "control_mode", "side_group",
                    "target_x_56", "target_y_58", "mode_saved_62",
                    "assignment_current", "reaction_threshold",
                    "movement_boost_72", "upper_restart",
                    "lower_restart", "behavior_flags"}
        if not required.issubset(sample["actors"][0]["raw"]):
            raise AssertionError("AI/actor raw telemetry schema is incomplete")
        if len(sample["controllers"]["held_raw"]) != 5 or \
                not {"raw_087a", "subject_raw", "side_group_raw"}.issubset(
                    sample["camera"]) or "flags_raw" not in sample["ball"]:
            raise AssertionError("controller/camera/ball telemetry schema is incomplete")
        scheduler = sample["scheduler"]
        if set(scheduler) != {"due_raw", "actor_pass_dt_raw",
                             "actor_pass_mask_raw", "actor_pass_order_raw"}:
            raise AssertionError(f"scheduler telemetry schema is incomplete: {scheduler}")
        digest = hashlib.sha256(Image.open(frame).convert("RGB").tobytes()).hexdigest()
        if digest != EXPECTED_LAB_RGB:
            raise AssertionError(f"Gameplay Lab pixels changed: {digest}")

        paused = run(base + ["--gameplay-lab", "--gameplay-paused",
                             "--gameplay-step-count", "3", "--frames", "10",
                             "--debug-state"], "Gameplay Lab pause/step")
        if "GF:000003 SF:00003" not in paused.stdout:
            raise AssertionError("paused single-frame stepping did not advance exactly 3 frames")

        rom_proxy = root / "rom_proxy.jsonl"
        proxy_rows = []
        for row in rows[:4]:
            copied = json.loads(json.dumps(row))
            copied["source"] = "rom"
            copied["frame"] -= 1
            copied["scene_frame"] -= 1
            proxy_rows.append(copied)
        rom_proxy.write_text("".join(json.dumps(row) + "\n" for row in proxy_rows))
        comparator = Path(__file__).with_name("compare_gameplay_traces.py")
        compared = run([sys.executable, str(comparator), "--rom-trace", str(rom_proxy),
                        "--port-trace", str(trace)], "Gameplay trace comparator")
        if "PASS" not in compared.stdout or "frames=4" not in compared.stdout:
            raise AssertionError("aligned core comparison did not pass")
        proxy_rows[0]["actors"][0]["x"] += 1
        rom_proxy.write_text("".join(json.dumps(row) + "\n" for row in proxy_rows))
        failed = subprocess.run(
            [sys.executable, str(comparator), "--rom-trace", str(rom_proxy),
             "--port-trace", str(trace)], capture_output=True, text=True, check=False)
        if failed.returncode == 0 or "actors.0.x" not in failed.stdout:
            raise AssertionError("trace comparator did not reject an actor-position mismatch")

        # The ROM's strict 0..9 pass can cross an NMI/render boundary. Verify
        # the comparator coalesces prefix/suffix slices before C comparison.
        due_rows = [row for row in rows
                    if row["scheduler"]["actor_pass_order_raw"] == list(range(10))][:2]
        split_rows = []
        for index, row in enumerate(due_rows):
            for half, order in enumerate((list(range(5)), list(range(5, 10)))):
                copied = json.loads(json.dumps(row))
                copied["source"] = "rom"
                copied["scene_frame"] = index * 2 + half
                copied["frame"] = copied["scene_frame"]
                copied["scheduler"]["actor_pass_order_raw"] = order
                copied["scheduler"]["actor_pass_mask_raw"] = \
                    sum(1 << actor for actor in order)
                split_rows.append(copied)
        split_proxy = root / "rom_split_proxy.jsonl"
        split_proxy.write_text("".join(json.dumps(row) + "\n" for row in split_rows))
        logical = run([sys.executable, str(comparator),
                       "--rom-trace", str(split_proxy),
                       "--port-trace", str(trace), "--logical-passes"],
                      "logical actor-pass comparison")
        if "PASS" not in logical.stdout or "frames=2" not in logical.stdout:
            raise AssertionError("NMI-split actor pass did not coalesce")

    source = Path(__file__).parents[1]
    win32 = (source / "src" / "win32_game_main.c").read_text()
    debugger = (source / "src" / "nba_gameplay_debugger.c").read_text()
    for marker in ("VK_F8", "NBA_BTN_DEBUG_F8"):
        if marker not in win32:
            raise AssertionError(f"F8 input mapping lost {marker}")
    for marker in ("NBA_BTN_A", "NBA_BTN_X", "selected_actor",
                   "nba_gameplay_telemetry_write_jsonl"):
        if marker not in debugger:
            raise AssertionError(f"Gameplay Lab implementation lost {marker}")
    mesen = (source / "tools" / "mesen_tipoff_capture.lua").read_text()
    for marker in ("gameplay_rom.jsonl", "0x80cb8f", "0x879244", "0x85963d",
                   "0x0936", "score_left_raw",
                   "assignment_current", "raw_087a", "0x0998",
                   "play_step_raw", "play_selector_raw", "subject_raw"):
        if marker not in mesen:
            raise AssertionError(f"Mesen gameplay oracle lost {marker}")
    print("Gameplay Lab regression checks passed")


if __name__ == "__main__":
    main()
