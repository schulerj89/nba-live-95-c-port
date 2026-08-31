# Scheduler primitive checkpoint

The owner accepted the independently reviewed queue and epoch-wait primitives
from scheduler freezev3. Source and provenance-verifier review is recorded in
`completion-scheduler-primitives-independent-audit.md`; earlier rejected
verifier versions and their reports remain preserved.

The owner copied only the checked-in source/probe/capture/documentation files,
verified their freeze hashes, and rebuilt the probe separately. Fresh owner
results in `build/scheduler-owner-v1/` and adjacent logs pass599queue dispatches,
126DMA publications,12budget stops,4word waits and16byte waits. All7080V1 and
7102V2 observations remain unchanged. All32 supplied integrity tests pass;
the auditor independently passed its additional32cases on the same source.

This module is **not wired into production transitions** and is not presented
as fixing Rules reentry. It does not change the executable's production source
manifest, native fixtures, asset pack, primary main or desktop shortcut.
Producer work and audio/SPC acknowledgement continuation are still required;
see `setup-scheduler-consultation-plan.md` for the completed read-only diagnosis
and exact remaining dependency.

The source preserves and comments the original arithmetic and width behavior,
including the palette low-byte test and caller-width epoch comparison. No
observed native quirk was normalized to make the tests pass.
