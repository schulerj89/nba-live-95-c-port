"""Long-running regression checks for ROM-derived CPU-versus-CPU gameplay."""

import argparse
import hashlib
import json
import math
import subprocess
import sys
import tempfile
from pathlib import Path

from PIL import Image


FORMATION = [
    (8, 3), (-16, -83), (-24, 80), (104, -56), (96, 59),
    (-8, -3), (16, 83), (24, -80), (-104, 56), (-96, -59),
]
EXPECTED_RGB = {
    600: "7c4a8e81cf354ef3eff358e8ccc7d0eafc2b631e86de853784e0e45bd9e61301",
    1300: "7e47b104411142d532be5b71c29d2f3dfe51c189aa6b89c4cfaac914e56a3250",
}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--pack", required=True)
    parser.add_argument("--exe", required=True)
    parser.add_argument("--rom", required=True)
    args = parser.parse_args()

    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        trace = root / "cpu_gameplay.jsonl"
        command = [
            args.exe, "--headless", "--rom", args.rom, "--assets", args.pack,
            "--tipoff-only", "--frames", "2000", "--gameplay-trace", str(trace),
            "--debug-state",
        ]
        result = subprocess.run(command, capture_output=True, text=True, check=False)
        if result.returncode or "INT:$85:9700" not in result.stdout or \
                "BALL M:" not in result.stdout:
            raise AssertionError(result.stdout + result.stderr)
        rows = [json.loads(line) for line in trace.read_text().splitlines()]
        if len(rows) != 2000:
            raise AssertionError(f"expected 2000 CPU frames, got {len(rows)}")

        def frame(number):
            return rows[number - 1]

        live = frame(240)
        if any(actor["control"] != 0 for actor in frame(1900)["actors"]):
            raise AssertionError("CPU-versus-CPU mode assigned a human actor")
        moved = sum((actor["x"], actor["y"]) != FORMATION[index]
                    for index, actor in enumerate(live["actors"]))
        if moved < 8:
            raise AssertionError(f"only {moved}/10 CPU actors broke formation")
        if (live["actors"][0]["x"], live["actors"][0]["y"]) != FORMATION[0]:
            raise AssertionError("center reaction delay no longer matches the ROM trace")

        play_codes = {row["possession"]["play_code_raw"] for row in rows[219:]}
        if not {0x35, 0x01, 0x0F, 0x26}.issubset(play_codes):
            raise AssertionError(f"recurring ROM play codes missing: {play_codes}")
        teams = {row["possession"]["team"] for row in rows[219:]}
        if not {0, 1}.issubset(teams):
            raise AssertionError(f"CPU offense did not change sides: {teams}")
        modes = {row["ball"]["state"] for row in rows[219:]}
        if not {3, 4, 5, 6}.issubset(modes):
            raise AssertionError(f"ball physics modes missing: {modes}")
        owners = {row["ball"]["owner"] for row in rows[219:]}
        if len(owners - {-1}) < 4:
            raise AssertionError(f"ballhandler did not rotate: {owners}")

        def signed16(value):
            return value - 0x10000 if value & 0x8000 else value

        prior_dx = prior_dy = 0
        camera_positions = set()
        for row in rows[199:]:
            camera = row["camera"]
            if camera["routine"] != 0x859192:
                raise AssertionError("camera telemetry lost $85:9192 attribution")
            x, y = camera["x"], camera["y"]
            if not (-582 <= x <= 328 and -242 <= y <= -53):
                raise AssertionError(f"camera escaped ROM bounds: {(x, y)}")
            dx = abs(signed16(camera["raw_085c"]) -
                     signed16(camera["raw_085e"]))
            dy = abs(signed16(camera["raw_0860"]) -
                     signed16(camera["raw_0862"]))
            if dx > 22 or dy > 22 or dx > prior_dx + 2 or dy > prior_dy + 2:
                raise AssertionError(
                    f"$85:9352 camera cadence changed: {prior_dx, prior_dy} -> {dx, dy}")
            prior_dx, prior_dy = dx, dy
            expected_source = (0x8006 + camera["raw_086c"] * 104 +
                               camera["raw_086e"] * 2) & 0xFFFF
            if camera["raw_0876"] != expected_source:
                raise AssertionError("$85:8EE6 court source pointer changed")
            camera_positions.add((x, y))
        if len(camera_positions) < 25:
            raise AssertionError("camera did not follow CPU play")

        attached = []
        for row in rows[219:]:
            owner = row["ball"]["owner"]
            if row["ball"]["state"] == 4 and owner >= 0:
                actor = row["actors"][owner]
                attached.append(math.hypot(row["ball"]["x"] - actor["x"],
                                           row["ball"]["y"] - actor["y"]))
        if not attached or max(attached) > 20.0:
            raise AssertionError("ball is not attached at the actor hand offset")

        for first in range(220, 1900, 240):
            last = min(first + 239, len(rows) - 1)
            counts = []
            for actor in range(10):
                counts.append(sum(
                    (rows[index]["actors"][actor]["x"],
                     rows[index]["actors"][actor]["y"]) !=
                    (rows[index - 1]["actors"][actor]["x"],
                     rows[index - 1]["actors"][actor]["y"])
                    for index in range(first + 1, last + 1)))
            if min(sum(counts[:5]), sum(counts[5:])) < 60:
                raise AssertionError(
                    f"CPU team became stationary in frames {first}-{last}: {counts}")

        analyzer = Path(__file__).with_name("analyze_cpu_gameplay_trace.py")
        analyzed = subprocess.run(
            [sys.executable, str(analyzer), str(trace), "--require-sustained"],
            capture_output=True, text=True, check=False)
        if analyzed.returncode or "PASS" not in analyzed.stdout:
            raise AssertionError(analyzed.stdout + analyzed.stderr)

        for number, expected in EXPECTED_RGB.items():
            output = root / f"cpu_{number}.bmp"
            render = subprocess.run([
                args.exe, "--headless", "--rom", args.rom, "--assets", args.pack,
                "--tipoff-only", "--frames", str(number), "--dump-frame", str(output),
            ], capture_output=True, text=True, check=False)
            if render.returncode:
                raise AssertionError(render.stdout + render.stderr)
            digest = hashlib.sha256(Image.open(output).convert("RGB").tobytes()).hexdigest()
            if digest != expected:
                raise AssertionError(f"CPU gameplay frame {number} changed: {digest}")

    source = Path(__file__).parents[1]
    implementation = (source / "src" / "nba_tipoff.c").read_text()
    for marker in ("$85:9700-$985F", "$85:BC43-$BC81", "$85:B95C",
                   "$87:B832", "$87:B649", "$87:B66A", "$85:9192",
                   "nba_gameplay_camera_update",
                   "cpu_begin_possession", "cpu_update_possession",
                   "ball_attach_to_actor", "ball_launch"):
        if marker not in implementation:
            raise AssertionError(f"CPU gameplay implementation lost {marker}")
    for relative in ("tools/ghidra/DumpCpuGameplay.java",
                     "tools/ghidra/Run-CpuGameplayAnalysis.ps1"):
        if not (source / relative).is_file():
            raise AssertionError(f"CPU Ghidra evidence tool missing: {relative}")
    print("CPU-versus-CPU gameplay regression checks passed")


if __name__ == "__main__":
    main()
