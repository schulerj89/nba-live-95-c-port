"""Quantify port progress from evidence instead of memory.

Crosses three machine-readable sources:
  1. Executed-code coverage: every `.analysis/**/exec_*.txt` range file the
     Mesen captures produced (the denominator: code the game actually runs).
  2. Provenance ranges: `$XX:XXXX` and `$XX:XXXX-$YYYY` references in
     `src/*.c` comments (what the port documents as covered).
  3. `docs/verified-routines.json` plus recomp-discovered functions from
     `../NBA-Live-95-Recomp/generated/bank*.c` (function-level status).

Nothing here is hand-maintained: re-run after capturing, porting, or
verifying and the numbers move on their own.

Usage:
    python tools/progress.py [--recomp <path>] [--write docs/progress.md]
"""

import argparse
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def merge(intervals):
    merged = []
    for start, end in sorted(intervals):
        if merged and start <= merged[-1][1] + 1:
            merged[-1][1] = max(merged[-1][1], end)
        else:
            merged.append([start, end])
    return [(s, e) for s, e in merged]


def total(intervals):
    return sum(e - s + 1 for s, e in intervals)


def intersect(a, b):
    result, i, j = [], 0, 0
    while i < len(a) and j < len(b):
        start = max(a[i][0], b[j][0])
        end = min(a[i][1], b[j][1])
        if start <= end:
            result.append((start, end))
        if a[i][1] < b[j][1]:
            i += 1
        else:
            j += 1
    return result


def subtract(a, b):
    result = []
    j = 0
    for start, end in a:
        cursor = start
        while j < len(b) and b[j][1] < cursor:
            j += 1
        k = j
        while k < len(b) and b[k][0] <= end:
            if b[k][0] > cursor:
                result.append((cursor, b[k][0] - 1))
            cursor = max(cursor, b[k][1] + 1)
            k += 1
        if cursor <= end:
            result.append((cursor, end))
    return result


def load_executed(analysis):
    intervals = []
    for path in analysis.rglob("exec_*.txt"):
        for line in path.read_text().splitlines():
            match = re.fullmatch(r"([0-9A-Fa-f]{6})-([0-9A-Fa-f]{6})", line.strip())
            if match:
                intervals.append((int(match.group(1), 16), int(match.group(2), 16)))
    return merge(intervals)


PROVENANCE = re.compile(
    r"\$([0-9A-Fa-f]{2}):([0-9A-Fa-f]{4})"
    r"(?:\s*-\s*\$?(?:([0-9A-Fa-f]{2}):)?([0-9A-Fa-f]{4}))?")


def load_provenance(src):
    ranges, points = [], set()
    for path in sorted(src.glob("*.c")):
        for match in PROVENANCE.finditer(path.read_text(encoding="utf-8")):
            bank, offset, end_bank, end_offset = match.groups()
            start = (int(bank, 16) << 16) | int(offset, 16)
            if end_offset:
                end = (int(end_bank, 16) << 16 if end_bank
                       else start & 0xFF0000) | int(end_offset, 16)
                if end >= start and end - start < 0x10000:
                    ranges.append((start, end))
                    continue
            points.add(start)
    return merge(ranges), points


def load_recomp_functions(recomp):
    functions = set()
    for path in recomp.glob("generated/bank*.c"):
        for match in re.finditer(
                r"^[^\s/].*?\bbank_([0-9A-Fa-f]{2})_([0-9A-Fa-f]{4})\s*\(",
                path.read_text(encoding="utf-8", errors="replace"), re.M):
            functions.add((int(match.group(1), 16) << 16) |
                          int(match.group(2), 16))
    return functions


def load_verified(docs):
    path = docs / "verified-routines.json"
    if not path.exists():
        return [], []
    entries = json.loads(path.read_text())["routines"]
    intervals = []
    for entry in entries:
        start, end = (int(part, 16) for part in entry["range"].split("-"))
        intervals.append((start, end))
    return merge(intervals), entries


def fmt(address):
    return f"${address >> 16:02X}:{address & 0xFFFF:04X}"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--recomp",
                        default=str(ROOT.parent / "NBA-Live-95-Recomp"))
    parser.add_argument("--write", help="also write a markdown report here")
    parser.add_argument("--gaps", type=int, default=10,
                        help="largest undocumented executed regions to list")
    args = parser.parse_args()

    executed = load_executed(ROOT / ".analysis")
    ported, points = load_provenance(ROOT / "src")
    verified, verified_entries = load_verified(ROOT / "docs")
    recomp_functions = load_recomp_functions(Path(args.recomp))

    covered = intersect(executed, ported)
    verified_executed = intersect(executed, verified)
    gaps = sorted(subtract(executed, ported),
                  key=lambda iv: iv[0] - iv[1])[:args.gaps]

    def in_ranges(address, intervals):
        return any(s <= address <= e for s, e in intervals)

    recomp_seen = {fn for fn in recomp_functions if in_ranges(fn, executed)}
    recomp_ported = {fn for fn in recomp_functions
                     if in_ranges(fn, ported) or fn in points}

    lines = ["# Port progress", "",
             "Derived by tools/progress.py from Mesen exec coverage, src/ "
             "provenance comments, docs/verified-routines.json, and the "
             "recomp function set. Do not edit by hand.", ""]
    exec_total = total(executed)
    lines += [
        "## Executed-code bytes (banks the captures observed)", "",
        "| metric | bytes | % of executed |", "|---|---|---|",
        f"| executed (denominator) | {exec_total} | 100.0% |",
        f"| documented by port provenance | {total(covered)} | "
        f"{100 * total(covered) / exec_total:.1f}% |",
        f"| verified against ground truth | {total(verified_executed)} | "
        f"{100 * total(verified_executed) / exec_total:.1f}% |", "",
        "## Per bank", "", "| bank | executed | documented | % |", "|---|---|---|---|"]
    banks = sorted({s >> 16 for s, _ in executed})
    for bank in banks:
        window = [(bank << 16, (bank << 16) | 0xFFFF)]
        bank_exec = total(intersect(executed, window))
        bank_cov = total(intersect(covered, window))
        lines.append(f"| ${bank:02X} | {bank_exec} | {bank_cov} | "
                     f"{100 * bank_cov / bank_exec:.1f}% |")
    lines += ["", "## Functions", "",
              f"- recomp-discovered functions: {len(recomp_functions)} "
              f"(banks 00/80/81/82 only; static analysis stops at indirect "
              f"dispatch)",
              f"- of those observed executing in captures: {len(recomp_seen)}",
              f"- of those referenced by port provenance: {len(recomp_ported)}",
              f"- verified routines (ledger): {len(verified_entries)}", "",
              "## Largest undocumented executed regions", "",
              "| range | bytes |", "|---|---|"]
    for start, end in gaps:
        lines.append(f"| {fmt(start)}-{fmt(end)} | {end - start + 1} |")
    report = "\n".join(lines) + "\n"

    print(f"[PROGRESS] executed={exec_total} "
          f"documented={total(covered)} ({100 * total(covered) / exec_total:.1f}%) "
          f"verified={total(verified_executed)} "
          f"({100 * total(verified_executed) / exec_total:.2f}%) "
          f"provenance_points={len(points)} "
          f"recomp_fns={len(recomp_functions)}/{len(recomp_ported)} ported")
    if args.write:
        Path(args.write).write_text(report, encoding="utf-8")
        print(f"[PROGRESS] wrote {args.write}")


if __name__ == "__main__":
    main()
