# Accepted reset/upload/F1 component checkpoint

Current verification uses [the additive v4 boundary repair](bootstrap-boundary-v4-integration.md).
The original v3 reader below is retained as historical evidence; subsequent QA
found terminal-register and stdout gaps that v4 closes without changing execution.

The source component and repaired verifier are accepted through the explicit
CPU80:80BC stop before8A57. Root copied the27 reviewed files and rebuilt and
verified them in `build/bootstrap-accepted-root-v3`. This is a standalone
component checkpoint: NbaGame and the40-source production manifest are unchanged.
No claim of completed startup, DMA, NMI,03DB, audible audio or Rules timing.

## Provenance and integration

Final source/evidence freeze:
63ee77d19e7720986573365e41169c329a67f1bb0043ab70c84fc324c76f7ad1.
Root rehashed all1,013 direct identities and the exact delivery manifest
a27d8b7ecbd4a93661fb62db0f836f533c4207203fbe8f6c7d70b93aa1cb6b05.
The independent audit is `checkpoint-qa-20260831/build/s1-bootstrap-independent-audit.md`,
SHA256f637c3726adab893add7397bb1cb67dd93cb4ef7fc887f8a252ecb15071efdaa.

Six of nine existing prerequisites differed only in LF/CRLF between root's
older working files and the fresh reviewed checkout. Root verified normalized
content equality, preserved the previous bytes and copied the accepted exact
bytes with explicit -text attributes. This is an encoding-only change, not a
logic change. QA checked the include closure: none of those six files is in the
active40-source game build. Old frozen proofs retain their old identities and
are not relabeled as fresh checks. Root's first runner stopped on a handoff-key
error, then its second stopped on these byte differences; both failed attempts
remain in v1/v2 directories and copied no source. The successful v3 receipt
records all27 additions, nine prerequisites and six ending changes.
All nine prerequisite Git blobs now retain the reviewed physical bytes; three
already matched in the working tree but previously relied on checkout newline
conversion. Their working bytes did not change. The reviewed ROM module's
extra final blank line is retained rather than changing its accepted identity.

## Fresh root results

-Eight-source MSVC /W4 /WX probe and contracts build pass.
-Regeneration from the canonical ROM/decoder matches all143 fixed source
  states. The production component does not interpret arbitrary CPU opcodes.
-The repaired v3 reader matches28,405CPU instruction states,16,259CPU data
  accesses,9,616SPC instruction states,6,594SPC writes/I/O reads,84scalars,
  131,072WRAM bytes and two65,536-byte ARAM snapshots.
-Contracts pass18distinct checks plus65,536 initial-zero ARAM byte checks.
-All21protocol and12profile negative cases reject, with both positive baselines.
-The unchanged independent nine trace/summary and three boundary-metadata
  corruptions reject, with positive baselines. Earlier failed verifiers remain
  historical files; `verify_bootstrap_v3.py` was this checkpoint's entry point.
  The current entry point is now `verify_bootstrap_v4.py` as noted above.

Fresh root probe SHA256:
9c443d76eae40a3413ad8819acb12a5296a8f9d61d2dd78e56bf640f44d843ec.
Trace SHA256 remains245371d44b6bd3ed5954349b95278530b6d14c0286833232c88ba3a1b5a7191c.
It ends at CPU80BC, master719846,95,048CPU cycles, SPC tick68,726/PC038E/phase2,
with1,264uploaded bytes, resident entry and F1 completed, and528refreshes.

## Limits and next interface

The declared software profile is NTSC zero RAM and32000+40Hz SPC. The pinned
Mesen source commit is a source reference, not proof of the installed binary's
build commit. Native lazy-SPC callback master times during later DMA differ
from logical deadlines; comparisons use the correct observed SPC cycles and
CPU port boundaries, not a fitted offset. The first three IPL instructions
predate the observer; hidden staged inputs and complete DSP evolution are not
individually observed. No native state is used to seed this C constructor.

The newer first-fill/DMA component is excluded from this root checkpoint. Its
frozen source plus the new fill-v2 verifier now has bounded independent acceptance;
root integration remains separate. Do not turn the80BC refusal into successful license
entry. The eventual persistent owner must keep one canonical WRAM bus across
scenes: graphics queue0100..02FF, head35/tail37/budget39, RNG07F6 and overlapping
direct-page bytes need borrowed views with endian-safe accessors. A value-based
queue or gameplay helper may use only an exclusive temporary projection with
source-ordered writeback, not a second persistent authoritative copy. This
ownership work and the remaining actual CPU/SPC continuation precede runtime
enablement. The C screenshot gallery is unchanged by this standalone checkpoint.
