"""Emit bounded Setup configuration reference C; never a behavioral oracle."""
import argparse
from pathlib import Path
import sys


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--recompiler',required=True)
    parser.add_argument('--rom',required=True)
    parser.add_argument('--output',required=True)
    parser.add_argument('--bank',choices=('81','82'),default='81')
    args=parser.parse_args()
    sys.path.insert(0,str(Path(args.recompiler).resolve()))
    from v2.emit_bank import BankEntry,emit_bank
    output=Path(args.output)
    if output.exists():parser.error('reference output must be new')
    output.parent.mkdir(parents=True,exist_ok=True)
    rom=Path(args.rom).read_bytes()
    if len(rom)%0x8000==512:rom=rom[512:]
    entries=[BankEntry(name,start,end,0,0) for name,start,end in (
        ('MenuInputRepeat',0xab58,0xac04),
        ('ApplySelectedStyle',0xbfaa,0xc00b),
        ('InitializeOrLoadConfiguration',0xc19a,0xc398),
        ('LoadCustomRules',0xc398,0xc3d5),
        ('SaveCustomRules',0xc3d5,0xc41e),
        ('AdjustAndCommitRules',0xd446,0xd53d),
    )]
    if args.bank=='82':entries=[BankEntry('OptionsInput',0x8cd1,0x8f9c,0,0)]
    output.write_text(emit_bank(rom,int(args.bank,16),entries),encoding='utf-8')
    print(output)


if __name__=='__main__':main()
