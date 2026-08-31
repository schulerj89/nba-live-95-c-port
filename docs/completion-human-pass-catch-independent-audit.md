# Independent bounded catch component audit

Accepted: the frozen AD3D catch component and strict verifier for their
documented boundaries. This does not enable human play, establish the whole
AB2D initializer, execute B468, or claim natural coverage of clear lanes,
receiver initialization, extreme coordinates, or both controller sides.

Reviewed controller `build/human-pass-catch/freeze-v1.json`, SHA256
`a167ae67119300e09b2b07fde9a1965d7cd8c4ecf8ee6647a1b17c22e2c646e6`.
All 282 declared source/dependency/object/earlier-freeze/artifact identities
rehash. No original artifact was missing or modified. Private evidence is
auditor `build/pass-catch-audit-v1`.

## Actual source and input boundary

Read the original ROM, fresh frozen Ghidra listings/reference provenance,
the C body/header, probe, verifier, capture and runner. The scope is
`86:AD3D-AE0F`, `80:CEE7-CEFC`, `85:F02D-F099`, `85:F5E4-F727`, and the
unwitnessed `86:AF66-AF82` prefix stopping at the first B468 child entry.

AD3D uses carried receiver Y. AD83 reads **[00]+42**, independently of
ADA9's [E0]+39 profile access. Original source bytes, captured Y=42,
the source pointer/read metadata and sparse raw memory agree: left court
1591/1641 read 00004C/06004C=7E50, and 1760 reads 02004A=7000. The C comment
preserves these actual WRAM-mirror accesses instead of replacing them with a
roster identity. Geometry uses wrapped subtraction/negation, CMP's N flag,
two logical shifts of the minor magnitude and the ROM D04A table.

F02D preserves carried X; it is not F34F. Its first CMP/BPL at F061/F063
does not have F34F's additional equality branch. Both decisions retain
16-bit wrap. The zero vector yields AA=8, B2=0. RNG retains zero->9146 and
the original shift/XOR1D87 contract.

F5E4 uses the source's asymmetric X/Y box and source order cursor. The
forward X miss at F697 switches to the backward scan; the backward X miss
at F6F4 returns clear. Y misses continue. Ball and same-team entries skip;
the original scan order and volatile AA/AE/AC/B2/92 are preserved. Saved
B6/BA/BE/C2/9A/A6 remain unchanged. ADFE temporarily changes 96 to the
receiver for F5E4; AE07 restores it. The bounded typed API does not claim a
complete CPU register/stack state, including the child's carried X.

AF66 retains the raw six-byte band index into AFA6. Band 30 reads AFC4's
opcode bytes A6 8E, producing timer 8ECA after +36. The source comment
distinguishes this from a natural bug witness. The prefix exchanges 96/8E
and stops before B468; it does not apply the later pointer/51 restoration,
mode 14, flags OR4, or any synthetic child result.

The runner uses private settings/home/saves and ordinary released controller
inputs. The capture has no WRAM/ROM/PC injection; observed sources and exact
arguments/environment are attested. The probe consumes only each requested
entry raw file, not its expected endpoint. Unknown/uncaptured bus addresses
fail. The API is bounded to binary arithmetic, native 16-bit A/X and DP=0;
every catch boundary has the explicit D=0 guard. Decimal execution is neither
implemented nor silently normalized.

## Independent source and native checks

Fresh `/W4 /WX` compiled the catch body and probe into private output; the
38 legacy supporting objects are the explicitly frozen, independently hashed
objects. This is not a claim that those legacy dependencies were rebuilt here.
An initial attempt to compile a minimal asset-loader source closure lacked
its transitive intro header and is retained in `compiled/build.log`.
The final probe SHA256 is
`1d8d66535c975359085c10956dc5518ee0d9712245b2b7ecbfca04246ce5dd1f`.

Fresh left replay matches all 31,788 values, with identical stdout to the
frozen final run: three geometry, RNG, direction, lane, attempt and combined
prefix executions. Natural calls occur at court1591/1641/1760 and all stop at
AE10 with obstructed lanes. The right capture attests nine pass calls and
zero catch calls; its report has `comparison_performed:false`, no `passed`
field and no C invocation. This is not right-side catch parity.

The independent `test_human_catch_rom_audit.py` runs a **diagnostic-only**
bounded binary-word executor directly over original ROM instructions, then
compares a separately compiled DLL of the frozen C source. It does not read
C implementation text, captured endpoints or expected C goldens. It models
only the source operations needed for these leaves/children; it has no cycle,
interrupt, hardware, decimal or production CPU claim.

All 70,700 deliberately controlled cases pass across 336 executed source PCs:

- 576 full-word direction and 576 geometry combinations, including equality,
  7FFF/8000 wraps and zero vectors.
- All 65,536 RNG inputs.
- 2,000 varied-coordinate/team/order lane cases, including scan short circuits.
- 1,000 attempt and 1,000 combined geometry/attempt cases.
- Six direct receiver band cases and six explicit clear-lane combined cases,
  the latter reaching the real B468 stop with both pointer exchange and all
  six literal timers checked, including 8ECA.

The extra clear-lane cases use deliberately same-team order entries. They
extend controlled source validation, not the natural capture's coverage.
Initial leaf-only results and subsequent extended results remain separately
retained. A failed relative batch-path invocation preceded the corrected
absolute-path build; it made no source or native change.

## Verifier integrity

The verifier retains exact source/artifact/route/environment, typed metadata,
raw/event, clock and completion checks. It calls the isolation helper on a
copy and compares all recorded observations afterward. Children are required
from source call conditions in exact order, not merely accepted if balanced.
It rejects incomplete comparisons instead of using right-side zero coverage
as a successful C result. Successful stderr must be exactly the asset-loader
line for the SHA-pinned pack argument and process status must be integer zero.

All 93 frozen local mutations reject in the auditor worktree. An additional
independent 24-case suite rejects missing/short/extra output, duplicated leaf
keys, high/bool values, malformed process/diagnostic results and decimal mode
at **every actually observed catch tag**. A positive observation-mode test
replaces subprocess execution with a trap, confirming the right-side report
does not invoke C or acquire a parity `passed` field.

## Identities and retained limits

| File | SHA256 |
| --- | --- |
| `src/nba_human_pass_catch.c` | `bbcf07d70a7f46e832af8cb558b4261b1ef588e6f30c142c415c7d67a21b54f9` |
| `include/nba_human_pass_catch.h` | `f25afc5c98b046bb2d60704c081b84a2b87a78cba4aa5c1913745008692e90a5` |
| `verify_human_pass_catch.py` | `1f3e1cdf2fb286db29733adf5a360dc54dfde66a90618132a6ee538068489c37` |
| Independent ROM diagnostic | `3261f6cd1d7161a8fd7a36be1e5369702921d694fe560af0749ed25bd46e2ccf` |
| Independent protocol tool | `3bd90277a90f7e3cfc00df7fac166181d1683edb4879d66842bf835b32106ff9` |

No original bug or raw fixture was repaired. Source wrapping, literal pointer
access and table-overrun behavior remain commented and preserved. Natural
zero-seed/profile-boundary/extreme/clear-lane/AF66 coverage, full register and
stack parity, B468 continuation, normal human wiring and whole-game phase
remain unaccepted.
