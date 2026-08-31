# Independent human switch checkpoint review

FAIL for acceptance of switch freeze-v1, SHA256
`ce108ebccee954926567d7102ad78b259511ff66fd83b45efac617ba9e40367f`.
The newly added switch body matches the inspected original routine and all
captured persistent effects, but its existing direction dependency diverges
for supported full-word inputs, and the verifier accepts impossible or
contradictory event metadata. Frozen source and native fixtures remain intact.

All69 source/dependency/object/artifact identities in the freeze were checked.
The eleven new files were copied byte-exact to
`build/switch-audit-v1/source`. I rebuilt the new module and probe with
MSVC /W4 /WX, linking the auditor's earlier freshly built accepted controller
objects read-only. Fresh probe SHA256:
`5bd9c8f927924668450c586203c1278914291929b0bbb71481555136ae51a834`.
Both unchanged native captures pass:14+26 whole switch calls,68,480 compared
values,32 transfers/eight retains, and no filtered frame crossings. The
original35 verifier tests also pass. These are native-prestate replays, not
normal C human journeys or production enablement.

The original ROM SHA256 is
`2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`.
I compared all five reference byte ranges and all ten reference artifacts,
read both Ghidra listings and the C caller/probe, and checked the capture's
ordinary-button route and private-process settings. Switch bytes84:E141-E2AC
hash to `c725d8ffa282684386f10cb3d42a64d094f95b8b8a654922f123fdb9ad797411`.
The unsigned count gate, assigned-player exclusion, directional later-tie
replacement, neutral earlier-tie retention, source-group transfer and published
090C record destination agree with the original. Incoming A6/C2 are deliberately
retained on the no-improvement path. That original uninitialized-scratch quirk
is commented, not normalized. Unmapped host pointers remain explicitly rejected;
CPU flags/registers, volatile scratch and production scheduling are outside the
semantic API. The probe consumes entry bytes only; native returns stay in the
verifier and compare all advertised persistent regions and restored words.

The blocking source defect is in the frozen existing
`src/nba_gameplay_ai.c:80` and`:84`, SHA256
`94cce032be9023edc6f93ba4d7d80bd91568b8e6d97a3ffe70909b52838fd9ea`.
F37D and F399 branch on the sign of wrapped16-bit subtraction. The C helper
instead compares separately signed operands, which differs when subtraction
overflows. F37D accepts equality or negative; F399 accepts negative only.
This is a port error; preserving the original game requires preserving its
wrapped branch behavior.

A concrete controlled C prestate derived from an actual entry isolates the
effect: actor0 at(0,0), candidate1 at($8000,1), direction0, all other teammates
controlled, incoming A6=current. At F37D,0000-8000 wraps to8000 and takes the
swap. At F399,7FFF-0002 is positive. The negative dx sign contributes key8,
and the swap adds key2: ROM map[10] gives direction6 and distance8000;
the switch's angular penalty80 yields8080. E1D3 compares8080-0640=7A40,
rejecting that candidate; E1F6 retains the current actor. The frozen C returns
transfer and moves the assignment to candidate1 instead. The exact original
bytes are recorded in `controlled-extreme-counterexample-v2.json`, and the
7936-byte controlled input is `controlled-extreme.bin`. Neither file is
represented as a natural native capture. Ordinary court reachability of these
coordinates is not established; the current API has no narrower coordinate
precondition or guard.

The retained v1 counterexample prose omitted the negative-x direction key and
said direction2. Its v2 correction is direction6; both give the same two-octant
penalty and selection result. The independently executable576-case arithmetic
guard already expected direction6 and was not changed by this correction.

Independent helper observation tools `human_direction_audit_probe.c` and
`test_human_direction_source_audit.py` check576 full-word boundary pairs,
including8000/7FFF, doubling/sign changes and both equality policies. The
reference uses literal source CMP/BPL arithmetic and asserts the actual ROM
instruction bytes. The frozen helper fails79 pairs; report
`direction-arithmetic.json` retains exact expected direction/distance and C
results. This is a bounded diagnostic, not a production CPU interpreter or
a claim of exhaustive65536-squared coverage.

The second blocker is frozen `verify_human_switch.py`, SHA256
`194604cdec59209eab2d393f06225ac3d6dd36bb2b287c8a592e074869ae6dcc`.
Independent38-case tool `test_human_switch_integrity_audit.py` rejects23 but
accepts15 invalid event views: six word fields at65536, seven entry fields
disagreeing with raw bytes, and all court/frame clocks shifted by100000. The
artifact and C comparison layers are left operational; only the parsed event
view changes in memory, never the original files. The broad24-bit numeric
domain is inappropriate for word metadata. For raw events the Lua fields
must agree with C2(actor label),093E(owner),0936(live),093A(offense),0092
(candidate),00AA(score), and the direction word addressed by090C (or the
capture's explicitFFFF fallback). Clocks need their source runner bounds and
completion relationships, not just monotonicity. Existing source/version,
command/environment, isolation and exact typed C-output guards reject the
independent mutations aimed at those layers.

Evidence lives under `build/switch-audit-v1`, including freeze and reference
rechecks, both fresh native reports, local tests, the controlled source
counterexample, arithmetic report and `independent-integrity-v1/report.json`.
The accepted controller19 and repaired human stage are untouched. A separate
repair freeze and independent rerun are required before switch acceptance;
no normal human enablement follows from this component review.
