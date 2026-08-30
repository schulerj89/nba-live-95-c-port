"""Emit bounded human free-throw reference C from the user's ROM.

Reference output only: never linked into the portable port or treated as the
behavioral oracle.  It is a readable instruction-semantics cross-check beside
the Mesen vectors and focused Ghidra listing.
"""

import argparse
import sys
from pathlib import Path


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--recompiler", required=True)
    parser.add_argument("--rom", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    sys.path.insert(0, str(Path(args.recompiler).resolve()))
    from v2.emit_bank import BankEntry, emit_bank

    entries = [
        BankEntry("HumanFreeThrowScene", 0x9CBF, 0xA018, 0, 0),
        BankEntry("HumanAimOscillator", 0xA018, 0xA046, 0, 0),
    ]
    output = Path(args.output)
    if output.exists():
        parser.error("reference output must be new")
    output.parent.mkdir(parents=True, exist_ok=True)
    rom = Path(args.rom).read_bytes()
    if len(rom) % 0x8000 == 512:
        rom = rom[512:]
    output.write_text(
        emit_bank(rom, 0x87, entries),
        encoding="utf-8")
    print(output)


if __name__ == "__main__":
    main()
