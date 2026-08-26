# Port progress

Derived by tools/progress.py from Mesen exec coverage, src/ provenance comments, docs/verified-routines.json, and the recomp function set. Do not edit by hand.

## Executed-code bytes (banks the captures observed)

| metric | bytes | % of executed |
|---|---|---|
| executed (denominator) | 27901 | 100.0% |
| documented by port provenance | 8480 | 30.4% |
| verified against ground truth | 213 | 0.8% |

## Per bank

| bank | executed | documented | % |
|---|---|---|---|
| $00 | 30 | 0 | 0.0% |
| $80 | 7517 | 128 | 1.7% |
| $81 | 2589 | 10 | 0.4% |
| $82 | 1937 | 418 | 21.6% |
| $83 | 1043 | 0 | 0.0% |
| $84 | 195 | 0 | 0.0% |
| $85 | 5095 | 3367 | 66.1% |
| $86 | 6114 | 3529 | 57.7% |
| $87 | 3381 | 1028 | 30.4% |

## Functions

- recomp-discovered functions: 298 (banks 00/80/81/82 only; static analysis stops at indirect dispatch)
- of those observed executing in captures: 252
- of those referenced by port provenance: 67
- verified routines (ledger): 8

## Largest undocumented executed regions

| range | bytes |
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
