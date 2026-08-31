# Pass evidence verifier repair, version 2

This is a verifier-only replacement in new files. The original pass11 module,
probe, captures, verifier and freeze remain byte-identical, as does the later
AB2D11 checkpoint. No native fixture, C behavior, source manifest or human-play
enablement changes. `tools/verify_human_pass_v2.py` is the proposed replacement;
root owns integration after independent acceptance.

The independent pass audit found that the original verifier accepted 15 of 39
mutations despite the natural routine comparisons passing. The gaps were
out-of-range 16-bit event words, contradictions between event metadata and its
raw snapshot, and uniform clock values beyond capture limits. The original
rejection report is retained in
`build/human-pass-verifier-v2/original-independent-rejection.json`.

Version2 adds exact source-identity fields; 16-bit word bounds separate from
PC, frame and index bounds; raw agreement for actor, owner, live state, offense,
candidate, score and direction; and bounded, internally consistent clocks.
Its exact initial clock anchors belong to the existing supported capture and
runner versions: Player entry2697, court entry4590 for selection0 and4390 for
selection2. It rejects even a uniform one-frame offset, and does not claim to
support other input routes or script versions. Existing strict settings/home/
saves, command/environment, artifact, source-version and complete typed C
response checks remain in place.

The unchanged original captures still pass: **25 DF7A calls, 41 F1C1 calls,
42766 compared values**. The C stdout for each route is byte-identical to its
original frozen verification output. No C rebuild or expected-value refresh
was needed. `unchanged-c-responses.json` records both comparisons.

`tools/test_human_pass_evidence_v2.py` rejects 62 metadata, event and C-response
mutations. A byte-identical copy of the independently authored
`test_human_pass_integrity_audit.py` also rejects all39 of its original cases
against this verifier. These tests were run by the implementer; they are not
an independent acceptance decision. Reports are in `mutations-v1/report.json`
and `independent-test-v1/report.json` under the new private build directory.

This repair does not expand gameplay claims. DF7A still stops before AB2D in
the original pass11 probe; its original suffix-search quirk is preserved.
The frozen target-direction dependency also has a known extreme-input port
defect found in the separate switch audit. Root is repairing that dependency
elsewhere. The natural pass captures do not close arbitrary16-bit direction
cases, absent NO_RECEIVER branches, or the remaining human-play pipeline.

Identities:

- Original pass11 freeze SHA256:
  `2e66c85ccfff9353e9588c148dd9aab1079d65c856284af85b29adaaddb7211f`.
- Unchanged original probe SHA256:
  `8b0773fd910df0da327b084cd67a61c1e770ee672796167e00e12eaf71f69b14`.
- New verifier SHA256:
  `3dadecb7be59ecbefec1f76a30c5e89b6fe488d6e07c7f295a31b5b52130ce79`.
- Original ROM SHA256:
  `2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`.

Run `tools/verify_human_pass_v2.py --capture <original selection*-v1> --probe
<original human_pass_probe.exe> --rom <original ROM> --output <new report>`.
The new test uses the same arguments with a new output directory.
`build/human-pass-verifier-v2/freeze-v1.json` binds these three new files,
the original identities, reports, independent test source and patch. No commit
or push was made here.
