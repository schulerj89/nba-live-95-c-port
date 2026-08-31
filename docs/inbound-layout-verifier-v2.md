# Inbound layout verifier revision2

Independent review found nine protocol contradictions accepted by the first
verifier: an extra manifest declaration, float controlled addresses, a wrong
isolation method, an extra isolation declaration, three same-byte source path
aliases, an additional opposite dispatcher destination, and probe stderr.
The original source, captures, verifier,33 local tests and failed independent
report remain unchanged. This is an evidence-validation defect, not an
original-game bug or a new change to the gameplay fix.

The new `verify_inbound_layout_v2.py` requires exact manifest/isolation fields,
integer controlled addresses, the known isolation method and exact executed
source paths. It binds those source entries to the actual copied script,
runner/helper artifacts and portable executable. Exactly one constructor
destination must appear in the recorded PC list, matching the raw input
layout. The probe must return canonical six-word stdout with empty stderr.
All earlier source, artifact, settings, raw-WRAM, CPU, clock and output checks
remain in place; no expected native output changes.

`build/inbound-layout-verifier-v2` passes the same54 words from all nine
accepted captures, all33 local checks and the unchanged nine independent
corruption cases. The source repair and private frozen production binaries
are unchanged from `build/inbound-layout-freeze-v2.json`. Independent revision2
acceptance is pending. The original source-boundary and whole-game limitations
in `inbound-layout-repair.md` still apply; no complete phase or game parity is
claimed by the stricter metadata protocol.
