"""Regenerate bounded court-loader references with Ghidra and snesrecomp."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROM_SHA256 = "2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870"
RANGES = {
    0x84: [("LoadCourtResources", 0xE4D8, 0xE5B3)],
    0x80: [("UploadVram", 0x8BA1, 0x8BD0), ("QueueVram", 0x8BD0, 0x8C2B)],
    0x85: [("SelectCourtLayout", 0x8BBF, 0x8C4F),
           ("StreamCourtMap", 0x8EE6, 0x90C4), ("InitCourtMap", 0x90C4, 0x917A)],
}


def digest(data):
    return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("rom", "output", "recompiler", "ghidra", "jdk"):
        parser.add_argument("--" + name, type=Path, required=True)
    args = parser.parse_args()
    rom = args.rom.read_bytes()
    if digest(rom) != ROM_SHA256:
        raise ValueError("expected the verified USA ROM")
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    script = Path(__file__).with_name("ghidra") / "DumpCourtLogo.java"
    shutil.copyfile(script, out / script.name)
    shutil.copyfile(__file__, out / Path(__file__).name)
    sys.path.insert(0, str(args.recompiler.resolve()))
    from v2.emit_bank import BankEntry, emit_bank

    env = os.environ.copy()
    env["JAVA_HOME"] = str(args.jdk.resolve())
    project = out / "project"
    project.mkdir()
    manifest = {"rom_sha256": ROM_SHA256, "commands": [], "ranges": {},
                "tool_sources": {}}
    ghidra_root = args.ghidra.resolve().parent.parent
    sources = [*args.recompiler.resolve().rglob("*.py"), args.ghidra.resolve(),
               ghidra_root / "Ghidra/application.properties",
               *[p for p in (ghidra_root / "Ghidra/Processors/65816").rglob("*")
                 if p.is_file()]]
    manifest["tool_sources"] = {str(p): digest(p.read_bytes()) for p in sources}
    for bank, ranges in RANGES.items():
        binary = out / f"bank{bank:02x}.bin"
        bank_start = (bank & 0x7f) * 0x8000
        binary.write_bytes(rom[bank_start:bank_start + 0x8000])
        generated = emit_bank(rom, bank, [
            BankEntry(name, start, end, 0, 0) for name, start, end in ranges])
        (out / f"court_bank{bank:02x}.c").write_text(generated, encoding="utf-8")
        for name, start, end in ranges:
            data = rom[bank_start + start - 0x8000:bank_start + end - 0x8000]
            manifest["ranges"][name] = {
                "start": f"{bank:02X}:{start:04X}",
                "end_exclusive": f"{bank:02X}:{end:04X}",
                "bytes": data.hex(), "sha256": digest(data)}
        command = [
            str(args.ghidra), str(project), "CourtLogo", "-import", str(binary),
            "-processor", "65816:LE:16:default", "-loader", "BinaryLoader",
            "-loader-baseAddr", "0x8000", "-noanalysis", "-scriptPath", str(out),
            "-postScript", script.name, str(out), f"{bank:02x}"]
        with (out / f"ghidra{bank:02x}.log").open("w", encoding="utf-8") as log:
            run = subprocess.run(command, env=env, stdout=log,
                                 stderr=subprocess.STDOUT, timeout=180,
                                 creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
        manifest["commands"].append({"args": command, "exit_code": run.returncode})
        (out / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
        listing = out / f"court_bank{bank:02x}.txt"
        if run.returncode or not listing.is_file() or not listing.stat().st_size:
            raise RuntimeError(f"Ghidra bank {bank:02x} failed; see retained log")
        print(f"bank {bank:02X}: fresh Ghidra and recomp references", flush=True)
    manifest["artifacts"] = {
        p.name: {"bytes": p.stat().st_size, "sha256": digest(p.read_bytes())}
        for p in out.iterdir() if p.is_file() and p.name != "manifest.json"}
    (out / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")


if __name__ == "__main__":
    main()
