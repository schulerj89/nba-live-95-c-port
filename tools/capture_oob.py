"""Capture the original out-of-bounds HUD in a private headless Mesen game."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess

from PIL import Image
from mesen_portable import prepare, verify
from regenerate_oob_reference import ROM_SHA256, sha


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("rom", "output", "mesen"):
        parser.add_argument("--" + name, type=Path, required=True)
    parser.add_argument("--controlled", action="store_true",
                        help="position the grounded owner outside the sideline before the native detector")
    args = parser.parse_args()
    if sha(args.rom) != ROM_SHA256:
        raise ValueError("expected verified USA ROM")
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    exe, isolation = prepare(out, args.mesen)
    script = out / "capture.lua"
    shutil.copyfile(Path(__file__).with_name("mesen_oob_capture.lua"), script)
    env = {k: v for k, v in os.environ.items() if not k.startswith("NBA95_")}
    env["NBA95_CAPTURE_DIR"] = out.as_posix()
    env["NBA95_OOB_CONTROLLED"] = "1" if args.controlled else "0"
    command = [str(exe), "--testrunner", "--timeout=180", str(args.rom.resolve()), str(script)]
    run = subprocess.run(command, cwd=exe.parent, env=env, capture_output=True,
                         timeout=200, creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
    (out / "stdout.txt").write_bytes(run.stdout)
    (out / "stderr.txt").write_bytes(run.stderr)
    manifest = {"command": command, "exit_code": run.returncode, "state_injection": args.controlled,
                "rom_sha256": sha(args.rom), "script_sha256": sha(script),
                "runner_sha256": sha(__file__), "mesen_sha256": sha(exe),
                "isolation": isolation, "frames": []}
    if run.returncode == 0:
        if args.controlled:
            manifest["scenario"] = json.loads((out / "scenario.json").read_text())
        manifest["completion"] = (out / "capture_complete.txt").read_text().strip()
        verify(out, isolation)
        calls = [json.loads(line) for line in (out / "calls.jsonl").read_text().splitlines()]
        if [c["pc"] for c in calls] != [0x83DA12, 0x83DA8C, 0x83EBDB]:
            raise AssertionError("incomplete original OOB lifecycle")
        manifest["calls"] = calls
        for row in map(json.loads, (out / "frames.jsonl").read_text().splitlines()):
            if not row["name"]:
                continue
            prefix = out / row["name"]
            raw = prefix.with_suffix(".rgb").read_bytes()
            png = prefix.with_suffix(".png")
            Image.frombytes("RGB", (256, 239), raw).crop((0, 7, 256, 231)).save(png)
            manifest["frames"].append({**row, "sha256": sha(png)})
        manifest["artifacts"] = {p.name: sha(p) for p in out.iterdir()
                                 if p.is_file() and p.suffix in (".wram", ".vram", ".cgram", ".jsonl")}
    (out / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps({"exit_code": run.returncode, "frames": len(manifest["frames"]), "output": str(out)}))
    if run.returncode:
        raise SystemExit(run.returncode)


if __name__ == "__main__":
    main()
