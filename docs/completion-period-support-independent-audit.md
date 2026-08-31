# Period support v1: independent source and protocol review

Decision: the three bounded source projections pass within the explicit raw
adapter preconditions below. The original v1 verifier/adapter composite is
rejected until those preconditions and complete metadata/route checks are
enforced. This is not integrated period or whole-game acceptance.

Owner `build/period-support-freeze-v1.json` SHA-256
`96124c0eef5fbc945f7e5f6befbb68dd295126efa199418f0a74df2da794919c`
contains 856 independently rehashed identities. Source/header dependencies
were privately snapshotted before the build. The fresh `/W4 /WX` build compiles
candidate, probe and player-lab source and links the exact frozen root objects;
it has no compiler warning. All eleven original native calls pass 34,126
compared bytes. All seventeen original protocol/metadata negatives and nine
unsupported-input refusals pass. Thirty-three binary/stdout/stderr artifacts
are byte-identical to the owner's frozen strict-v1 outputs.

## Assignment D85E–DA17

The four native calls pass. An independent fixed source-dataflow diagnostic
reads the original actor/statistic tables, carried WRAM roster pointers and
original ROM profile bytes; it uses no C asset accessor for expected results.
It reproduces the four native endpoints, then passes 120 bijective selector
permutations with varied roster slots and high selector flags against fresh C.
This is controlled source coverage, not natural permutation coverage.

D7B8 copies carried tables $3471/$34A1 into active pointers. D8B4/D8DE read
actual actor +00 IDs. D939/D9C6 write help flags through the paired actor;
the candidate correctly retains and comments that destination. The key sum
wraps at eight bits at D920/D9AD before the optional +100 class. D73E keeps
equal-key order; both calls reuse $09DA, so only the second key list survives.
All those results match the reviewed translation.

The typed input uses selected-team/roster semantics and assumes the parent
published actor IDs 0..9. Its raw adapter must verify both conditions rather
than silently repairing original memory. Two retained controlled witnesses:

- Actor0 +00 changed to 1 makes original D8DE write alternate assignment 2 at
  $3A63, while v1 C writes 0.
- Replacing a carried $3471 pointer with the next roster pointer changes the
  original active pointer, roles, sorted keys and team order; v1 C reconstructs
  the unmodified selected-team pointer and differs in nine projected bytes.

No natural violation was observed. These are unrepresented raw-input domains,
not confirmed original-game bugs.

## D5DB object sort

An actual-ROM-byte diagnostic independently passes all four native cases and
512 controlled cases with shuffled objects, poisoned carried links, equality,
$7FFF/$8000/$FFFF and random full-word coordinates. It visits 59 source PCs.
The wrapped subtraction-sign comparison and stable equal-key behavior match;
links not moved by the original are preserved.

The lower array bound represents **zero at original address $34D1**. After
moving a record into the first slot, D632/D634 decrement the source pointer,
D636 reads the preceding sentinel and D63A stops only if it is zero. With a
controlled $34D1=$4000, $4004=100 and an ordered leading pair, original ROM
publishes list[0]=$4000 and actor1 link=$34D1, while C has list[0]=$35EB and
link=$34D3. This witness is retained under `independent-sort-v1`. It is outside
the bounded typed-array contract. Original raw memory must be rejected or the
zero sentinel established by the real caller; it must not be normalized.

## E183–E1A4 attachment

Three native composed calls pass without seeding any later child endpoint.
The source order is BC9B transfer, B538/B555 cancellation, B3BD state12 install,
AEC3 pose, B649 XY and B66A Z. AEC3 uses requested facing +4E. The source state12
descriptors at $84:C79A/$CE1E/$D1B4 have mode0/lock0/duration512/count1.

An independent original-ROM table check passes 64 controlled CPU/no-controller
cases over all eight facings, both lower tables, four mirror-flag combinations
and wrapping XYZ inputs. Full compared actor/ball/controller projections are
checked. State12's resource set never supplies the -128 Y byte; this does not
certify the reused generic attachment helper for arbitrary byte extremes or
arbitrary poses. The native/controlled cases do not establish human transfer
coverage. B649's previous-X save and integer-only XY/Z writes preserve ball
fractions and velocities as required.

## Verifier rejection and later repair work

V1 `validate_metadata` checks selected scalar types but has no complete row/field
schema or court origin, and does not enforce the three raw preconditions above.
The independent twelve-case protocol tool accepts all twelve malformed cases:
extra row key, missing/nontext tag, missing/external raw path, missing/wrong-type
fields, boolean field word, wrong court clock, and the three raw-source
counterexamples. It also trusts the old report's command/case list; the same
missing/duplicated/stale-route concern documented for appearance v2 applies.

Subsequent **unfrozen** owner support verifier v2 was examined separately:
its new source-domain checker rejects all three retained independent raw
counterexamples, and its team pointer check correctly derives all 24 roster
pointers from original $84:E640 records. Its derived command list no longer
reads the old C report. A new full-comparison-path test verifies eleven exact
canonical commands and rejects six malformed parsed cases, but a wrong numeric
fields.093e still reaches C because the shared metadata function checks schema
without raw-field equality. The owner was notified before freezing a repair.
This paragraph is development evidence, not acceptance of an unfrozen revision.

No original frozen source, capture or rejected evidence was edited. This packet
excludes CPU/DP register residue, DBR proof, hardware timing and later role/depth
work. The old README's description of 80FBFF as a video child is not accepted;
the owner's later source inspection identifies a separate depth sort, outside
this review.

## Identities and retained evidence

| Object | SHA-256 |
| --- | --- |
| candidate C | `3c9c82354bf5856be81065b00878f7d5fa040b06e4017964b85f38a698552896` |
| header | `eb7e5d31aa5f135722400aafd00f7c2c3367c1a36b79da299ebfad37adf9b590` |
| adapter | `db9f1b24c938ee4f18f30594b075b7f7c46740fe582543eb7c8bbe82280154f5` |
| v1 strict verifier | `c44ad43910f5de4b01b70397837c09abf169ae2703343102f7bcbecb4e571e63` |
| private executable | `ae14a6eb0ba1d4d8aa7a6d4e71fea028d51497546f733222c013f0ec3a8fbd13` |
| independent ROM sort diagnostic | `2097888549372e45e4c3d0af1dabc4419efa66836a8140d434352200f52c594c` |
| independent assignment diagnostic | `d53f4a64dd3872a41f6842d8f1758ae3be35edad7fd310062e6272c75029cb0d` |
| independent attachment diagnostic | `e1924bca064d3b005391afb3be7a3e52e148dfa382fd7fa20e1c47c48ea99bc3` |
| independent v1 protocol cases | `d965553a1574788fa637764711466c368c0945e18de2c7b39fe7496fcda2c6b0` |
| adaptive derived-route cases | `35fc4a2ff740e0fe7bf95bb507b2bb38a826243017969001a2baeb2e58f1b099` |

All diagnostic tools are in the auditor's `tools/`; private build and reports
are under `build/period-support-audit-v1`, including the unchanged counterexample
input and ROM-result files. The original ROM SHA is
`2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`.
