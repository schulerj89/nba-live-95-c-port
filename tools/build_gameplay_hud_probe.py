"""Build a fresh HUD verification executable from current repository sources."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
NATIVE = ["tools/hud_native_lifecycle_probe.c", "src/nba_gameplay_hud.c",
          "src/nba_rom_font.c", "src/nba_assets.c", "src/nba_gameplay_ai.c",
          "src/nba_ea_intro.c", "src/nba_intro_text.c", "src/nba_renderer.c",
          "src/nba_snes_ppu.c"]
ENTRIES = {"runtime": "tools/hud_runtime_probe.c",
           "contract": "tools/hud_lifecycle_contract_probe.c"}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--kind", required=True, choices=["cli", "native", "runtime", "contract"])
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    names = [line.strip() for line in (ROOT / "nba95_sources.txt").read_text().splitlines()
             if line.strip() and not line.lstrip().startswith("#")]
    if args.kind == "native":
        names = NATIVE
    elif args.kind != "cli":
        names = [name for name in names if name != "src/main.c"] + [ENTRIES[args.kind]]
    headers = list((ROOT / "include").glob("*.h"))
    before = {name: sha(ROOT / name) for name in names}
    header_before = {path.name: sha(path) for path in headers}
    vswhere = Path(os.environ["ProgramFiles(x86)"]) / "Microsoft Visual Studio/Installer/vswhere.exe"
    result = subprocess.run([str(vswhere), "-latest", "-products", "*", "-requires",
                             "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
                             "-property", "installationPath"], capture_output=True, check=True)
    vcvars = Path(result.stdout.decode().strip()) / "VC/Auxiliary/Build/vcvars64.bat"
    if not vcvars.is_file():
        raise ValueError("MSVC C++ build tools are required")
    exe = out / ("hud_" + args.kind + ".exe")
    flags = f'/nologo /W4 /WX /O2 /MD /utf-8 /I "{ROOT / "include"}" /Fo"{out.as_posix()}/" /Fe"{exe}"\n'
    (out / "compile.rsp").write_text(flags + "\n".join(f'"{ROOT / name}"' for name in names) +
                                     "\nuser32.lib gdi32.lib winmm.lib\n", encoding="utf-8")
    batch = out / "compile.bat"
    batch.write_text(f'@echo off\ncall "{vcvars}" >nul\nif errorlevel 1 exit /b %ERRORLEVEL%\n'
                     'cl.exe @compile.rsp\nexit /b %ERRORLEVEL%\n', encoding="utf-8")
    env = {key: value for key, value in os.environ.items() if not key.startswith("NBA95")}
    run = subprocess.run(["cmd.exe", "/c", str(batch)], cwd=out, env=env,
                         capture_output=True, creationflags=subprocess.CREATE_NO_WINDOW)
    (out / "build.log").write_bytes(run.stdout + run.stderr)
    if run.returncode:
        raise RuntimeError(f"MSVC failed; retained log: {out / 'build.log'}")
    if before != {name: sha(ROOT / name) for name in names} or header_before != {
            path.name: sha(path) for path in (ROOT / "include").glob("*.h")}:
        raise RuntimeError("Sources changed during build; retained attempt is not accepted")
    (out / "manifest.json").write_text(json.dumps({"compiled_sources": before,
        "headers": header_before, "exe_sha256": sha(exe)}, indent=2) + "\n", encoding="utf-8")
    print(f"Built {args.kind}: {len(names)} sources, /W4 /WX; {exe}")


if __name__ == "__main__":
    main()
