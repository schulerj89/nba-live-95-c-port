"""Prepare and summarize the conservative full-ROM Ghidra census.

This deliberately avoids a linear sweep. Seeds come from native execution,
verified routine entries, source provenance, recomp functions, and vectors.
Ghidra follows reachable control flow; undecoded ROM remains unknown data/code.
"""

import argparse
import bisect
import json
import re
import shutil
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CANONICAL_BANKS = range(0x80, 0xB0)  # 1.5 MiB / 48 LoROM banks
PROVENANCE = re.compile(r"\$([0-9A-Fa-f]{2}):([0-9A-Fa-f]{4})")


def canonical(address):
    bank = (address >> 16) & 0xFF
    if bank < 0x80:
        bank |= 0x80
    return (bank << 16) | (address & 0xFFFF)


def merge(intervals):
    result = []
    for start, end in sorted(intervals):
        start, end = canonical(start), canonical(end)
        if (start >> 16) != (end >> 16):
            continue
        if result and start <= result[-1][1] + 1:
            result[-1][1] = max(result[-1][1], end)
        else:
            result.append([start, end])
    return [(a, b) for a, b in result]


def contains(intervals, address):
    # Intervals are merged and sorted. Reports perform this lookup millions of
    # times, so avoid a linear scan for every decoded instruction.
    index = bisect.bisect_right(intervals, (address, 0xFFFFFF)) - 1
    return index >= 0 and intervals[index][0] <= address <= intervals[index][1]


def load_exec():
    rows = []
    for path in (ROOT / ".analysis").rglob("exec_*.txt"):
        for line in path.read_text(errors="replace").splitlines():
            match = re.fullmatch(r"([0-9A-Fa-f]{6})-([0-9A-Fa-f]{6})", line.strip())
            if match:
                rows.append(tuple(int(value, 16) for value in match.groups()))
    return merge(rows)


def load_verified():
    data = json.loads((ROOT / "docs/verified-routines.json").read_text())
    return merge(tuple(int(value, 16) for value in row["range"].split("-"))
                 for row in data["routines"])


def provenance_seeds():
    result = set()
    for path in (ROOT / "src").glob("*.c"):
        for bank, offset in PROVENANCE.findall(path.read_text(errors="replace")):
            address = canonical((int(bank, 16) << 16) | int(offset, 16))
            # Port comments also cite graphics/audio ROM locations. Only the
            # eight banks witnessed executing natively are eligible as source
            # provenance seeds; direct calls can discover code elsewhere.
            if 0x80 <= address >> 16 <= 0x87:
                result.add(address)
    return result


def recomp_seeds(path):
    result = set()
    if not path.exists():
        return result
    pattern = re.compile(r"\bbank_([0-9A-Fa-f]{2})_([0-9A-Fa-f]{4})\s*\(")
    for source in path.glob("generated/bank*.c"):
        for bank, offset in pattern.findall(source.read_text(errors="replace")):
            result.add(canonical((int(bank, 16) << 16) | int(offset, 16)))
    return result


def prepare(args):
    rom = Path(args.rom).read_bytes()
    if len(rom) == 0 or len(rom) % 0x8000:
        raise SystemExit("Expected an unheadered whole-number LoROM image")
    banks = len(rom) // 0x8000
    if banks != 48:
        raise SystemExit(f"Expected 48 banks for the US ROM, found {banks}")
    out = Path(args.out)
    # A fresh census must not inherit listings/calls from an older seed policy.
    for child in ("banks", "seeds", "listings", "calls"):
        target = out / child
        if target.exists():
            shutil.rmtree(target)
    (out / "banks").mkdir(parents=True, exist_ok=True)
    (out / "seeds").mkdir(parents=True, exist_ok=True)
    for index, bank in enumerate(CANONICAL_BANKS):
        (out / "banks" / f"bank_{bank:02X}.bin").write_bytes(
            rom[index * 0x8000:(index + 1) * 0x8000])

    seeds = provenance_seeds() | recomp_seeds(Path(args.recomp))
    executed = load_exec()
    verified = load_verified()
    seeds.update(start for start, _ in executed)
    seeds.update(start for start, _ in verified)

    # Native reset/NMI/IRQ vectors in physical bank 0. Vector values are local
    # offsets and therefore enter canonical bank $80.
    for offset in (0x7FE4, 0x7FE6, 0x7FEE, 0x7FF4, 0x7FF6, 0x7FFE, 0x7FFC):
        target = struct.unpack_from("<H", rom, offset)[0]
        if target >= 0x8000:
            seeds.add(0x800000 | target)

    for bank in CANONICAL_BANKS:
        values = sorted(address for address in seeds
                        if address >> 16 == bank and address & 0xFFFF >= 0x8000)
        (out / "seeds" / f"bank_{bank:02X}.txt").write_text(
            "".join(f"{address:06X}\n" for address in values), encoding="ascii")
    print(f"[CENSUS] prepared {banks} banks and {len(seeds)} initial seeds")


