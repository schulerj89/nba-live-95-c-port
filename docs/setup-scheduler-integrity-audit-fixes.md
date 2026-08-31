# Scheduler verifier audit corrections

Date: 2026-08-31. This supersedes the evidence-integrity acceptance claims
for primitive freeze-v2. The auditor independently rebuilt the C primitive
and reproduced599 queue dispatches/126 publications and20 waits, but found
concrete verifier/probe defects. These are test-boundary repairs; production
timing, source queue/epoch semantics and all native captures remain unchanged.

## Reproduced defects and repairs

* Capture validation previously hashed only declared entries. Removing
  `scheduler.jsonl` from the artifact table, or omitting script/runner
  source identities, could avoid verification. Required manifest/source/
  artifact sets are now explicit; the complete on-disk capture inventory
  must match, and named source paths must resolve to their actual copied
  files. The supported legacy8-dump and extended12-dump captures retain
  their original identities. Tail captures require all92 NMI snapshots and
  their extra logs/script/sentinel.
* Build validation accepted `sources={}` and numeric booleans. It now
  requires all four exact source entries, their expected relative paths,
  hashes, successful integer compiler exit and integer schema. Capture and
  row numeric fields likewise reject booleans, floats, negatives and values
  outside their actual serialized domains. Known row fields, hook PCs,
  queue alignment and observation ordering are checked.
* Persisted settings were not revalidated by the verifier. Both initial
  and persisted settings now must satisfy the exact typed capture contract;
  the persisted file must match its recorded hash. Private paths, observed
  environment, launch arguments and the stated natural input schedule are
  also checked. This reads private capture settings; it changes none.
* The C probe used `scanf` unsigned conversions and `%2x`. Numeric
  `4294967296` wrapped, `-0` was accepted, and `1g` could become byte1.
  The probe now parses complete bounded decimal tokens and both exact hex
  nibbles. Signs, suffixes, overflow, embedded NUL, wrong line arity and
  wrong hex length fail before invoking a primitive or publishing output.

The original `src/nba_setup_scheduler.c` and header are byte-unchanged.
In particular, caller M width, epoch wrap, palette low-byte testing,
subtraction borrowing and native queue service behavior were not adjusted.

## Verification and source continuity

The new fresh probe lives in `.analysis/scheduler-build-v3/`; the previous
build was not overwritten. Exact freeze-v2 contents are retained at
`.analysis/scheduler-frozen-v2-source/`, including all14 previously frozen
objects and `retained-source-map.json`. The auditor's separate snapshot
also preserves the original six module/probe/verifier files. Freeze-v2
metadata and all historical reports/captures remain immutable; its old
working-file paths are not assertions about the updated working files.

* Fresh native report `.analysis/scheduler-primitives-v5.json`:599 queue
  dispatches,126 DMA publications,12 budget stops,4 M=0 header waits and16
  M=1 waits PASS. All7080 V1 and7102 V2 native observations still match.
* Local `tools/test_setup_scheduler.py`:32 tests PASS, including explicit
  missing identity, empty build sources, numeric domain, settings, source
  path and malformed probe-input rejection tests. Several tests have
  multiple independently checked mutation cases.
* Auditor-authored `test_setup_scheduler_audit.py`, run by the implementer
  against the repaired files:32/32 PASS. The original freeze-v2 result
  was13/32. The new result is
  `.analysis/scheduler-independent-mutations-v3-final.json`; independent
  final rerun/acceptance remains the auditor's responsibility.
* The interrupt attribution was rerun with the repaired capture verifier:
  `.analysis/interrupt-tail-attribution-v2.json` preserves all46 intervals,
  67247 instructions,9044 hardware reads,92 old WRAM snapshots and the
  previously established clock/cycle attribution.

Reproduction from the scheduler worktree:

```powershell
.\tools\build_setup_scheduler_probe.ps1 -OutputDirectory .analysis/scheduler-build-new
python tools/verify_setup_scheduler.py --native .analysis/native-scheduler-v3 --previous-native 'C:/Users/joshs/Projects/nba-live-95-c-port/.analysis/setup-scheduler-20260830/native-v1' --previous-native .analysis/native-scheduler-v2 --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --exe .analysis/scheduler-build-new/setup_scheduler_probe.exe --report .analysis/scheduler-primitives-new.json
python tools/test_setup_scheduler.py --native .analysis/native-scheduler-v3 --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --exe .analysis/scheduler-build-new/setup_scheduler_probe.exe
```

Stop on any unsuccessful command. These checks remain bounded evidence
and engineering integrity checks, not a production transition repair.
Repeated Rules entry, full producer work scheduling and complete game
acceptance are still open. No captured timing or command list was added
to production and no native fixture was rewritten.
