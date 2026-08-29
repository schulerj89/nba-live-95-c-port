"""Generate bounded reference C with the user's snesrecomp source package.

Reference output only: never compiled into the C port or treated as an oracle.
"""
import argparse
import sys
from pathlib import Path


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--recompiler', required=True, help='snesrecomp/recompiler source directory')
    parser.add_argument('--rom', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    sys.path.insert(0, str(Path(args.recompiler).resolve()))
    from v2.emit_bank import BankEntry, emit_bank
    entries = [BankEntry(name, first, last + 1, 0, 0) for name, first, last in (
        ('BallInitializationPrefix', 0xe056, 0xe0ab),
        ('JumpReach', 0xec32, 0xee75),
        ('DefensiveIdle', 0xe39a, 0xe3ca),
        ('DefensivePose', 0xe3e1, 0xe4a6),
        ('InboundContinuation', 0xf43a, 0xf668),
        ('TimeoutConfirmation', 0x844e, 0x8467),
    )]
    output = Path(args.output)
    if output.exists():
        parser.error('reference output must be new')
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(emit_bank(Path(args.rom).read_bytes(), 0x86, entries), encoding='utf-8')
    print(output)


if __name__ == '__main__':
    main()
