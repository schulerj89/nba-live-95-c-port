"""Fresh bounded recompiler reference, never the native expected-output oracle."""
import argparse
from pathlib import Path
import sys


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--recompiler", required=True)
    parser.add_argument("--rom", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    sys.path.insert(0, str(Path(args.recompiler).resolve()))
    from v2.emit_bank import BankEntry, emit_bank
    rom = Path(args.rom).read_bytes()
    if len(rom) % 0x8000 == 512:
        rom = rom[512:]
    output = Path(args.output)
    if output.exists():
        parser.error("reference output must be new")
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(emit_bank(rom, 0x85, [
        BankEntry("FormationRoute", 0xAD6B, 0xAF5C, 0, 0),
    ]), encoding="utf-8")
    print(output)


if __name__ == "__main__":
    main()
