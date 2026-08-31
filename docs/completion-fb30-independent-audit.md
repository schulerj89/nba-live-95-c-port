# Independent bounded FB30 acceptance

2026-08-31. **PASS for the frozen FB30 component and its bounded verification.** No production scheduler, complete backdrop, forward NMI/audio/SPC phase, repeated Rules timing, or whole-transition parity is accepted here. The separate accepted FB46 source/API and original failed verifier v1 remain unchanged.

Owner freeze: `completion-scheduler/.analysis/fb30-freeze-v1.json`, SHA-256 `a8698a5648188392c0c9ff82176516174c712b509faae0310e02a085c2c82ad6`. All 24 frozen identities were independently rehashed. Auditor source/build/results are in `completion-auditor/build/fb30-audit-v1`.

| Artifact | SHA-256 |
|---|---|
| FB30 C module | `21232110c709df3fa0b3c4c52610dd80b44974951769d92c0a31a88e1cc340ec` |
| FB30 header | `17f952a70f8559263c8cee56258c91921245564719132376bc23f027459851f8` |
| Generated static C program | `4027a3cb00dfa5ce7f81a5e46349635f36a05d6f5b9682bd55c6864b93483891` |
| Source generator | `cc93d509972c3b56f7685bc29028503cf92f6517ba4f32a9c3338fb3ffab1862` |
| Pinned static decoder | `b1864664d3ac0abcd439055e88bf7220cf66be7a6ae9dfc5d59b3186f1469a46` |
| Native verifier | `6e10523a1eabc78f0bac1d12ff174222c3f5743979bac1d9ec39234699598b90` |
| Independent semantic witness | `90b3b5882bab73328a0551facaa0c0625bea937e820bda3cbce8ec6f6b038c77` |
| Fresh independent probe | `e25b754467126defa71b313432aeffac4c800081031a7512977c9bff438534cc` |
| Original ROM | `2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870` |
| Unchanged native codec manifest | `392e653f348441a2e80bb2f8f355b37a284fa34c58c3bf261418ce51dd05b52f` |

The full source and probe were freshly rebuilt with `/W4 /WX` in a new auditor directory, without game objects or shared build outputs. Regeneration from original ROM plus the frozen static decoder produces the exact versioned C program: 892 source states. The generator accepts no native capture input. Inspection confirms runtime code uses compiled source labels and explicit bounded jump-table targets; it does not decode opcodes or consume an instruction trace at runtime.

Fresh native verification passes **36,418 first-call instruction-entry PC/A/X/Y/SP/DB/P states and CPU durations, 9,935 ordered CPU writes and their positions, and four complete payloads**. All 7,102 previous scheduler records remain unchanged. The `$AE:A0AF` resource produces 960 bytes in 115,554 CPU cycles and 736,896 intrinsic master clocks. The payload SHA-256 is `0b5c2dafc1a19c277807c8d7c81425e8fb0c23623f694ee9574452f1492b3bd7`.

There are **676 distinct native first-call states**, leaving **216 of the 892 static states without a natural first-call witness**; `state-coverage.json` makes that distinction explicit. Static regeneration is not evidence that all 892 states were naturally executed. Synthetic depth-one-through-fifteen tests exercise additional paths but are not original-game journeys.

The semantic witness was independently rerun against unchanged raw native exits. It matches the payload, canonical symbols/counts/offsets/thresholds, both fast tables, and terminating cursor/prefetch state for calls 1/6/11/16. It is a separate byte/codebook calculation without timing. Its 719-byte input window has SHA-256 `c8e1e58135576ccb0f9b4de0d60d48565a9ecaf528c4dd53c70f06b8b19af340`.

The auditor also freshly compiled `tools/setup_fb30_endpoint_audit.c` and matched all four C682 endpoints' A/X/Y/SP/DB/P fields (24 fields). Only ROM, source operand and typed native entry registers/empty cursor are supplied to C; expected exits and native WRAM are not. See `endpoint-and-quirk-source-v2.json`. That report corrects an earlier auditor-only disassembly diagnostic that started at a misaligned address and omitted display-width metadata; the native endpoint results were unaffected, and the earlier diagnostic remains preserved.

Source review covered width-aware accumulator/index updates, ADC/SBC flags, indexed idle conditions, DP-indirect pointer bytes latched at their actual read cycles, relocatable pending state, ordinary low-byte-first stores versus native high-byte-first word RMW, stack/return handling, and instruction-limit exits. Supported bus recipes fit the shared ten-cycle maximum. No additional source defect was found within the stated caller preconditions.

The original quirks are retained and commented:

- `$80:C44D LDA $1E; C44F BMI C467` exits without consuming the terminating bit. Native cursor bit 5740, prefetch `$A37E`, buffer `$8300` remain exact.
- `$80:BEB1/BEB3` tests the count; `$BEB5 STZ $0540,X` preserves a zero threshold for zero-count slots.
- With a synthetic depth-16 tree, `$BEBE LDA $14` yields zero; `$BEC0 CLC` clears carry, `$BEC1 BEQ` skips shifts, and `$BECD BCC` continues construction. The work-limit outcome is retained, not mathematically normalized. This is source-derived synthetic behavior, not a claim that the original depth-ten resource triggers the edge.

All 18 owner Python tests and 14 C continuation cases pass independently. These include pending reads, pointer latching, word-RMW order, relocation, both empty-queue paths, malformed protocols and explicit unsupported states. The auditor's separate `test_setup_fb30_work_audit.py` (SHA `de5b80b2aa417edbe24f51e21aef393be3aafa1a98ab2683f0f9381b9705d24a`) freshly reruns the actual probe for six in-memory C trace corruptions. All are rejected: backward intrinsic time, mixed-row reordering, register/instruction-cycle corruption and write-cycle/value corruption. Its baseline remains valid. Original native fixtures are never mutated.

The verifier uses the accepted strict codec trace contract and exact capture script/base/runner pins. Expected raw exits are consumed only by verification. NMI hardware writes and induced `$2180` WRAM effects are positively checked before exclusion; FB30 correctly separates the first 152 symbol-table writes to `$7E:0100` from the subsequent payload writes to `$7F:2000`.

Limits remain explicit: native mode, DP zero, M/X/decimal clear on entry, FastROM, valid live stack/mirrors, noncrossing source stream and empty immediate queue. The public API is a bounded source component, not a general processor or full arbitrary-format decoder. The semantic witness's legacy native reader is included in the freeze; full acceptance here uses the stricter FB30 verifier, not the witness alone. Recorded NMI intervals and refresh residues provide conservation checks only. No production manifest/caller enabling was present in the reviewed scheduler worktree.
