# Period CPU appearance: v1 independent review

Decision: the bounded C source and the forty immutable native input/output
pairs pass; the v1 verifier composite is rejected for missing CPU/caller
metadata validation. No production wiring, normal period transition, human
palette, hardware timing, or whole-game acceptance is implied.

The owner freeze `build/period-appearance-freeze-v1.json` has SHA-256
`80ea029505909b94ef2c67a6dd03aa2b8389b8d78a08261dc2f0318ded303c92`.
All 931 recorded identities were independently rehashed. The auditor copied
candidate, adapter, player-lab source and headers privately before building;
the exact 37 frozen root objects were linked read-only. No original file was
changed. The separate support packet's 856 identities were also rehashed and
its source dependencies snapshotted, without accepting that component.

## Fresh verification

Private `/W4 /WX` compilation in `build/period-appearance-audit-v1/compiled-v1`
compiled the candidate, adapter and player-lab source. The build log contains
no warning. Probe SHA-256 is
`f8ff8cdbfe8678e105f69f653056902be30e1509f416ec3a25803e1568995239`.
Forty calls compare all 128 actor words, RNG $07F6 and owner pointer $0940:
5,200 words pass. The copied strict verifier, redirected only to the private
probe/report/output directory, independently passes its 14 malformed-output
rejections and nine unsupported-input refusals. Existing failed owner attempts
remain immutable.

The actual raw records and metadata were inspected independently. All forty
selected before/after pairs have binary 16-bit status (`PS & $38 == 0`), DP zero,
matching frame/court/SP, the expected `$86:DFCB -> DFCF` or `DFD8 -> DFDC`
call/return PCs, and DP $96 selecting the compared actor. First-before Y equals
that actor address after DFC9's STY; second-before Y is the first actor and A
equals its address plus $500 after DFD1/DFD3. All supply $C6=2. All forty have
carried base/upper/lower state zero. DBR is absent from this capture schema;
this is not independent proof of a general DBR or CPU-register contract.

`tools/test_period_appearance_rom_audit.py` executes the actual immutable ROM
bytes for AAB2 and its bounded CPU children with binary 16-bit arithmetic,
DP0/DB0, and no hardware interleaving. It independently reproduces all 5,200
native words, then compares the fresh C probe against 114 controlled records:
19 original carried base states, both lower-body tables, and facings 0/3/7.
The cases carry a nonzero catcher latch, exercise direction fallback from
$FFFF, and include the full-word XBA input $0102 as well as $0002. All pass,
visiting 256 source PCs. Those controlled inputs are diagnostic copies, not
modified native fixtures or evidence of natural reachability. The diagnostic
is not production code and makes no instruction-duration claim.

## Source contract and preserved quirks

The source clears owner pointer at $87:A9DF under owner $FFFF, then performs
the stationary, unboosted, nonowner B572 table chain using original ROM tables
$84:C3CA, C462 and C3A4. Z is zero, so the airborne table is not entered.
The admitted zero-velocity path does not execute B590's catcher-latch clear;
the candidate correctly carries that latch. B572 updates base state as well
as unlocked channels. B606 uses the upper descriptor for upper phase-count
validation; **B630 uses canonical C218 even when +A8 selects alternate lower
cadence at AB5F**. The candidate preserves and comments that original table
choice. This is source behavior, not a claimed naturally observed bug.

AB38 repairs requested direction >=8 from displayed direction; actual cadence
and resource selection use displayed +52. Status bit $8000 is cleared then set
for displayed facing <3. ABA7/ACC6 consume the complete byte-swapped C6 word.
The existing channel helper preserves lower resource resolution before upper
cadence, queued state replacement without reloading locks, and the original
negative-descriptor synchronization paths. The controlled diagnostic includes
the idle RNG path from carried state seven. Human palette work is skipped by
the explicitly unassigned controller and remains excluded.

This source acceptance is bounded by the candidate's CPU-period conditions
and the exercised original-ROM cases. It does not certify all arbitrary
animation queues/descriptor states, every original AAB2 input, CPU/DP residue,
or a complete period caller.

## Verifier defect requiring a separate revision

Owner `verify_protocol.py` rehashes the native freeze, validates strict C output
and compares endpoints, but never interprets CPU metadata. Its claimed
upstream checker `build/period-restart-attribution-v1/check_captures.py` likewise
never validates PS, DP, call/return PC, SP, or caller registers. File hashes
attest the immutable files; they do not implement these semantic checks.

`tools/test_period_appearance_metadata_audit.py` mutates parsed rows after the
original file hashes have been checked, leaving all files and hashes intact.
The original checker accepts all twelve malformed cases: decimal mode before
or after, M8, X8, DP1, wrong call PC, wrong return PC, SP0 on return, wrong first
entry Y, missing PS, boolean PS, and a wrong court clock. Each mutation reaches
25 appearance rows across the checker's ten historical capture directories.
This test does not claim an attacker can bypass a pinned freeze by editing
its files. It demonstrates the missing parsed-state contract promised by the
verifier. Reports are retained under `independent-metadata-v1`.

A new verifier must check exact typed metadata, PS/DP domain, expected adjacent
call/return tags and PCs, unchanged frame/court/SP, and the actual source-defined
DP96/actor and Y/A relations. It should reconstruct native calls from those
validated rows, rather than treating stored `attempt-v3/report.json` commands
and actor/exit fields as self-authenticating. These are verifier repairs; no
candidate C change or original capture rewrite is indicated by this review.

## Identities

| Object | SHA-256 |
| --- | --- |
| candidate C | `05f3489e1ce59a769631b43bfc776e5ed997ee7e962339b8e9e7ee1a94e7027b` |
| candidate header | `49c6265e94bba9ca2b228ad4bf491274794e0add8180ff0509e02493dbccf8b7` |
| adapter | `9b53630c6408bda17a8711a79f1680a6edec110a62adf54b1cafb106144f2bdb` |
| original strict verifier | `848276d510aef4c7b344c42f76d1dc7b9e562ef663d72697ff5ef1d30059c939` |
| original capture checker | `90026887bc435f33e6833ec6497a229c95c97c282e2838ce94a8739306d7f99e` |
| independent ROM diagnostic | `4144fd5a636d8c0b9f860d6f73625eb792fcfc0fa3104b83a383a292a6fb8d1b` |
| independent metadata cases | `b32f83860d73aa44e311dc33b280b6e0c3ff2cabd9da566c1b2af48f541e22ef` |
| original ROM | `2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870` |

Audit outputs are under the auditor's `build/period-appearance-audit-v1`.
Original source, native snapshots, rejected evidence and freezes are unchanged.
