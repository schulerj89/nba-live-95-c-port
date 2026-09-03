"""Generate fresh Ghidra and snesrecomp references for CPU reaction reloads."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys


ROM_SHA256 = "2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870"
RANGES = [
    ("ReactionReload", 0xB95C, 0xB9D2),
    ("RoleRebuild", 0xBD0D, 0xBE06),
]
REQUIRED = (0xB95C, 0xB95E, 0xB969, 0xB971, 0xB9B2, 0xB9CE,
            0xBD0D, 0xBD39, 0xBD96, 0xBDF3, 0xBE03)


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
    script = Path(__file__).with_name("ghidra") / "DumpCpuReaction.java"
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
        BankEntry(name, start, end, 0, 0) for name, start, end in RANGES
    ])
    (out / "cpu_reaction_bank85.c").write_text(generated, encoding="utf-8")

    env = os.environ.copy()
    env["JAVA_HOME"] = str(args.jdk.resolve())
    project = out / "project"
    project.mkdir()
    command = [str(args.ghidra), str(project), "CpuReaction", "-import",
               str(binary), "-processor", "65816:LE:16:default", "-loader",
               "BinaryLoader", "-loader-baseAddr", "0x8000", "-noanalysis",
               "-scriptPath", str(out), "-postScript", script.name, str(out)]
    with (out / "ghidra.log").open("w", encoding="utf-8") as log:
        run = subprocess.run(command, env=env, stdout=log,
                             stderr=subprocess.STDOUT, timeout=180,
                             creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
    listing = out / "cpu_reaction_bank85.txt"
    if run.returncode or not listing.is_file():
        raise RuntimeError("Ghidra failed; see retained ghidra.log")
    text = listing.read_text(encoding="utf-8")
    for pc in REQUIRED:
        if f"$85:{pc:04X} [" not in text:
            raise RuntimeError(f"Ghidra missed $85:{pc:04X}")

    manifest = {
        "schema": 1,
        "rom_sha256": ROM_SHA256,
        "command": {"args": command, "exit_code": run.returncode},
        "ranges": {
            name: {
                "start": f"85:{start:04X}",
                "end_exclusive": f"85:{end:04X}",
                "sha256": hashlib.sha256(
                    rom[bank_base + start - 0x8000:
                        bank_base + end - 0x8000]).hexdigest(),
            }
            for name, start, end in RANGES
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
    print("bank 85: fresh Ghidra and recomp CPU reaction references")


if __name__ == "__main__":
    main()
