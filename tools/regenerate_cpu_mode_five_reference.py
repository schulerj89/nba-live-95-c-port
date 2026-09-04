"""Generate fresh Ghidra and snesrecomp references for CPU mode five."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys


ROM_SHA256 = "2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870"
FIRST = 0xF2CA
END = 0xF34F
REQUIRED = (0xF2CA, 0xF2E3, 0xF2E7, 0xF2FA, 0xF31A, 0xF31F,
            0xF32B, 0xF32F, 0xF335, 0xF33D, 0xF345, 0xF346, 0xF34E)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("rom", "output", "recompiler", "ghidra", "jdk"):
        parser.add_argument("--" + name, type=Path, required=True)
    args = parser.parse_args()
    if sha(args.rom) != ROM_SHA256:
        raise ValueError("expected the verified USA ROM")
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    script = Path(__file__).with_name("ghidra") / "DumpCpuModeFive.java"
    shutil.copyfile(script, out / script.name)
    shutil.copyfile(__file__, out / Path(__file__).name)

    rom = args.rom.read_bytes()
    bank = 0x86
    bank_base = (bank & 0x7F) * 0x8000
    binary = out / "bank86.bin"
    binary.write_bytes(rom[bank_base:bank_base + 0x8000])
    sys.path.insert(0, str(args.recompiler.resolve()))
    from v2.emit_bank import BankEntry, emit_bank
    generated = emit_bank(rom, bank, [
        BankEntry("CpuModeFive", FIRST, END, 0, 0)
    ])
    (out / "cpu_mode_five_bank86.c").write_text(generated, encoding="utf-8")

    env = os.environ.copy()
    env["JAVA_HOME"] = str(args.jdk.resolve())
    project = out / "project"
    project.mkdir()
    command = [str(args.ghidra), str(project), "CpuModeFive", "-import",
               str(binary), "-processor", "65816:LE:16:default", "-loader",
               "BinaryLoader", "-loader-baseAddr", "0x8000", "-noanalysis",
               "-scriptPath", str(out), "-postScript", script.name, str(out)]
    with (out / "ghidra.log").open("w", encoding="utf-8") as log:
        run = subprocess.run(
            command, env=env, stdout=log, stderr=subprocess.STDOUT, timeout=180,
            creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
    listing = out / "cpu_mode_five_bank86.txt"
    if run.returncode or not listing.is_file():
        raise RuntimeError("Ghidra failed; see retained ghidra.log")
    text = listing.read_text(encoding="utf-8")
    for pc in REQUIRED:
        if f"$86:{pc:04X} [" not in text:
            raise RuntimeError(f"Ghidra missed $86:{pc:04X}")

    source = rom[bank_base + FIRST - 0x8000:bank_base + END - 0x8000]
    manifest = {
        "schema": 1,
        "rom_sha256": ROM_SHA256,
        "command": {"args": command, "exit_code": run.returncode},
        "range": {
            "start": f"86:{FIRST:04X}",
            "end_exclusive": f"86:{END:04X}",
            "bytes": source.hex(),
            "sha256": hashlib.sha256(source).hexdigest(),
        },
        "recompiler_sources": {
            str(path): sha(path) for path in args.recompiler.rglob("*.py")
        },
    }
    manifest["artifacts"] = {
        path.name: {"bytes": path.stat().st_size, "sha256": sha(path)}
        for path in out.iterdir()
        if path.is_file() and path.name != "manifest.json"
    }
    (out / "manifest.json").write_text(
        json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print("bank 86: fresh Ghidra and recomp CPU mode-five references")


if __name__ == "__main__":
    main()
