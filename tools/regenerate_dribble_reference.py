"""Regenerate Ghidra and snesrecomp references for owned-ball dribbling."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys

ROM_SHA256 = "2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870"
RANGES = {
    0x80: [("DrawBallOwner", 0xAF1E, 0xB120),
           ("SpriteDescriptorSubmit", 0xB344, 0xB499),
           ("FullDrawSort", 0xFBFF, 0xFC80),
           ("CarriedDrawPass", 0xFC80, 0xFCA2)],
    0x85: [("OwnedBallDispatch", 0x9A24, 0x9A78),
           ("OwnedBallSubstep", 0xA4F2, 0xA5F4),
           ("OwnedBallTail", 0xA7A1, 0xA7C8)],
    0x86: [("ReverseDribblePose", 0xE545, 0xE593)],
    0x87: [("ProjectSortActors", 0xA357, 0xA4E1),
           ("OwnerDrawArguments", 0xA61E, 0xA6A9),
           ("AnimationCadence", 0xAD5B, 0xAEC3),
           ("AttachBall", 0xB649, 0xB67C),
           ("BallOffsetXY", 0xB832, 0xB953),
           ("BallOffsetZ", 0xB953, 0xB996)],
}
REQUIRED_INSTRUCTIONS = {
    0x80: (0xAF1E, 0xAFEE, 0xB0A8, 0xB0FF, 0xB10C, 0xB11B,
           0xB344, 0xB348, 0xFBFF, 0xFC69, 0xFC7F, 0xFC80, 0xFCA1),
    0x85: (0x9A37, 0xA4F2, 0xA50D, 0xA510, 0xA513, 0xA7C7),
    0x86: (0xE545,),
    0x87: (0xA357, 0xA3B6, 0xA472, 0xA4D5, 0xA65E, 0xA671,
           0xAD5B, 0xAE89, 0xB649, 0xB66A, 0xB832, 0xB953),
}


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
    script = Path(__file__).with_name("ghidra") / "DumpDribble.java"
    shutil.copyfile(script, out / script.name)
    shutil.copyfile(__file__, out / Path(__file__).name)
    sys.path.insert(0, str(args.recompiler.resolve()))
    from v2.emit_bank import BankEntry, emit_bank

    env = os.environ.copy()
    env["JAVA_HOME"] = str(args.jdk.resolve())
    project = out / "project"
    project.mkdir()
    ghidra_root = args.ghidra.resolve().parent.parent
    sources = [*args.recompiler.resolve().rglob("*.py"), args.ghidra.resolve(),
               ghidra_root / "Ghidra/application.properties",
               *[p for p in (ghidra_root / "Ghidra/Processors/65816").rglob("*")
                 if p.is_file()]]
    manifest = {"rom_sha256": ROM_SHA256, "commands": [], "ranges": {},
                "tool_sources": {str(p): sha(p) for p in sources}}
    def offset(address):
        return ((address >> 16) & 0x7f) * 0x8000 + (address & 0x7fff)
    pointer = 0x898000 + (struct.unpack_from("<I", rom, offset(0x898000) + 0x81d * 4)[0] & 0xffffff)
    data = rom[offset(pointer):offset(pointer) + 49]
    manifest["ball_descriptor"] = {"resource": "081D", "address": f"{pointer:06X}",
        "bytes": data.hex(), "part_x": struct.unpack_from("<h", data, 10)[0],
        "part_y": struct.unpack_from("<h", data, 12)[0],
        "sha256": hashlib.sha256(data).hexdigest()}
    for bank, ranges in RANGES.items():
        base = (bank & 0x7f) * 0x8000
        binary = out / f"bank{bank:02x}.bin"
        binary.write_bytes(rom[base:base + 0x8000])
        generated = emit_bank(rom, bank, [
            BankEntry(name, start, end, 0, 0) for name, start, end in ranges])
        (out / f"dribble_bank{bank:02x}.c").write_text(generated, encoding="utf-8")
        for name, start, end in ranges:
            data = rom[base + start - 0x8000:base + end - 0x8000]
            manifest["ranges"][name] = {
                "start": f"{bank:02X}:{start:04X}",
                "end_exclusive": f"{bank:02X}:{end:04X}",
                "bytes": data.hex(), "sha256": hashlib.sha256(data).hexdigest()}
        command = [str(args.ghidra), str(project), "Dribble", "-import", str(binary),
                   "-processor", "65816:LE:16:default", "-loader", "BinaryLoader",
                   "-loader-baseAddr", "0x8000", "-noanalysis", "-scriptPath", str(out),
                   "-postScript", script.name, str(out), f"{bank:02x}"]
        with (out / f"ghidra{bank:02x}.log").open("w", encoding="utf-8") as log:
            run = subprocess.run(command, env=env, stdout=log, stderr=subprocess.STDOUT,
                                 timeout=180, creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
        manifest["commands"].append({"args": command, "exit_code": run.returncode})
        (out / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
        listing = out / f"dribble_bank{bank:02x}.txt"
        if run.returncode or not listing.is_file() or not listing.stat().st_size:
            raise RuntimeError(f"Ghidra bank {bank:02x} failed; see retained log")
        text = listing.read_text(encoding="utf-8")
        for pc in REQUIRED_INSTRUCTIONS[bank]:
            if f"${bank:02X}:{pc:04X} [" not in text:
                raise RuntimeError(f"Ghidra missed required instruction ${bank:02X}:{pc:04X}")
        print(f"bank {bank:02X}: fresh Ghidra and recomp references", flush=True)
    manifest["artifacts"] = {p.name: {"bytes": p.stat().st_size, "sha256": sha(p)}
                             for p in out.iterdir() if p.is_file() and p.name != "manifest.json"}
    (out / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")


if __name__ == "__main__":
    main()