def merge_calls(args):
    out = Path(args.out)
    added = 0
    calls = out / "calls"
    for path in calls.glob("bank_*_calls.txt"):
        for line in path.read_text(errors="replace").splitlines():
            if not re.fullmatch(r"[0-9A-Fa-f]{6}", line.strip()):
                continue
            address = canonical(int(line, 16))
            bank, offset = address >> 16, address & 0xFFFF
            if bank not in CANONICAL_BANKS or offset < 0x8000:
                continue
            seed_path = out / "seeds" / f"bank_{bank:02X}.txt"
            current = set(seed_path.read_text().splitlines())
            value = f"{address:06X}"
            if value not in current:
                current.add(value)
                seed_path.write_text("".join(f"{x}\n" for x in sorted(current)),
                                     encoding="ascii")
                added += 1
    print(f"[CENSUS] merged {added} new cross-bank targets")


def report(args):
    out = Path(args.out)
    executed, verified = load_exec(), load_verified()
    banks, all_starts = [], set()
    for bank in CANONICAL_BANKS:
        path = out / "listings" / f"bank_{bank:02X}_instructions.tsv"
        starts, code_bytes = [], 0
        if path.exists():
            for line in path.read_text(errors="replace").splitlines():
                columns = line.split("\t", 2)
                if len(columns) < 2:
                    continue
                address, length = int(columns[0], 16), int(columns[1])
                starts.append(address); all_starts.add(address); code_bytes += length
        observed = sum(contains(executed, address) for address in starts)
        # Some ledger entries intentionally describe a whole host boundary.
        # Verification credit still requires the start to be natively observed.
        proven = sum(contains(executed, address) and contains(verified, address)
                     for address in starts)
        banks.append({"bank": f"{bank:02X}", "rom_bytes": 0x8000,
                      "decoded_starts": len(starts), "decoded_bytes": code_bytes,
                      "observed_starts": observed, "verified_starts": proven,
                      "undecoded_bytes": 0x8000 - code_bytes})
    totals = {key: sum(row[key] for row in banks) for key in
              ("rom_bytes", "decoded_starts", "decoded_bytes", "observed_starts",
               "verified_starts", "undecoded_bytes")}
    payload = {
        "schema": 1,
        "method": "recursive Ghidra seeds; no linear sweep; unknown bytes may be data or code",
        "totals": totals,
        "banks": banks,
    }
    Path(args.write_json).write_text(json.dumps(payload, indent=2) + "\n")
    decoded = totals["decoded_starts"]
    pct = 100.0 * totals["verified_starts"] / decoded if decoded else 0.0
    lines = ["# Full-ROM conservative instruction census", "",
             "Generated by `tools/full_rom_census.py` and headless Ghidra. It is a recursive",
             "code census seeded from native execution, verified routine entries, source",
             "provenance, recomp functions and SNES vectors. It deliberately does **not**",
             "linear-sweep undecoded bytes, because this ROM interleaves code, graphics, audio",
             "and tables. Undecoded bytes are unknown—not pending instructions.", "",
             "## Totals", "", "| metric | count |", "|---|---:|",
             f"| ROM bytes | {totals['rom_bytes']:,} |",
             f"| Conservatively decoded instruction starts | {decoded:,} |",
             f"| Bytes owned by decoded instructions | {totals['decoded_bytes']:,} |",
             f"| Decoded starts observed in retained execution | {totals['observed_starts']:,} |",
             f"| Decoded starts both observed and verified | {totals['verified_starts']:,} |",
             f"| Verified / conservatively decoded starts | {pct:.2f}% |",
             f"| Undecoded ROM bytes (data or undiscovered code) | {totals['undecoded_bytes']:,} |",
             "", "## Per physical LoROM bank", "",
             "| bank | decoded starts | code bytes | observed starts | verified starts | undecoded bytes |",
             "|---|---:|---:|---:|---:|---:|"]
    for row in banks:
        lines.append(f"| `${row['bank']}` | {row['decoded_starts']} | {row['decoded_bytes']} | "
                     f"{row['observed_starts']} | {row['verified_starts']} | {row['undecoded_bytes']} |")
    lines += ["", "## Interpretation", "",
              "This establishes a reproducible lower-bound instruction universe. The verified",
              "percentage above requires native observation plus a verified-ledger boundary",
              "and is a percentage of conservatively decoded starts, not retail",
              "feature completion. Expanding seeds or resolving an indirect table can increase",
              "the denominator without regressing the port. Whole-game planning belongs in",
              "`docs/feature-capture-matrix.md`.", "",
              "The decoded-start count also differs from `docs/progress.md`: retained exec",
              "files contain address-position intervals, while this report counts only Ghidra-",
              "decoded instruction starts. The two numerators must not be substituted.", ""]
    Path(args.write_md).write_text("\n".join(lines), encoding="utf-8")
    print(f"[CENSUS] decoded={decoded} verified={totals['verified_starts']} ({pct:.2f}%)")


def main():
    parser = argparse.ArgumentParser()
    sub = parser.add_subparsers(dest="command", required=True)
    p = sub.add_parser("prepare"); p.add_argument("--rom", required=True)
    p.add_argument("--out", required=True)
    p.add_argument("--recomp", default=str(ROOT.parent / "NBA-Live-95-Recomp"))
    p.set_defaults(run=prepare)
    p = sub.add_parser("merge"); p.add_argument("--out", required=True); p.set_defaults(run=merge_calls)
    p = sub.add_parser("report"); p.add_argument("--out", required=True)
    p.add_argument("--write-md", required=True); p.add_argument("--write-json", required=True)
    p.set_defaults(run=report)
    args = parser.parse_args(); args.run(args)


if __name__ == "__main__":
    main()
