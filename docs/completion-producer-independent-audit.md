# Setup producer freeze v1 independent audit

Verdict: **source replay passes; freeze acceptance rejected for verifier integrity**. This is a bounded source-work result, not production scheduler, elapsed-DMA, epoch or Rules display acceptance. No original fixture, frozen implementation or production source was changed during this review.

The reviewed scheduler freeze is `.analysis/producer-freeze-v1.json`, SHA256 `347a7ce0d87f771bc2b854f815f0c9b345b6e5298ac7730e6fe28f0cdb3a1724`. All 36 identities were checked; exact source copies and independent outputs are in the auditor worktree's `build/producer-audit-v1`. Canonical ROM SHA256 is `2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`.

## Source and fresh replay

Fresh private MSVC `/W4 /WX` compilation produced `compiled/setup_producer_work_probe.exe`, SHA256 `3d86fd17bebcd7833682e7bb33968ce0447a5754614b27991b1db762cbf7fe0a`. The producer C is `4f910a659b101d78ab7c80ca7675e1878401b74e03a49940218e800465bf0824`; its generated source program is `c1f1efe9ac82178e78895e3445eaab91edc4a24037c982ffb36239ec6bfef8f8`. Its unchanged FB30 and FB46 dependencies retain their separately accepted source identities.

Independent generation against actual ROM bytes checked 390 static states. Natural trace coverage reaches 351 local states; the remaining 39 are explicitly listed in `source-and-prestate-recheck.json`, not claimed as natural coverage. The six source ranges checked independently include `$80:EC68..EDFC`, fill `$80:8AD2..8B34`, graphics `$80:8BA1..8BCF`, palette `$80:8A02..8A41`, row publication `$80:8CD0..8D98` and transform `$80:EA4B..EA78`. Their individual hashes and four actual native entry operand checks are retained in that report.

The API exposes resumable individual read/write/idle work from `$80:EC68` through the return after `$80:EDF8`. The codec selector advances both unchanged source wrappers only while their next bus events match; it exposes and charges that common prefix once and selects a child at the source format branch. It does not infer a format by fetching ahead. Source loops, word widths, wrapped arithmetic, high-byte-first RMW writes, low-byte-first ordinary writes, separate fixed fills and selector-35 fallback are retained. Queue-wait routes outside this immediate-publication contract return unsupported rather than being silently skipped. Relocation and pending-read continuation checks exercise actual state preservation.

Default operands come from the ROM layout table and its original conversions, not native snapshots. All four actual entry snapshots agree on graphics word 4096, map word 2048, header word 16384 and selector 34. Running with ordinary native register inputs also produces identical work and DMA payloads. Caller preconditions remain native mode, direct page zero, M/X/decimal clear, FastROM, valid stack, mirror addressing, bounded resource streams and immediate publication; this is not a general 65816 implementation.

Fresh `proof-v1/native-validation.json` checks 155,750 native PC/register states and every CPU instruction duration, 40,003 ordered CPU write positions, 27,726 DMA bytes across 73 requests, and all four 6,336-byte scratch results. The body produces 550,552 CPU cycles and 3,493,454 intrinsic master clocks; the actual `$81:D018` caller JSL adds eight CPU cycles and 54 intrinsic master clocks. Four native intervals independently conserve 550,560 CPU cycles after observed interrupt work. All four earlier JSON traces remain byte-identical. The raw native manifest is `a8ff3873d3cc3e756b50017b9fcd2111102de90a4b5bba62b3b96a7edee38eae`.

The 155,677 intervals without DMA conserve intrinsic work after observed interrupt work and refresh quanta. The 73 DMA service intervals have no elapsed-time prediction. The probe applies a request's payload immediately for byte checking; production still needs pending service, alignment, transfer progress, refresh, interrupt eligibility, audio and forward phase. Native DMA begins at trigger CPU cycle +2 under the following instruction's PC. No timing offsets or loaded epoch constants were accepted.

Thirteen local Python tests, eleven C continuation cases and the existing nine corruption cases pass. These successes do not cover the independent native-record defects below.

## Required verifier repairs

Frozen `tools/verify_setup_producer_work.py`, SHA256 `af4bc9bb0e055c208743f87e2d1c8458021ce2566916d34cd2703c4d46dffcb6`, accepts all five invalid cases in `independent-native-integrity/report.json`. The independent tool is `tools/test_setup_producer_native_integrity_audit.py`, SHA256 `f24ef59f1359fee36f86315065e1ca94af44f60822fa5cd8abf986ca323959e6`. It mutates only parsed JSON views after the genuine identity checks, confirms mutation reachability, and runs the full verifier with a fresh probe for every case. Original files remain unchanged.

| Invalid native view | Concrete change | Frozen result |
| --- | --- | --- |
| DMA pair before scope | Events 33/34 master clocks become 0/4, retaining the four-clock pair gap | Accepted |
| CPU write before scope | Event 0 master clock becomes 0 | Accepted |
| DMA source PC impossible | Event 33 PC becomes zero | Accepted |
| Instruction chronology reversed | Residual instruction rows 1/2 swapped; ordinal fields renumbered | Accepted |
| Mixed bus chronology reversed | Bus rows 0/1 swapped; event ordinals renumbered | Accepted |

`producer_records` at lines 165–196 checks types, ordinal fields and scope labels but not native instruction or mixed bus chronology, nor each record's enclosing clock interval. Line 261 sorts merged instructions/writes by CPU count and thereby hides a reversed input stream. The DMA comparison at lines 301–315 ties values and CPU positions to requests, but ignores native DMA PCs and accepts any master timestamps preserving the four-clock read/write gap.

Before acceptance, reject chronological reversals before merging/sorting; bind all native events to the actual enclosing source scope/instruction using both clock domains; and verify DMA PCs against the following instruction identified by the request/service boundary. Check CPU writes and DMA records independently. These checks concern the consistency of recorded evidence and do not require predicting DMA elapsed service time. Preserve this freeze and its failed mutation report when creating a repaired verifier revision.

No original game bug was repaired by this audit. Source quirks remain source behavior. The failures above belong to the host verification protocol, and must not be described as native quirks or dismissed by the passing natural replay.
