# Independent nested return component audit

Accepted for the typed saved-word restoration and independently checked native
calling convention. No normal human play, complete initializer, launch,
later `$87:91C3` instruction, generic stack/CPU implementation, interruption
behavior or whole-game phase is accepted.

Controller `build/human-pass-return/freeze-v1.json` SHA256
`02e50df9d3b39bb09fcd641bb2530fd082d4339f2dd3fc4f5dd5193c917cff9c` and all
277 declared source/dependency/object/earlier-freeze/external/artifact
identities independently rehash. Private outputs are auditor
`build/pass-return-audit-v1`. All original failed captures and verifier
attempts remain unchanged.

## Source and caller review

Read actual ROM bytes, the frozen Ghidra/recompiler references, the new C
body/header, probe, capture/runner and verifier. The source saves B8/B6/BC/BA/
C0/BE/9C/9A at both `$86:AB2D-AB3B` and `$84:DF7A-DF88`. The respective
AF4D-AF63 and E09C-E0B2 returns restore them in reverse push order. These are
distinct caller-entry frames. E2E8 jumps directly to E3E6, which restores
only the human caller's B6 saved at E2AC. The original B-button mask in the
selector must not replace that outer value; this is commented in C.

The module saves/restores typed words and composes these operations, with no
opcode, native endpoint or global-initializer shortcut. A future caller must
retain its separate saved frames across the bodies; this does not supply a
general aliasing/lifetime or hardware-stack abstraction. No production caller
or human enable flag is modified in this checkpoint.

The capture buffers the actual E2AC prestate when the incoming B bit is set,
then publishes that exact buffer only when native gates enter DF7A. It does
not reconstruct the human frame from a later selector state. Ordinary input,
private process/settings/home/saves, original sources and route arguments are
attested. The probe receives the current segment prestate plus the earlier
initializer/selector/human caller files; no expected endpoint is an input.

## Fresh and independent checks

Built only the frozen new C module and probe with `/W4 /WX /O2 /MD`, using
the frozen type header. No old object, asset or shared executable is linked.
Fresh probe SHA256:
`1cc38da125da5326b481561b85f1b2eae1aa92af9d4493318137411841973fee`.

The new probe matches 121,024 left plus 68,076 right values = 189,100, with
all 25 calls checking initializer/selector/human/combined stages separately.
Both stdout files are byte-identical to the original frozen successful run;
stderr is empty. Every declared output row, saved-frame word, vector and
numeric type remains checked.

An independent diagnostic (`test_human_return_rom_audit.py`) reads the actual
ROM and executes only PEI/PLA/STA/JMP/RTL for these bounded source ranges.
Unlike the implementation or its verifier formulas, this diagnostic walks the
original bytes against the captured physical stack and full sparse memory.
It has no cycle, interrupt, generic CPU or production execution claim.

All 55 source PCs exercised by the 25 calls agree:

- 75 caller save frames match the later actual stack contents and depths.
- 175 return segments (seven per call) match every captured A/X/Y/PS/D/SP/
  DBR/program-bank/PC value and all 13,824 sparse raw bytes per segment:
  2,419,200 byte comparisons, including unchanged physical stack memory.
- The three RTL targets/depths and direct E2E8 jump are reproduced. The C
  slice stops before E3E9; actual native RTL to 8791C3 is separately observed.
- The original selector crossing court1471->1472 remains exactly recorded;
  the bounded return segments themselves remain within one frame.
- Restored outer B6 is negative in eight calls and positive in seventeen;
  no naturally restored zero outer B6 or no-receiver selector-only call is
  claimed.

The native supported domain is D=0, 16-bit A/X, DP0, DBR7E and complete
low-WRAM stack frames. The return instructions are not decimal-sensitive,
but D=0 remains an explicit inherited route precondition. No arbitrary
DBR/DP/emulation/out-of-WRAM/interrupt case is generalized from these captures.

## Evidence integrity

The strict verifier checks exact identity/schema/settings/route/environment,
raw metadata and independent stack-preview/raw agreement. Every saved stack
word is tied to its correct earlier caller; both 16-byte pops, the 2-byte
outer pop, three RTL addresses/depths, final PLA accumulator/N/Z, and unchanged
register/memory fields are enforced. Frame-crossing relaxation applies only
before the bounded return, so the retained original crossing is not shifted.
The isolation helper runs on a copy and cannot silently overwrite attestation.

All 112 frozen mutations reject in the independent worktree, including the
17 saved-frame word changes. An additional independent 25-case suite rejects
short/missing/extra/duplicate output, bool/high words, noninteger status,
any nonempty stderr and the excluded decimal domain at every actual return
tag. This probe's successful stderr is exactly empty; it never invents a
loader diagnostic or acquires an asset dependency.

| File | SHA256 |
| --- | --- |
| `src/nba_human_pass_return.c` | `01189fb42a5db97f546e28b7c8dc82cdf9a7133bf8c78e62e501f7d48a19f1cc` |
| `include/nba_human_pass_return.h` | `dbd68c12a880b15769e35ddb92d3aaf39b62f74eb50d757a104bbb4e42cca48b` |
| `verify_human_pass_return.py` | `0bb4af5b5460bfda9300faf3220c7d964c28661b9b237b38a8b1dca5c314f98e` |
| Independent ROM diagnostic | `d0bf2df8ed6ac71604599aef6b2300de2b521777feafe20066c73bd072c39ed2` |
| Independent protocol tool | `41842e94a50e50975e9583b486f30bc24ab85bfd74cd2a5fb730f9623db2adae` |

No original bug, frame crossing, saved pointer or raw fixture is normalized.
This closes the bounded saved-state return contract, not the skipped bodies,
future caller integration, subsequent mode-15 launch or whole human flow.
