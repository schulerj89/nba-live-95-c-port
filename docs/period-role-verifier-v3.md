# Period role verifier v3

Verifier-only correction: each emitted row now applies the declared projection
width. The ten context+49 order bytes must be0..255, while word fields remain
0..65535; booleans remain invalid numbers. C source, public API, native fixtures
and the1013-identity v2 freeze are unchanged.

Auditor found v2 accepted a forged first-return order byte256/65535 because that
row's schema allowed16bits and only finalE1F7 values have a native counterpart.
V3 rejects both at the per-row schema guard. It does not claim unobserved native
first-return value parity. Controlled original-ROM cases still compare every
emitted first/final value separately.

The unchanged auditor13case tool now passes. Its hardcoded v2 module import is
bound to the v3 verifier only by the explicit test launcher; no case or original
tool file is rewritten. All four native final223field comparisons pass; their
8binary-input/text-trace files are byte-identical to the v2 baseline. No new
source behavior, timing or production acceptance is implied. Independent review
is pending.

Reproduction: run tools/test_period_roles_verifier_v3.py with --audit_tool set
to ../completion-auditor/tools/test_period_roles_protocol_audit.py and the same
--rom, --exe, --native and a fresh --output as the v2 protocol run. Use
verify_period_roles_v3.py for the full four-capture native check.
