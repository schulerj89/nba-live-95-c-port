"""Rebuild the Game Setup screen's VRAM/CGRAM from the ROM alone.

Replays the exact sequence the ROM performs when entering Game Setup:
  1. call the ROM's own decompressor ($80:C62B) for each traced source pointer,
  2. apply the DMA transfers it then issues into VRAM ($2118/$2119) and
     CGRAM ($2122).

The traced sequence lives in .analysis/setup_capture/decomp_trace.txt, captured
live from Mesen, so every pointer here came from the running ROM rather than a
guess. Verify against the captured vram.bin/cgram.bin with --verify.
"""
import argparse
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from extract_assets import Snes65816Decompressor

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
CAP = os.path.join(ROOT, ".analysis", "setup_capture")

DECOMP_RE = re.compile(r"frame=(\d+) DECOMP src=(\w\w):(\w{4}) dst=(\w\w):(\w{4})")
DMA_RE = re.compile(
    r"frame=(\d+) DMA ch=(\d+) param=(\w\w) dest=21(\w\w) src=(\w\w):(\w{4}) size=(\w{4}) vram=(\w{4}) cg=(\w\w)")


def parse_trace(path, lo, hi):
    """Return the ordered decompress/DMA event list inside a frame window."""
    events = []
    for line in open(path, encoding="utf-8", errors="replace"):
        m = DECOMP_RE.match(line)
        if m:
            f = int(m.group(1))
            if lo <= f <= hi:
                events.append(("decomp", f, int(m.group(2), 16), int(m.group(3), 16),
                               int(m.group(4), 16), int(m.group(5), 16)))
            continue
        m = DMA_RE.match(line)
        if m:
            f = int(m.group(1))
            if lo <= f <= hi:
                events.append(("dma", f, int(m.group(3), 16), int(m.group(4), 16),
                               int(m.group(5), 16), int(m.group(6), 16),
                               int(m.group(7), 16), int(m.group(8), 16), int(m.group(9), 16)))
    return events


def run(lo, hi, verbose=False):
    rom = open(ROM_PATH, "rb").read()
    emu = Snes65816Decompressor(rom)
    vram = bytearray(0x10000)
    cgram = bytearray(0x200)
    oam = bytearray(0x220)
    cg_addr = 0

    def src_byte(bank, addr, i, fixed):
        a = addr if fixed else (addr + i) & 0xFFFF
        return emu.read8(bank, a)

    for ev in parse_trace(os.path.join(CAP, "decomp_trace.txt"), lo, hi):
        if ev[0] == "decomp":
            _, f, sb, sa, db, da = ev
            emu.decompress(sb, sa, db, da)
            if verbose:
                print(f"  f{f} decompress ${sb:02X}:{sa:04X} -> ${db:02X}:{da:04X}")
        else:
            _, f, param, dest, sbank, saddr, size, vaddr, cgstart = ev
            size = size if size else 0x10000
            fixed = bool(param & 0x08)
            if dest in (0x18, 0x19):
                # VRAM: mode 1 writes $2118/$2119 alternately, one word per step
                addr = vaddr
                if dest == 0x18 and (param & 0x07) == 1:
                    for i in range(0, size, 2):
                        lo_b = src_byte(sbank, saddr, i, fixed)
                        hi_b = src_byte(sbank, saddr, i + 1, fixed)
                        vram[(addr * 2) & 0xFFFF] = lo_b
                        vram[(addr * 2 + 1) & 0xFFFF] = hi_b
                        addr = (addr + 1) & 0x7FFF
                else:
                    # single-register transfers ($2118 low bytes, or $2119 high)
                    off = 1 if dest == 0x19 else 0
                    for i in range(size):
                        vram[(addr * 2 + off) & 0xFFFF] = src_byte(sbank, saddr, i, fixed)
                        addr = (addr + 1) & 0x7FFF
            elif dest == 0x22:
                cg_addr = cgstart * 2
                for i in range(size):
                    cgram[cg_addr & 0x1FF] = src_byte(sbank, saddr, i, fixed)
                    cg_addr += 1
            elif dest == 0x04:
                for i in range(size):
                    oam[i % len(oam)] = src_byte(sbank, saddr, i, fixed)
    return vram, cgram, oam


def verify(vram, cgram):
    ref_v = open(os.path.join(CAP, "vram.bin"), "rb").read()
    ref_c = open(os.path.join(CAP, "cgram.bin"), "rb").read()
    for name, got, ref in (("VRAM", vram, ref_v), ("CGRAM", cgram, ref_c)):
        same = sum(1 for a, b in zip(got, ref) if a == b)
        print(f"{name}: {same}/{len(ref)} bytes match ({100.0*same/len(ref):.2f}%)")
        if same != len(ref):
            diffs = [i for i, (a, b) in enumerate(zip(got, ref)) if a != b]
            runs = []
            for i in diffs:
                if runs and i == runs[-1][1] + 1:
                    runs[-1][1] = i
                else:
                    runs.append([i, i])
            print(f"  {len(runs)} differing run(s); first 12:")
            for a, b in runs[:12]:
                print(f"    {a:#06x}-{b:#06x} ({b-a+1} bytes)")


if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("--rom", required=True)
    ap.add_argument("--from-frame", type=int, default=1700)
    ap.add_argument("--to-frame", type=int, default=1790)
    ap.add_argument("--verify", action="store_true")
    ap.add_argument("--out", default=None)
    ap.add_argument("-v", "--verbose", action="store_true")
    args = ap.parse_args()
    ROM_PATH = args.rom
    vram, cgram, oam = run(args.from_frame, args.to_frame, args.verbose)
    if args.verify:
        verify(vram, cgram)
    if args.out:
        os.makedirs(args.out, exist_ok=True)
        open(os.path.join(args.out, "setup_vram.bin"), "wb").write(vram)
        open(os.path.join(args.out, "setup_cgram.bin"), "wb").write(cgram)
        print("wrote", args.out)
