# Independent FB46 source-work audit

2026-08-31. **Source component passes the bounded review; codec freeze v1 verification acceptance is withheld for two demonstrated protocol gaps.** Neither finding changes original native evidence or identifies a bug in the current C continuation. A separate verifier repair is pending. This component is not accepted as a production scheduler, Rules reentry fix, or forward timing prediction.

Owner: `.analysis/worktrees/completion-scheduler`. Auditor: `.analysis/worktrees/completion-auditor/build/codec-audit-v1`.

## Source and executable identity

| Artifact | SHA-256 |
|---|---|
| Owner `.analysis/codec-freeze-v1.json` | `86fb1fcd1e7ca677b56c485b97dcd38758ea7c76ad626ecd07be723a402bfd6f` |
| `src/nba_setup_codec_work.c` | `4787f419083d6588eaf93b1435d874ffd82b32b9a188838cbf3254599afc1a50` |
| `include/nba_setup_codec_work.h` | `fda380a291f4d9a6b69983399f7931ad4ccd5d8ccc507852bdc9cc6f155813cf` |
| Frozen probe source | `a667e1309752c77b100955c5926c7dd3b441bfa3f94dbef5175b691fce60cfdb` |
| Frozen verifier | `627959f76e3b69bb6599b63116e47013777d38abfaf7da8de9affe1b25347dd9` |
| Imported accepted scheduler verifier | `be33e53c5712b491eeed3e506e233106ac700b5b8996282c252a44ebc268eaea` |
| Fresh independent probe | `457f74287a82089221d1f4f31ebecbd46200c0be5e633bf2d625b0c878b04c86` |
| Original ROM | `2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870` |
| Native codec manifest | `392e653f348441a2e80bb2f8f355b37a284fa34c58c3bf261418ce51dd05b52f` |

All 15 frozen identities were checked before snapshotting the nine source/doc/tool files into an independent directory. The unchanged module/probe were fully rebuilt there using the frozen `/W4 /WX` build script, with no game objects or owner object reuse. The original ROM bytes, source hash, native artifacts and original owner reports were not changed.

The four retained Mesen source references were read and rehashed against `.analysis/codec-source-v1/manifest.json` (`5eeae397cdb3733381b1568e9e6419ecba75b29d238b0c36767232cfffa89590`). Direct source inspection covered JSL fetch/push order, JSR/RTS/RTL work, native RMW idle behavior, 16-bit-index extra cycles, and FastROM/mirror bus costs. The original ROM independently matches 200 C macro source sites' opcode classes and the wrapper/FB46 routine byte ranges. These are additional source checks, not a claim that opcode-class checks alone prove translation. `independent-source-and-raw.json` records them and confirms all 40 raw entry/exit snapshots agree with captured source/destination/mode/head/tail fields.

## Fresh observations

The fresh independent verifier run passes four input-driven source checksums, 112,814 native instruction-entry PC/A/X/Y/SP/DB/P states, every corresponding CPU instruction duration, 28,218 ordered CPU writes with their cycle positions, and 16 complete native FB46 payloads. All 7,102 earlier scheduler events remain unchanged. Native input contains 20 codec calls, including four FB30 calls that are retained but outside this implementation.

| Source | Bytes output | CPU cycles | Intrinsic master clocks |
|---|---:|---:|---:|
| `$AE:C446` | 960 | 34826 | 218142 |
| `$AE:D153` | 2054 | 94738 | 602298 |
| `$A6:C5FC` | 6336 | 246864 | 1556550 |
| `$AF:97AA` | 838 | 32091 | 202290 |

The auditor additionally compiled `tools/setup_codec_endpoint_audit.c` to project the actual C682 endpoint registers. All 16 native FB46 calls match the six A/X/Y/SP/DB/P endpoint words/bytes (96 fields), including the final PLP effect beyond the existing instruction-entry comparison. C receives only original ROM, source operand and typed native entry registers/empty queue cursor; it does not receive native WRAM or expected exits. `endpoint-comparison.json` records the results.

