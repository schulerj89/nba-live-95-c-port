# Producer/header verifier revision 2 independent acceptance

**PASS for the bounded source-work components with their repaired native observation protocol.** This accepts neither production scheduler integration nor carried phase, elapsed DMA service, interrupt/audio/SPC continuation, wait epochs or Rules display parity. Earlier rejected verifiers and their failing independent reports remain preserved.

The scheduler producer freeze v2 is SHA256 `0f784ce695df33e3c4dca126e94865b8ce582d13e865e3408c65b7108fe5e39e` (51 identities); header freeze v2 is `0f527db9a72689641ca702dc379314c9a49cb6ce82ba2383db354f4e400790dc` (49 identities). Every identity was independently checked before copying and again after testing. The direct dependency closure includes the new common guard. Exact private copies and outputs are in auditor `build/producer-audit-v2` and `build/header-audit-v2`.

Reviewed verifier identities:

| File | SHA256 |
| --- | --- |
| `verify_setup_producer_work_v2.py` | `c9b10699dc60927acea33d847ed6f94560a5e551ef1796c4cd1df42ab406e6ed` |
| `verify_setup_header_work_v2.py` | `2812e8c91dcf03d7d7f0607a0ae6abbc2aad5713bb54cc48a87fdcae3aed8733` |
| `setup_native_trace_contract.py` | `82cd9ebac4b597e73825bef13c7e7cacb3db6ae2102d77330b1f697f22c9826e` |

The common guard checks original instruction chronology strictly in both CPU/master domains and mixed bus chronology monotonically before composition. The producer validates each original codec/residual list before sorting the merged sequence, then validates both residual mixed bus events and raw codec writes against that full sequence. Both components bind the first instruction's clocks/registers/PC to the scope entry, require all events within the scope, and associate each bus PC with an enclosing source instruction in both clock domains.

Inclusive interval endpoints preserve the actual callback convention: a final data access can share the following instruction-entry clock. Both adjacent intervals are considered only if the PC and both clocks agree. Same-clock induced effects remain allowed. The header retains the observed NMI-entry hook and RTI read gap; positive hardware stack checks remain separate. These are evidence consistency checks, not predictions of interrupt-body or DMA timing. A focused positive test deliberately extends a DMA interval and remains accepted, avoiding an accidental fixed-duration assumption.

Fresh private MSVC `/W4 /WX` builds produced producer probe `500d056dc974c463a61cf3360edc5f4772fb47cd21fe5403592c9358d666a58e` and header probe `90f0fa3662e37ceea8d7d6ae246c4f4c75cbb1976f0de3032cfda26ae2a2c02e`. Source C, generated programs and APIs remain byte-identical to the independently reviewed v1 snapshots. For each component, all eight default/native-entry report, trace, DMA trace and WRAM outputs are byte-identical to its original fresh audit run; `unchanged-source-results.json` records each hash.

Fresh producer verification passes 155,750 native instruction/register states and CPU durations, 40,003 ordered CPU write positions, 27,726 DMA bytes/73 requests, four scratch results and four 550,560-CPU intervals including the caller JSL. Fresh header verification passes 126 instruction/register states and CPU durations, 66 CPU write positions, 8206 DMA bytes/three requests and four 440-CPU intervals. All earlier native JSON invariance gates remain intact.

The unchanged independent five-case tools now reject every case for both components, including all five formerly accepted producer views and all three formerly accepted header views. Each component's existing nine source/payload mutation cases passes. Both thirteen-test Python suites pass against the new verifiers and run their respective eleven C continuation cases. All ten new focused common protocol tests pass, including both clock domains, wrong PC/instruction associations, entry registers, terminal bounds and valid callback endpoints.

Source fidelity, caller preconditions, natural versus static coverage and original-quirk handling remain as recorded in `completion-producer-independent-audit.md` and `completion-header-independent-audit.md`. In particular, the producer's 73 DMA service intervals and header's three are not elapsed-time predictions. The components remain standalone; no original game behavior or fixture was changed to repair verification.
