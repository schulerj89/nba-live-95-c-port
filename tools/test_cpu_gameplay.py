"""Regression checks for the post-tip CPU-versus-CPU gameplay state."""

import argparse
import hashlib
import json
import subprocess
import tempfile
from pathlib import Path

from PIL import Image


FORMATION = [
    (8, 3), (-16, -83), (-24, 80), (104, -56), (96, 59),
    (-8, -3), (16, 83), (24, -80), (-104, 56), (-96, -59),
]
EXPECTED_RGB = {
    240: "cea41b8085ab1bb7ec125af9f69b9732115ad5fc17c6385269705546e0f25557",
    450: "ef3785340ba7b91ef9cdf7f7e04cc6bdf81d534fc3ef79be492f45b8ba164fc9",
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
            "--tipoff-only", "--frames", "520", "--gameplay-trace", str(trace),
            "--debug-state",
        ]
        result = subprocess.run(command, capture_output=True, text=True, check=False)
        if result.returncode or "CPU:CPU" not in result.stdout:
            raise AssertionError(result.stdout + result.stderr)
        rows = [json.loads(line) for line in trace.read_text().splitlines()]
        if len(rows) != 520:
            raise AssertionError(f"expected 520 CPU frames, got {len(rows)}")

        def frame(number):
            return rows[number - 1]

        before = frame(219)
        live = frame(240)
        late = frame(500)
        if before["possession"]["play_code_raw"] != 0x35:
            raise AssertionError("ROM play $35 was not selected after the tip")
        if any(actor["control"] != 0 for actor in late["actors"]):
            raise AssertionError("CPU-versus-CPU mode assigned a human actor")
        moved = sum((actor["x"], actor["y"]) != FORMATION[index]
                    for index, actor in enumerate(live["actors"]))
        if moved < 8:
            raise AssertionError(f"only {moved}/10 CPU actors broke formation")
        if (live["actors"][0]["x"], live["actors"][0]["y"]) != FORMATION[0]:
            raise AssertionError("center reaction delay no longer matches the ROM trace")
        if live["actors"][8]["raw"]["assignment_current"] != 4:
            raise AssertionError("ballhandler assignment did not change to traced target 4")
        if live["actors"][8]["animation"] == 0:
            raise AssertionError("CPU ballhandler did not enter a movement animation")
        if frame(350)["camera"] == frame(220)["camera"]:
            raise AssertionError("camera did not follow live CPU play")
        owners = {frame(number)["ball"]["owner"] for number in (420, 450, 480, 500)}
        if not {8, 9}.issubset(owners):
            raise AssertionError(f"CPU pass did not transfer ball ownership: {owners}")
        if late["actors"][8]["x"] == live["actors"][8]["x"]:
            raise AssertionError("ballhandler stopped after the post-tip break")

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

    source = Path(__file__).parents[1] / "src" / "nba_tipoff.c"
    text = source.read_text()
    for marker in ("$85:F34F", "$85:BC43-$BC81", "$85:B95C",
                   "$87:B832", "$85:8EE6", "cpu_set_targets",
                   "cpu_move_actor", "cpu_update_ball", "cpu_update_camera"):
        if marker not in text:
            raise AssertionError(f"CPU gameplay implementation lost {marker}")
    print("CPU-versus-CPU gameplay regression checks passed")


if __name__ == "__main__":
    main()