The 13 owner integrity tests and 12 C continuation contract cases pass on the fresh binary. Inspection confirms these include live pending data, RMW continuation copying, synthetic dictionary/literal/escape input, stack/status preservation, both empty-queue branches, unsupported formats/queue states and instruction limits. They are controlled source tests, not claimed native executions of every synthetic state.

## Two verifier defects

`tools/test_setup_codec_work_audit.py`, SHA `a7e470e6ce9c53f3601aa2abbe8aef7151b8e8f9f300510a6508a124a5f30afe`, freshly runs the built probe for a baseline and each of six independent trace mutations. Only the Python view of generated C JSON is changed. Native files, original ROM and the generated trace files themselves remain unchanged. Results are in `independent-integrity-v1/report.json`.

1. **Backward intrinsic time accepted.** For `$AE:C446`, C instruction 233 at `$80:BD24`, changing cumulative master clock 6570 to 6530 makes the preceding intrinsic interval **-28**. The following native instruction includes a 40-clock refresh, allowing the existing conservation checks to absorb the compensation. The entire validator still prints PASS. This is an impossible C time series, regardless of whether refresh is forward-predicted. Reject nonmonotonic source clocks and require each instruction's intrinsic clocks to satisfy its actual covered 6/8-clock bus domain relative to CPU duration, including initial/final endpoints.
2. **Mixed-event order accepted.** Swapping a CPU write row and its following instruction row leaves the split instruction and write lists unchanged, so the validator accepts an out-of-order original C JSON stream. Require chronology of the complete mixed stream, not only the separately filtered lists.

The same independent tool confirms corrupt register state, instruction CPU cycle, write CPU cycle and write value are rejected. Thus current comparisons have real sensitivity, but the two missing guards must be repaired before accepting the verification checkpoint. The actual unmodified source run is ordered and has positive native-matching costs; these are verification defects, not native game quirks.

## Source fidelity and bounds

The C continuation translates the fixed original source flow from `$80:C62B` entry to `$80:C682` entry, excluding final RTL. It covers the empty `$86DA` helper path and FB46 `$BD1B..BE6A`. The source's byte type table, signed/zero branch distinctions, hand-unrolled tree, recursive helper and stack writes are retained. The API resolves operand reads at the exposed bus access and commits local instruction effects at completion. No captured clock, visit count, output bytes or event script drive the component; the C resume labels are source-local continuation labels. The probe's resource totals are verification checksums, not source-module dispatch constants.

Source-specific oddities, including unconditional 16-bit-index read idles and the native RMW idle rather than a dummy write, are retained/commented. No confirmed original game bug was normalized. No additional C source defect was found within the stated input domain.

Caller preconditions matter: native mode, clear decimal/M/X flags, direct page zero, FastROM, supported WRAM/IO mirrors, the covered non-bank-crossing ROM streams, valid live stack, and an empty immediate queue. Several are caller requirements rather than generic runtime validation. This is not a general processor or arbitrary-address codec interface. In particular, the indexed address helper wraps at 16 bits and is justified only by the non-crossing stream scope. Unsupported formats/nonempty queues and work exhaustion stop explicitly.

NMI removal is a validated observation convention: four exact hardware stack writes are checked before exclusion, and the `$2180` induced WRAM callback is checked against the corresponding CPU write, address sequence, value, time and event order. Nevertheless master-clock residuals are **conservation using recorded NMI intervals and refresh quanta**, not a forward interrupt/refresh schedule. FB30, audio/SPC, other backdrop/helper work and carried hardware phase remain unimplemented here. No production caller is wired to this module.

The imported scheduler-verifier dependency is recorded in generated proof identities and covered by its previously accepted queue freeze, but is not directly one of codec v1's 15 files. Capture scripts/runners are checked against their declared actual hashes and externally reviewed frozen manifests, not a hardcoded codec revision allowlist. These are external attestation limits, not proof that arbitrary replacement scripts implement the claimed route. A subsequent evidence freeze should include the dependency closure and retain the actual reviewed capture identities; no change to original fixtures is necessary.
