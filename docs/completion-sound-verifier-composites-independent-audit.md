# Independent sound verifier composite acceptance

Accept the three bounded composites listed below: unchanged source/native
components plus separately frozen verification repairs. Original rejected
verifiers, audit tools and failure reports remain unchanged. This verdict does
not accept a complete sound engine, normal initialization, CPU/SPC visibility,
timer/DSP progression, whole transition phase, Rules reentry or production wiring.

| Component | Reviewed revision | Freeze SHA256 | Rehashed identities |
| --- | --- | --- | --- |
| 65816 sound prefix | `sound-prefix-freeze-v2.json` | `e8de65bf75469303be4743f6467313a781342ad40d824da83a3c15dc44ee8bcf` | 45 new, 31 original |
| 65816 sound initializer | `sound-init-freeze-v2.json` | `be1c6e8306634daea8d4b4ed020c6afdc697881b63a0efb133518d05939b729b` | 45 new, 30 original |
| SPC resident slices | `spc-resident-freeze-v3.json` | `54e21b398e5bd22feb422e5167639c9ae4c9722d7378f8ad87a7a05653c21251` | 50 new, 23 original v1 |

Freeze paths are in the scheduler worktree's `.analysis`. All entries above
were independently read and rehashed, not accepted from implementer summaries.
Private copies/builds/evidence are auditor `build/sound-prefix-audit-v2`,
`build/sound-init-audit-v2`, and `build/spc-resident-audit-v3`.

## Verification repair review

The 65816 changes are limited to strict integer-zero process status, text
stdout/stderr with empty stderr, the shared fetch validator and its identity.
The previous native-register, CPU-duration, data-access-position, WRAM,
source/build/capture and pending-response checks remain in force.

`setup_sound_fetch_contract.py` reads the canonical ROM byte at each already
compared instruction PC. A/X immediate lengths use that instruction's PS
widths. Fetches must occupy their specified slots, with exact byte and address;
JSL retains its bank-byte fetch at slot 5. Idles require zero address/value.
Only validated fetches/idles are removed from the native data comparison.
This helper does not execute instructions or supply native responses to C.

Resident v3 additionally requires exactly one final stop. Completed instruction
end markers must be on the final accepted cycle. The terminal $048B/$0622
instruction must have precisely two accepted fetch cycles, phase 2, unchanged
entry registers, and no accepted timer/DSP read. The resident fetch validator
retains CBNE's relative-byte fetch at slot 4 after its DP read and idle.

The new resident evidence helper rejects duplicate JSON keys/non-JSON numeric
constants, checks exact envelope/source/artifact identity shapes, binds actual
archived source paths and exact runner argument sequence, and verifies exact
owned initial settings. Persisted settings may contain application defaults
but must preserve those owned values. All three native state boundaries bind
registers/cycles and directional latches to captured records, require the
bounded speed-zero/write-enabled hardware precondition, and reject a captured
$F0 write changing it. Initialization tags and CPU-port hook addresses/scope
are now checked. No hardware behavior was implemented to make these pass.

## Independent execution and meaningful rejection tests

Built each component afresh from its frozen private source with `/W4 /WX`.
No shared executable or object was reused.

| Component | Fresh unchanged differential | Independent malformed cases | Additional checks |
| --- | --- | --- | --- |
| Prefix | 46 calls, 3,673 instruction states, 2,392 data-access positions, 46 full WRAM endpoints | Original unchanged 9/9 reject | 9 original local tests; 10 parsed corruptions reject; pending-response independence passes |
| Sound initializer | 5 calls, 7,055 states, 2,450 data-access positions, 5 full WRAM endpoints; 1,264 ROM-to-ARAM bytes | Original unchanged 9/9 reject | 11 original local tests; 10 parsed corruptions reject; pending-response independence passes |
| Resident | 16 slices, 182 states, 175 attributed accesses | Original unchanged expanded 24/24 reject | New path-resolving revision of all 22 original regression cases passes |

All seven additional fetch tests pass, including both A/X widths, JSL/CBNE
nonconsecutive operand slots, fetch byte/address corruption and idle fields.
The original prefix/init unit scripts still import their frozen v1 verifier;
those runs check their unchanged common source/identity contracts. The actual
new v2 end-to-end verifiers are exercised separately by both parsed mutation
suites and the fresh complete differentials above.

Independently compared every generated trace and endpoint with this auditor's
previous successful source replay: prefix 92 files, initializer 10, resident
32 are byte-identical. `build/sound-composite-recheck.json` records this and
the unchanged original freeze identities. These outputs are generated C work,
not modified expected native fixtures.

Resident's original tool has no transitive import-path setup because v1 had
no helper imports. The first v3 audit invocation failed to import the new
helper; that empty output directory is retained. The unchanged tool succeeds
with the documented PYTHONPATH pointing at the private frozen tools directory;
the 24-case final result is `independent-protocol-v2/report.json`. No test logic
or native bytes was modified. Resident's revised local test also resolves
input paths before mutation matching, fixing the earlier unreachable-relative-
path test failure without changing the tested behavior.

## Preserved source limits and quirks

The preceding independent source reports remain applicable and immutable:
`completion-sound-prefix-independent-audit.md`,
`completion-sound-initialization-independent-audit.md`, and
`completion-spc-resident-independent-audit.md`.

Prefix stops before 44 $AAE6 idle-port reads or at two unimplemented $A2CE
sequencer entries. Initializer stops before $AACD's first idle-port data read;
its upload check proves byte provenance, not upload execution/timing. The
native caller states satisfy the declared bounded width/decimal/direct-page
preconditions; this is not a generic 65816 runtime API.

Resident's 175 accesses are the explicitly attributed ports, CALL stack and
uploaded-source/table subset of Mesen's mixed SPC/DSP callback stream, not a
complete SPC bus trace. Snapshots supply isolated prestates; the component
does not establish them through normal startup. Its visible input publication
still requires a future cross-clock owner. Pending timer/DSP responses remain
unconsumed, and output latches remain separate from CPU inputs while preserving
source ARAM writes.

Original source quirks are retained: prefix byte/word wrapping and sentinel
handling, initializer byte-underflow/full-word-copy/descending clear behavior,
and resident command/voice wrapping and read-before-write effects. The static
initializer $08FF clear omission remains source-only evidence outside the
resident component, not a newly claimed dynamic witness. No arithmetic helper
or original-game bug was normalized by these verification repairs.

## Repaired tool identities

| File | SHA256 |
| --- | --- |
| `verify_setup_sound_prefix_v2.py` | `eccdff26dc284dbda12acba952c1a7d1144836ea8d86371736efdce15ae34c9e` |
| `verify_setup_sound_init_v2.py` | `dd03b7527564cfe9d56773c6ad03cef833a2ca0397ced6695cfa409f6d30d751` |
| `setup_sound_fetch_contract.py` | `d98730346ff4cdb57aefa1560321155df896b0955b3bab964393814cd428dbdc` |
| `verify_setup_spc_resident_v3.py` | `cc075845a94c1b04722fca51852b615246eb7b6c2a30926b0aae5eabcfadb344` |
| `setup_spc_evidence_contract_v3.py` | `ab36254a47b0eb8af8f826d8b1f2cfb5bd716fe2e7ff81735a6f197cec5a3363` |
| `setup_spc_fetch_contract.py` | `ef0831ee2267b2c2aca24639365685875d93ec189b29c1c061d3899de3c964a6` |
