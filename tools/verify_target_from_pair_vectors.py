"""Replay `$86:E923-$E96E` paired-target calls through compiled C."""

import argparse, json, subprocess
from pathlib import Path


def memory(snapshot):
    return {int(base, 16) + i: value for base, payload in snapshot.items()
            for i, value in enumerate(bytes.fromhex(payload))}


def word(mem, address): return mem[address] | (mem[address + 1] << 8)


def rom_word(rom, bank, address):
    offset = ((bank & 0x7F) * 0x8000) + (address & 0x7FFF)
    return rom[offset] | (rom[offset + 1] << 8)


def main():
    parser = argparse.ArgumentParser(); parser.add_argument("--vectors", required=True); parser.add_argument("--probe", required=True); parser.add_argument("--rom", required=True); args = parser.parse_args()
    rom = Path(args.rom).read_bytes(); vectors = [json.loads(line) for line in Path(args.vectors).read_text().splitlines()]
    inputs, expected = [], []
    for vector in vectors:
        entry, exit_mem = memory(vector["entry"]["mem"]), memory(vector["exit"]["mem"])
        paired, subject = vector["entry"]["cpu"]["y"] & 0xFFFF, word(entry, 0x0096); index = word(entry, 0x00B2); table = word(entry, 0x008E); bank = 0x86
        inputs.append([word(entry, paired + 4), word(entry, paired + 8), word(entry, paired + 0x0E), word(entry, paired + 0x10), rom_word(rom, bank, (table + index) & 0xFFFF), rom_word(rom, bank, (table + index + 8) & 0xFFFF)])
        expected.append([word(exit_mem, subject + 0x56), word(exit_mem, subject + 0x58)])
    stdin = "\n".join(" ".join(f"{v:04x}" for v in row) for row in inputs) + "\n"
    result = subprocess.run([args.probe], input=stdin, capture_output=True, text=True, check=True)
    produced = [[int(v, 16) for v in line.split()] for line in result.stdout.splitlines()]
    mismatches = [(i + 1, inputs[i], expected[i], produced[i]) for i in range(min(len(expected), len(produced))) if expected[i] != produced[i]]
    print(f"[TARGET FROM PAIR] {'PASS' if not mismatches else 'FAIL'}: vectors={len(vectors)} mismatches={len(mismatches)}")
    for row in mismatches[:10]: print("  call=%d input=%s rom=%s port=%s" % row)
    if len(produced) != len(expected) or mismatches: raise SystemExit(1)


if __name__ == "__main__": main()
