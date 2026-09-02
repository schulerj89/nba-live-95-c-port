"""Capture consecutive gameplay frames and natural dribble calls in private Mesen."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess

from PIL import Image
from mesen_portable import prepare, verify
from regenerate_dribble_reference import ROM_SHA256
from normalize_ball_driver_owned_vectors import INPUT_FIELDS, OUTPUT_FIELDS


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def freeze_draw_vectors(out):
    out = Path(out)
    cases = [json.loads(line) for line in (out / "poses.jsonl").read_text().splitlines()]
    fixture = {"schema": 1, "rom_sha256": ROM_SHA256, "state_injection": False,
               "native_entry": "80AF1E", "native_exit": "80B0AB",
               "mesen_sha256": sha(out / "portable-mesen/Mesen.exe"),
               "script_sha256": sha(out / "capture.lua"),
               "source_poses_sha256": sha(out / "poses.jsonl"),
               "input_fields": ["upper_d6", "lower_d4", "head_da", "number_d8",
                                "flags_47", "head_order_51", "movement_c0", "attribute_4f",
                                "glyph_work_0884", "x", "y", "ball_order_9a",
                                "before_owner_3f31", "rim_resource_4015", "effect_gate_3f33",
                                "ball_attribute", "ball_x", "ball_y"],
               "part_fields": ["resource", "attribute", "x", "y"], "cases": cases}
    (out / "draw-vectors.json").write_text(json.dumps(fixture, indent=2) + "\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--frames", type=int, default=240)
    parser.add_argument("--mesen", type=Path, default=shutil.which("Mesen.exe"))
    args = parser.parse_args()
    if sha(args.rom) != ROM_SHA256 or not 120 <= args.frames <= 1200:
        raise ValueError("wrong ROM or capture duration")
    if args.mesen is None:
        raise ValueError("Mesen executable is required")
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    exe, isolation = prepare(out, args.mesen)
    script = out / "capture.lua"
    shutil.copyfile(Path(__file__).with_name("mesen_dribble_capture.lua"), script)
    env = {k: v for k, v in os.environ.items() if not k.startswith("NBA95_")}
    env.update(NBA95_CAPTURE_DIR=out.as_posix(), NBA95_DRIBBLE_FRAMES=str(args.frames))
    command = [str(exe), "--testrunner", "--timeout=180", str(args.rom.resolve()), str(script)]
    run = subprocess.run(command, cwd=exe.parent, env=env, capture_output=True,
                         timeout=200, creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
    (out / "stdout.txt").write_bytes(run.stdout)
    (out / "stderr.txt").write_bytes(run.stderr)
    manifest = {"command": command, "exit_code": run.returncode,
                "rom_sha256": sha(args.rom), "script_sha256": sha(script),
                "mesen_sha256": sha(exe), "isolation": isolation, "frames": []}
    if run.returncode == 0:
        manifest["completion"] = (out / "capture_complete.txt").read_text().strip()
        verify(out, isolation)
        for i in range(1, args.frames + 1):
            rgb = out / f"frame_{i:04d}.rgb"
            if rgb.stat().st_size != 256 * 239 * 3:
                raise AssertionError(f"missing/truncated native frame {i}")
            png = rgb.with_suffix(".png")
            Image.frombytes("RGB", (256, 239), rgb.read_bytes()).crop((0, 7, 256, 231)).save(png)
            oam = rgb.with_suffix(".oam")
            if oam.stat().st_size != 0x220:
                raise AssertionError(f"missing/truncated native OAM {i}")
            manifest["frames"].append({"frame": i, "png": png.name, "sha256": sha(png),
                                       "oam_sha256": sha(oam)})
        manifest["calls_sha256"] = sha(out / "calls.jsonl")
        manifest["trace_sha256"] = sha(out / "frames.jsonl")
        manifest["draws_sha256"] = sha(out / "draws.jsonl")
        manifest["poses_sha256"] = sha(out / "poses.jsonl")
        cases = [json.loads(line) for line in (out / "calls.jsonl").read_text().splitlines()]
        fixture = {"schema": 1, "rom_sha256": ROM_SHA256, "state_injection": False,
                   "native_entry": "859A37", "native_exit": "85A7C7",
                   "mesen_sha256": sha(exe), "script_sha256": sha(script),
                   "source_calls_sha256": manifest["calls_sha256"],
                   "input_fields": INPUT_FIELDS, "output_fields": OUTPUT_FIELDS,
                   "cases": cases}
        (out / "vectors.json").write_text(json.dumps(fixture, indent=2) + "\n")
        freeze_draw_vectors(out)
    (out / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps({"exit_code": run.returncode, "frames": len(manifest["frames"]),
                      "output": str(out)}))
    if run.returncode:
        raise SystemExit(run.returncode)


if __name__ == "__main__":
    main()
