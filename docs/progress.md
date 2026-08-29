# Port progress

Derived by tools/progress.py from Mesen exec coverage, src/ provenance comments, docs/verified-routines.json, and the recomp function set. Do not edit by hand.

## Captured-address coverage (banks the captures observed)

Counts address positions in the captured exec intervals. Some captures record instruction starts; older captures also bridge small gaps. These are not a disassembled instruction census, byte-accurate execution coverage, or a whole-game completion percentage.

| metric | address positions | % of captured |
|---|---|---|
| executed (denominator) | 27901 | 100.0% |
| documented by port provenance | 15940 | 57.1% |
| verified against ground truth | 15800 | 56.6% |

## Per bank

| bank | executed | documented | % |
|---|---|---|---|
| $00 | 30 | 0 | 0.0% |
| $80 | 7517 | 4690 | 62.4% |
| $81 | 2589 | 10 | 0.4% |
| $82 | 1937 | 418 | 21.6% |
| $83 | 1043 | 0 | 0.0% |
| $84 | 195 | 0 | 0.0% |
| $85 | 5095 | 4141 | 81.3% |
| $86 | 6114 | 4639 | 75.9% |
| $87 | 3381 | 2042 | 60.4% |

## Functions

- recomp-discovered functions: 136 (banks 00/80/81/82 only; static analysis stops at indirect dispatch)
- of those observed executing in captures: 126
- of those referenced by port provenance: 33
- verified routines (ledger): 212

## Largest undocumented executed regions

| range | address positions |
|---|---|
| $84:BF75-$84:BFA6 | 50 |
| $00:8600-$00:861C | 29 |
| $80:A8F9-$80:A90A | 18 |
| $80:A963-$80:A973 | 17 |
| $82:ACAE-$82:ACB8 | 11 |
| $80:BFC3-$80:BFCB | 9 |
| $80:C247-$80:C24F | 9 |
| $81:9A08-$81:9A10 | 9 |
| $81:9AD7-$81:9ADF | 9 |
| $81:9BDA-$81:9BE2 | 9 |
