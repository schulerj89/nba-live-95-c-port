# Port progress

Derived by tools/progress.py from Mesen exec coverage, src/ provenance comments, docs/verified-routines.json, and the recomp function set. Do not edit by hand.

## Captured-address coverage (banks the captures observed)

Counts address positions in the captured exec intervals. Some captures record instruction starts; older captures also bridge small gaps. These are not a disassembled instruction census, byte-accurate execution coverage, or a whole-game completion percentage.

| metric | address positions | % of captured |
|---|---|---|
| executed (denominator) | 27901 | 100.0% |
| documented by port provenance | 11332 | 40.6% |
| verified against ground truth | 10379 | 37.2% |

## Per bank

| bank | executed | documented | % |
|---|---|---|---|
| $00 | 30 | 0 | 0.0% |
| $80 | 7517 | 155 | 2.1% |
| $81 | 2589 | 10 | 0.4% |
| $82 | 1937 | 418 | 21.6% |
| $83 | 1043 | 0 | 0.0% |
| $84 | 195 | 0 | 0.0% |
| $85 | 5095 | 4124 | 80.9% |
| $86 | 6114 | 4583 | 75.0% |
| $87 | 3381 | 2042 | 60.4% |

## Functions

- recomp-discovered functions: 136 (banks 00/80/81/82 only; static analysis stops at indirect dispatch)
- of those observed executing in captures: 126
- of those referenced by port provenance: 6
- verified routines (ledger): 187

## Largest undocumented executed regions

| range | address positions |
|---|---|
| $80:81A8-$80:82A4 | 253 |
| $80:A3B8-$80:A465 | 174 |
| $80:A2BF-$80:A360 | 162 |
| $80:A9B3-$80:AA29 | 119 |
| $80:84EE-$80:853C | 79 |
| $80:A77B-$80:A7C5 | 75 |
| $80:CE33-$80:CE7A | 72 |
| $80:AACD-$80:AB05 | 57 |
| $80:E5C7-$80:E5FE | 56 |
| $80:A65C-$80:A692 | 55 |
