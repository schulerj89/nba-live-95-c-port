# Period restart parent v2: bounded acceptance

PASS for the independently reviewed parent component. This supersedes the v1
source/verifier rejection, which remains recorded without alteration in
`completion-period-restart-independent-audit.md`. It does not accept external
appearance/assignment/sort children, whole period transitions, or timing.

Scheduler `.analysis/period-restart-freeze-v2.json` has SHA-256
`02b6365e8e8e06d65e009fb4c6115e644392796e89fd2cb3a3d4eafbeb38a19f`.
All 809 identities were independently rehashed, including all 666 unchanged
v1 identities. Fresh private compilation used the exact copied v2 two-source
build script and `/W4 /WX`. No original source or native file was modified.

The production-source diff is only the new header name and the missing Y
negation in the nonnegative-anchor opening/overtime branch. The comment now
correctly calls the previous behavior a port error. Original bytes at
$86:DDE7–DDEE are `A5 BA 49 FF FF 1A 85 BA`; v2 preserves that original
negation. No other source behavior was changed to match tests.

The unchanged independent ROM diagnostic now passes all 20 cases / 1,600
coordinate fields with zero differences, versus the retained v1 eighty
Y/target-Y failures. This includes the concrete opening case where actor0
must be (-8,-3,2) and actor5 (+8,+3,6) for positive team0 anchor. These are
controlled source cases, not natural observations of the corrected branch.

The verifier now requires binary 16-bit status and DP0 from formation.table
onward, including the decimal bit. DD97 entry X=0/Y=$34EB, carried B6=context0
anchor, and cursor9A=$34D3 are bound to the actual earlier source stores. A is
not artificially constrained because DD97 overwrites it. The unchanged ten
independent malformed CPU/domain cases all reject. The first private command
omitted the test's required `--exe` argument and exited in argparse; the
corrected invocation ran the actual cases without editing the tool.

Fresh four-capture replay passes 70 parent boundaries / 28,420 typed word
comparisons. All eight native input/trace output files are byte-identical to
the auditor's v1 replay. All 56 local checks pass. The new local formation
reference reads actual ROM table/transform operands and explicit source-block
destinations; it no longer duplicates the candidate's incorrect Y assumption.
The independent diagnostic still executes actual original instructions and
does not depend on that new reference.

The limitations from the original audit remain: the native captures are
controlled expiry routes with tip winner5; excluded AAB2/D85E/D5DB child
returns are typed diagnostic inputs, not their implementations. Parent
regulation stops before E183's BC9B, and opening/OT stops at E1AC. Preserved
A6 loop indices, fractions, readiness and dead-coordinate behavior remain
unchanged and commented. No normal full-game parity or scheduler phase is
claimed. Appearance/support child reviews are separate packets.

| Object | SHA-256 |
| --- | --- |
| v2 C | `1fdd80edeee9c8dbfa033fd93151c437375339a10beb89a6ec341ac5748b8b0c` |
| v2 header | `582629dcbadfc124657005d4722cb6ee43172180f7f1a9490816a0099311d67a` |
| v2 verifier | `68d22789ecaff106b9b2c773a821a5a3510c3a984dd5d8aeac6e61b03c6f2eca` |
| private fresh executable | `7e3d316e7b9a713d60f367a2a7190710119fa8c8ba6c73be9b8ac145fa9382a5` |

Private evidence: `build/period-restart-audit-v2/{compiled-v1,native-v1,
local-v1,independent-rom-v1,independent-domain-v1,preserved-output.json}`.
