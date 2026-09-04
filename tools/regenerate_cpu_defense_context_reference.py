"""Generate fresh Ghidra and snesrecomp references for CPU defense selection."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys


ROM_SHA256 = "2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870"
FIRST = 0xB128
END = 0xB177
REQUIRED = (0xB128, 0xB130, 0xB13F, 0xB147, 0xB14F, 0xB157,
            0xB159, 0xB161, 0xB166, 0xB16B, 0xB170, 0xB173, 0xB176)


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
    script = Path(__file__).with_name("ghidra") / "DumpCpuDefenseContext.java"
    shutil.copyfile(script, out / script.name)
    shutil.copyfile(__file__, out / Path(__file__).name)

    rom = args.rom.read_bytes()
    bank = 0x85
    bank_base = (bank & 0x7F) * 0x8000
    binary = out / "bank85.bin"
    binary.write_bytes(rom[bank_base:bank_base + 0x8000])
    sys.path.insert(0, str(args.recompiler.resolve()))
    from v2.emit_bank import BankEntry, emit_bank
    generated = emit_bank(rom, bank, [
        BankEntry("CpuDefenseContext", FIRST, END, 0, 0)
    ])
    (out / "cpu_defense_context_bank85.c").write_text(
        generated, encoding="utf-8")

    env = os.environ.copy()
    env["JAVA_HOME"] = str(args.jdk.resolve())
    project = out / "project"
    project.mkdir()
    command = [str(args.ghidra), str(project), "CpuDefenseContext", "-import",
               str(binary), "-processor", "65816:LE:16:default", "-loader",
               "BinaryLoader", "-loader-baseAddr", "0x8000", "-noanalysis",
               "-scriptPath", str(out), "-postScript", script.name, str(out)]
    with (out / "ghidra.log").open("w", encoding="utf-8") as log:
        run = subprocess.run(
            command, env=env, stdout=log, stderr=subprocess.STDOUT, timeout=180,
            creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
    listing = out / "cpu_defense_context_bank85.txt"
    if run.returncode or not listing.is_file():
        raise RuntimeError("Ghidra failed; see retained ghidra.log")
    text = listing.read_text(encoding="utf-8")
    for pc in REQUIRED:
        if f"$85:{pc:04X} [" not in text:
            raise RuntimeError(f"Ghidra missed $85:{pc:04X}")

    source = rom[bank_base + FIRST - 0x8000:bank_base + END - 0x8000]
    manifest = {
        "schema": 1,
        "rom_sha256": ROM_SHA256,
        "command": {"args": command, "exit_code": run.returncode},
        "range": {
            "start": f"85:{FIRST:04X}",
            "end_exclusive": f"85:{END:04X}",
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
    print("bank 85: fresh Ghidra and recomp CPU defense-context references")


if __name__ == "__main__":
    main()
