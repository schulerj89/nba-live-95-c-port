# Port progress

Derived by tools/progress.py from Mesen exec coverage, src/ provenance comments, docs/verified-routines.json, and the recomp function set. Do not edit by hand.

## Captured-address coverage (banks the captures observed)

Counts address positions in the captured exec intervals. Some captures record instruction starts; older captures also bridge small gaps. These are not a disassembled instruction census, byte-accurate execution coverage, or a whole-game completion percentage.

| metric | address positions | % of captured |
|---|---|---|
| executed (denominator) | 29438 | 100.0% |
| documented by port provenance | 29101 | 98.9% |
| verified against ground truth | 11529 | 39.2% |

## Per bank

| bank | executed | documented | % |
|---|---|---|---|
| $00 | 30 | 30 | 100.0% |
| $80 | 7836 | 7836 | 100.0% |
| $81 | 2855 | 2855 | 100.0% |
| $82 | 1988 | 1651 | 83.0% |
| $83 | 1053 | 1053 | 100.0% |
| $84 | 206 | 206 | 100.0% |
| $85 | 5345 | 5345 | 100.0% |
| $86 | 6510 | 6510 | 100.0% |
| $87 | 3615 | 3615 | 100.0% |

## Functions

- recomp-discovered functions: 136 (banks 00/80/81/82 only; static analysis stops at indirect dispatch)
- of those observed executing in captures: 126
- of those referenced by port provenance: 135
- verified ledger entries: 233 total, 207 eligible for address coverage

## Largest undocumented executed regions

| range | address positions |
|---|---|
| $82:F903-$82:F908 | 6 |
| $82:F9D0-$82:F9D5 | 6 |
| $82:FA18-$82:FA1D | 6 |
| $82:F987-$82:F98A | 4 |
| $82:F9BA-$82:F9BD | 4 |
| $82:F9EA-$82:F9ED | 4 |
| $82:FA08-$82:FA0B | 4 |
| $82:EFED-$82:EFEF | 3 |
| $82:F8C9-$82:F8CB | 3 |
| $82:F8EE-$82:F8F0 | 3 |
