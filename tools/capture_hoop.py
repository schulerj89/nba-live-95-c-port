"""Capture both baskets, raw PPU inputs and raster writes in private Mesen."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess

from PIL import Image
from mesen_portable import prepare, verify
from regenerate_hoop_reference import ROM_SHA256, sha


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("rom", "output", "mesen"):
        parser.add_argument("--" + name, type=Path, required=True)
    parser.add_argument("--frames", type=int, default=24)
    args = parser.parse_args()
    if sha(args.rom) != ROM_SHA256 or not 8 <= args.frames <= 120:
        raise ValueError("wrong ROM or capture duration")
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    exe, isolation = prepare(out, args.mesen)
    script = out / "capture.lua"
    shutil.copyfile(Path(__file__).with_name("mesen_hoop_capture.lua"), script)
    env = {k: v for k, v in os.environ.items() if not k.startswith("NBA95_")}
    env.update(NBA95_CAPTURE_DIR=out.as_posix(), NBA95_HOOP_FRAMES=str(args.frames))
    command = [str(exe), "--testrunner", "--timeout=180", str(args.rom.resolve()), str(script)]
    run = subprocess.run(command, cwd=exe.parent, env=env, capture_output=True,
                         timeout=200, creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
    (out / "stdout.txt").write_bytes(run.stdout)
    (out / "stderr.txt").write_bytes(run.stderr)
    manifest = {"command": command, "exit_code": run.returncode, "state_injection": False,
                "rom_sha256": sha(args.rom), "script_sha256": sha(script),
                "mesen_sha256": sha(exe), "isolation": isolation, "frames": []}
    if run.returncode == 0:
        manifest["completion"] = (out / "capture_complete.txt").read_text().strip()
        verify(out, isolation)
        rows = [json.loads(line) for line in (out / "frames.jsonl").read_text().splitlines()]
        if len(rows) != args.frames * 2:
            raise AssertionError("incomplete native capture")
        for side in ("north", "south"):
            group = [r for r in rows if r["name"].startswith(side)]
            if [r["frame"] for r in group] != list(range(group[0]["frame"], group[0]["frame"] + args.frames)):
                raise AssertionError("dropped native frame")
        for row in rows:
            prefix = out / row["name"]
            files = {}
            for suffix, length in ((".rgb", 256 * 239 * 3), (".vram", 65536),
                                   (".cgram", 512), (".oam", 544)):
                path = prefix.with_suffix(suffix)
                if path.stat().st_size != length:
                    raise AssertionError(f"truncated capture: {path}")
                files[suffix] = sha(path)
            png = prefix.with_suffix(".png")
            Image.frombytes("RGB", (256, 239), prefix.with_suffix(".rgb").read_bytes()).crop((0, 7, 256, 231)).save(png)
            manifest["frames"].append({**row, "sha256": sha(png), "inputs": files})
    (out / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps({"exit_code": run.returncode, "frames": len(manifest["frames"]), "output": str(out)}))
    if run.returncode:
        raise SystemExit(run.returncode)


if __name__ == "__main__":
    main()
