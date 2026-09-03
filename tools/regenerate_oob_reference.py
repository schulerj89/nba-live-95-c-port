"""Generate fresh Ghidra and snesrecomp references for out-of-bounds HUD children."""
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
    0x81: [("TextAndCanvas", 0x9F54, 0xA242)],
    0x83: [("HudDispatch", 0xCC10, 0xCC7B),
           ("ViolationLayout", 0xDA12, 0xDA8C),
           ("ViolationText", 0xDA8C, 0xDB9D),
           ("HudRetirement", 0xEBDB, 0xED47)],
    0x85: [("WhistleConsume", 0x93F5, 0x945F)],
    0x87: [("BoundsDetector", 0x92ED, 0x93DD)],
}
REQUIRED = {0x81: (0x9F54, 0x9FDF, 0xA05F, 0xA1E7),
            0x83: (0xCC10, 0xDA12, 0xDA8C, 0xDB29, 0xDB6D, 0xDB95, 0xEC60, 0xECA4, 0xED46),
            0x85: (0x93F5, 0x945E), 0x87: (0x93C4, 0x93D2, 0x93D5)}


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("rom", "output", "recompiler", "ghidra", "jdk"):
        parser.add_argument("--" + name, type=Path, required=True)
    args = parser.parse_args()
    if sha(args.rom) != ROM_SHA256:
        raise ValueError("expected the verified USA ROM")
    rom = args.rom.read_bytes()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    script = Path(__file__).with_name("ghidra") / "DumpOutOfBounds.java"
    shutil.copyfile(script, out / script.name)
    shutil.copyfile(__file__, out / Path(__file__).name)
    sys.path.insert(0, str(args.recompiler.resolve()))
    from v2.emit_bank import BankEntry, emit_bank
    env = os.environ.copy()
    env["JAVA_HOME"] = str(args.jdk.resolve())
    project = out / "project"
    project.mkdir()
    manifest = {"rom_sha256": ROM_SHA256, "commands": [], "ranges": {},
                "recompiler_sources": {str(p): sha(p) for p in args.recompiler.rglob("*.py")}}
    for bank, ranges in RANGES.items():
        base = (bank & 0x7f) * 0x8000
        binary = out / f"bank{bank:02x}.bin"
        binary.write_bytes(rom[base:base + 0x8000])
        generated = emit_bank(rom, bank, [BankEntry(name, start, end, 0, 0)
                                         for name, start, end in ranges])
        (out / f"oob_bank{bank:02x}.c").write_text(generated, encoding="utf-8")
        for name, start, end in ranges:
            data = rom[base + start - 0x8000:base + end - 0x8000]
            manifest["ranges"][name] = {"start": f"{bank:02X}:{start:04X}",
                "end_exclusive": f"{bank:02X}:{end:04X}",
                "sha256": hashlib.sha256(data).hexdigest()}
        command = [str(args.ghidra), str(project), "OutOfBounds", "-import", str(binary),
                   "-processor", "65816:LE:16:default", "-loader", "BinaryLoader",
                   "-loader-baseAddr", "0x8000", "-noanalysis", "-scriptPath", str(out),
                   "-postScript", script.name, str(out), f"{bank:02x}"]
        with (out / f"ghidra{bank:02x}.log").open("w", encoding="utf-8") as log:
            run = subprocess.run(command, env=env, stdout=log, stderr=subprocess.STDOUT,
                                 timeout=180, creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
        manifest["commands"].append({"args": command, "exit_code": run.returncode})
        (out / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
        listing = out / f"oob_bank{bank:02x}.txt"
        if run.returncode or not listing.is_file():
            raise RuntimeError(f"Ghidra bank {bank:02x} failed; see retained log")
        text = listing.read_text(encoding="utf-8")
        for pc in REQUIRED[bank]:
            if f"${bank:02X}:{pc:04X} [" not in text:
                raise RuntimeError(f"Ghidra missed ${bank:02X}:{pc:04X}")
        print(f"bank {bank:02X}: fresh Ghidra and recomp references", flush=True)
    manifest["artifacts"] = {p.name: {"bytes": p.stat().st_size, "sha256": sha(p)}
                             for p in out.iterdir() if p.is_file() and p.name != "manifest.json"}
    (out / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")


if __name__ == "__main__":
    main()
