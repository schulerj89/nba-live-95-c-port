"""Capture native court uploads and every early Tipoff frame in private Mesen."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess

from PIL import Image
from mesen_portable import prepare, verify

ROM_SHA256 = "2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870"


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--team", type=int, required=True, choices=range(29))
    parser.add_argument("--frames", type=int, default=180)
    parser.add_argument("--mesen", type=Path, default=shutil.which("Mesen.exe"))
    args = parser.parse_args()
    if sha(args.rom) != ROM_SHA256 or not 160 <= args.frames <= 400:
        raise ValueError("wrong ROM or capture duration")
    if args.mesen is None:
        raise ValueError("Mesen executable is required")
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    exe, isolation = prepare(out, args.mesen)
    script = out / "capture.lua"
    shutil.copyfile(Path(__file__).with_name("mesen_court_logo_capture.lua"), script)
    env = {k: v for k, v in os.environ.items() if not k.startswith("NBA95_")}
    env.update(NBA95_CAPTURE_DIR=out.as_posix(), NBA95_HOME_TEAM=str(args.team),
               NBA95_COURT_FRAMES=str(args.frames))
    command = [str(exe), "--testrunner", "--timeout=180", str(args.rom.resolve()), str(script)]
    run = subprocess.run(command, cwd=exe.parent, env=env, capture_output=True,
                         timeout=200, creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
    (out / "stdout.txt").write_bytes(run.stdout)
    (out / "stderr.txt").write_bytes(run.stderr)
    manifest = {"command": command, "exit_code": run.returncode, "team": args.team,
                "rom_sha256": sha(args.rom), "script_sha256": sha(script),
                "mesen_sha256": sha(exe), "isolation": isolation, "frames": []}
    if run.returncode == 0:
        marker = (out / "capture_complete.txt").read_text()
        if f"requested={args.team} observed={args.team}" not in marker:
            raise AssertionError("home identity not verified")
        verify(out, isolation)
        for i in range(1, args.frames + 1):
            rgb = out / f"frame_{i:04d}.rgb"
            if rgb.stat().st_size != 256 * 239 * 3:
                raise AssertionError(f"missing/truncated native frame {i}")
            png = rgb.with_suffix(".png")
            Image.frombytes("RGB", (256, 239), rgb.read_bytes()).crop(
                (0, 7, 256, 231)).save(png)
            manifest["frames"].append({"frame": i, "png": png.name, "sha256": sha(png)})
        manifest["uploads"] = [json.loads(line) for line in
                               (out / "uploads.jsonl").read_text().splitlines()]
    (out / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps({"team": args.team, "exit_code": run.returncode,
                      "frames": len(manifest["frames"]), "output": str(out)}))
    if run.returncode:
        raise SystemExit(run.returncode)


if __name__ == "__main__":
    main()
