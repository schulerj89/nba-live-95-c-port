# Header pre-wait freeze v1 independent audit

Verdict: **bounded source replay passes; freeze acceptance rejected for native-trace integrity**. No frozen implementation, original native fixture or production source was modified. Epoch/wait execution, production integration and elapsed-time scheduling remain outside this result.

The scheduler freeze `.analysis/header-freeze-v1.json` is SHA256 `32e2b7e01af3e5780a4246de2931a1e45cc4f021a65c219f6efa01c95acc5a6a`. All 32 identities were checked and the source copied exactly into auditor `build/header-audit-v1/source`. A fresh private MSVC `/W4 /WX` build produced `compiled/setup_header_work_probe.exe`, SHA256 `78aefaa62e7ca8a778940a85789be9955d631b72752a237d6e4574724ce81418`. Source C is `895b7c8798eade6389d52eba821e102f83949fea5fe2bd479fe6a4ab95757594`; generated program is `4df1b167d71c14fba101d2ba5e31746681423b58b0426d83bc354bb9a948536f`.

## ROM and source boundary

Fresh generation checks all 118 static states against canonical ROM SHA256 `2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870` and the pinned disassembler. Actual ROM `$80:EEC6..EF19` is 84 bytes, SHA256 `762dd6415634de0923e9440f87ed1f7331475cbe28e58df53c0ad4da3a2dfbef`; fallback `$80:EF8E..EF93` is six bytes, `fc66680b0791d2d3b00f64e34646b368734dc973e4bb42a45d158e315c9ccd95`. The fill and palette helpers independently match their previously reviewed ROM hashes. `source-and-prestate-recheck.json` records all four ranges and the eight static states not executed naturally, including the deliberate stop label `$80:EF1A`.

The source preserves the five descending word clears at `$80:EECE` (ten byte writes, low then high within each word), two separate fixed-source 4096-byte fill requests, the full-word unsigned selector comparison against 35, original selector fallback to zero, forced blank `$8F` at `$80:EF02`, the source low-word palette pointer addition of `$72`, and seven palette words at CGRAM index `$59`. The source's pointer addition is not normalized into a generic far-pointer increment. The component finishes on entry to `$80:EF1A`; it emits no instruction or bus work for that wait JSL and does not read an epoch.

All four native entry snapshots independently show fill base 12288, selector 34 and publication byte `$8F`, matching the ROM table's ordinary caller operands. The source proof uses zeroed diagnostic memory plus ROM-derived layout inputs; no native snapshot or native clock drives the C component. Its caller preconditions remain native mode, DP zero, M/X/decimal clear, FastROM, valid stack and mirror addressing with immediate publication. The queued path returns unsupported, and the bank-fixing helper branches are not claimed as naturally reached coverage under these preconditions. The API supports relocation at each bus boundary and repeated peek without consumption.

## Fresh results and limits

`proof-v1/native-validation.json` passes 126 exact native instruction/register states and all CPU durations, 66 ordered CPU write positions, 8206 DMA bytes across the two fills and palette request, and four independent 440-CPU intervals. Intrinsic source work is 2764 master clocks. There are 110 unique naturally executed local states; static generation coverage is broader and is kept separate.

The 123 non-DMA instruction intervals conserve intrinsic work after observed interrupt work and refresh quanta. The three intervals with DMA service do not predict elapsed service or alignment. Four hardware NMI stack writes are positively identified by source PC, stack address, pushed return/status bytes and CPU position before exclusion from source writes. Raw read observations are retained, but full read timing is not an accepted claim. The exact new native manifest is `1670fdb5a688f738be33b8f0c17b4d20a926a410957fd73322743f7f2debd1cf`; all seven preceding scheduler/codec/producer JSON traces remain byte-identical.

Fresh thirteen Python tests, eleven C continuation cases and the existing nine trace corruption cases pass. A first invocation of the existing mutation tool used its verifier's option spelling `--previous-native`; argparse rejected that invocation before running. The corrected `--previous` invocation passes all nine and is recorded in `local-mutations/report.json`.

## Native verifier defects

The frozen verifier `tools/verify_setup_header_work.py` is SHA256 `1dd3ccb96c9b7a5bf4139bcb6cfa5b554a21c4a35ae07b68cf7690137c294214`. Independent `tools/test_setup_header_native_integrity_audit.py`, SHA256 `24504c6667abf9ac340d6311dcb16234527e6ac5606003766b956be884e3d4a7`, runs a genuine baseline and five full-verifier mutations after real identity checks. Mutations affect only parsed native views, with reachability asserted; original files remain unchanged.

| Mutated native view | Frozen result |
| --- | --- |
| DMA events 40/41 moved before scope to master clocks 0/4 | Accepted |
| First CPU write master clock changed to zero | Accepted |
| First DMA read source PC changed to zero | Accepted |
| Instruction rows 1/2 swapped with renumbered ordinals | Rejected by register sequence comparison |
| First two bus rows swapped with renumbered ordinals | Rejected by CPU write sequence comparison |

`native_records` at lines 165–193 checks field types and ordinals without clock order, scope bounds or PC-to-instruction membership. The DMA loop at lines 277–294 associates request CPU positions, byte values and four-clock read/write gaps, but ignores the DMA PC and enclosing master-clock interval. Reject the three accepted invalid views and validate original-order native chronology, scope and source-instruction association explicitly. This does not require modeling DMA elapsed time. Preserve v1 and its failed report when introducing a repaired verifier revision.

No original game bug was altered. Source quirks and the bounded API limitations remain visible; the rejected behavior is a host evidence-validation defect. Forward master phase, DMA service, refresh/NMI/audio and the subsequent epoch wait still require their own implementation and acceptance.
