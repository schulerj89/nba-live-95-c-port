"""Regenerate bounded ROM semantics; reference only, never the test oracle."""
import argparse
import sys
from pathlib import Path


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--recompiler', required=True)
    parser.add_argument('--rom', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    sys.path.insert(0, str(Path(args.recompiler).resolve()))
    from v2.emit_bank import BankEntry, emit_bank
    output = Path(args.output)
    if output.exists():
        parser.error('reference output must be new')
    output.parent.mkdir(parents=True, exist_ok=True)
    rom = Path(args.rom).read_bytes()
    if len(rom) % 0x8000 == 512:
        rom = rom[512:]
    output.write_text(emit_bank(rom, 0x86, [
        BankEntry('InboundSideGate', 0xF61F, 0xF654, 0, 0)]), encoding='utf-8')
    print(output)


if __name__ == '__main__':
    main()
