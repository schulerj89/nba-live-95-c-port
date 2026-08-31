# Period render tail: source accepted, original verifier rejected

The frozen C component faithfully implements the bounded owned-data projection of original `$86:E1F7` through `$86:E207`. The original verifier is **not accepted**: four duplicate/order corruptions still complete all four native comparisons. A separate verifier repair is being reviewed; this report preserves the original rejection.

The independently rehashed original freeze is `completion-owner/build/period-render-tail-freeze-v1.json`, SHA-256 `564cf0b7f17625b6ce38d45b2af163f1468260e590d3ecb9d809684779dc6ec2`, with 2,082 identities. Original files, native fixtures, historical failed attempts and old reports were not changed. Private source snapshots and results are under auditor `build/period-render-tail-audit-v1`.

## Original-source contract

- `$86:E1F7` calls `$86:D5DB`; `$86:E1FB` calls `$80:FBFF`; `$86:E1FF–E204` increments `$084A` with carry into `$084C`. Raw caller bytes are `22 db d5 86 22 ff fb 80 ee 4a 08 d0 03 ee 4c 08 6b`. `$80:FBFF` is a depth sort, not video DMA.
- D5DB is the independently accepted support sorter: eleven canonical objects plus a terminating zero, with preceding `$34D1` explicitly zero. The source insertion reads the preceding sentinel; the typed component must refuse other sentinels. Link updates and wrapped comparison flags are retained.
- `$80:FBFF–FC0E` derives gaps 7, 3, 1. `$FC32–FC3D` compares the right depth against the left with wrapped 16-bit `CMP`/N. Equal values do not swap. `$FC4A–FC53` continues backward by the gap even if a comparison did not swap; the C loop retains this exact order.
- `$FC69–FC7C` subtracts X from Y with 16-bit wrap, performs two sign-preserving right shifts through `CMP #$8000`/`ROR`, subtracts camera `$0860` with wrap, and stores actor `$68`. Z is not read. The C comments correctly explain these source details; no native quirk was normalized.
- The typed API requires twelve unique canonical draw pointers, eleven canonical collision pointers and a zero terminal, consistent X values in its two views, and a zero leading sentinel. Invalid inputs are refused without changing the caller's state. The raw probe derives both X views from the same original actor bytes.

The original ROM is `F:/Games/SNES/NBA Live 95 (USA).sfc`, SHA-256 `2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`. The fresh raw decode is retained as `raw-rom-fbff.txt`. The Ghidra bank86 listing confirms the caller; no absent bank80 listing was treated as evidence for FBFF.

## Independent results

`build/run_period_render_tail_audit.py` compiled private exact C/header snapshots with `/W4 /WX`; read-only linked dependencies remain pinned by the freeze. No compiler warnings occurred. Its four before-state-only native replays match 12,496 bytes: collision list, all twelve actor/ball records, draw list and frame counter. The old eight malformed-process/output and nine invalid-domain guards also pass against this fresh executable.

`tools/test_period_render_tail_rom_audit.py` separately interprets original FBFF instruction bytes and reuses the auditor's earlier original-D5DB diagnostic. It does not import the owner's reference or the C algorithm. Four natural snapshots match both original endpoints and fresh C. Another 384 controlled cases match 1,199,616 bytes, covering signed/wrapped coordinate extremes, camera extremes, equal depths, randomized permutations, poisoned previous depths/links and low/high counter overflow. The source diagnostics execute 121 sort PCs; caller counter bytes are checked directly. All equal-depth cases preserve input draw order. Every unrelated output WRAM byte remains unchanged. These are controlled source checks, not claims that the extreme states naturally occur.

`build/run_period_render_tail_api_audit.py` freshly builds an exported DLL and checks null refusal, full counter wrap, equal-depth order and ten invalid typed states. Every invalid state remains byte-for-byte unchanged, including inconsistent typed X views that cannot be constructed through the raw adapter.

## Original verifier defect

`verify_strict.py` selects the first `roles.after` and first `formation.return` with `next()`. Its shared appearance validator verifies appearance hook counts and row fields but does not enforce complete boundary sequence identity. The render verifier adds PC, frame/court, SP and binary16/DP0 guards, but omits exact tail multiplicity/order/adjacency.

The independent `tools/test_period_render_tail_protocol_audit.py` patches parsed boundary text after immutable hashing and exercises the actual comparison loop. It independently checks all four C command routes. The baseline passes, but each of the following malformed traces also passes all four calls: duplicate `roles.after`, duplicate `formation.return`, reversed tail endpoints, and an inserted `roles.before` between the endpoints. No raw fixture or C output is altered. Wrong PC, SP, decimal flag, raw-bound field and clock controls correctly reject before C.

The original failure is retained at `independent-protocol-v1/report.json`. A new verifier must reject the four malformed sequences before invoking C; the C source needs no repair for this finding.

## Limits and identities

Captured period 0 clocks are frame/court 6767/2377 to 6768/2378; period 1 is 7127/2737 to 7128/2738; periods 2 and 3 remain in their entry frame. These native crossings are expressly observed, not reproduced by the C component. Native SP restores and D/M/X/DP guards support the bounded input contract. DBR is not captured. The proof excludes register/DP residue, interrupts, hardware, video/DMA, elapsed time, normal scheduler phase, whole-period integration and human play. The probe retains input bytes outside its owned projection; it does not seed a native poststate.

| Object | SHA-256 |
| --- | --- |
| C | `8552701d29ee36357a6b51c9c0395e3ca5b979d8e7293bf4842e68d78c99f18f` |
| Header | `e6eaf3598e7a3a70348ed0f9397accc0b1a4c014fc4d5ed416dc2cf9286df982` |
| Probe | `a1b1dbcd6026f706b3eed5f7349d350cc60050719029f371f380a8d39de41941` |
| Original verifier | `4eef04f555c96c01b906fd120fb32cbed9def61939ecec567cba80df610c2e48` |
| Fresh private executable | `ae4517df505a566dd82cb93135a9652351a410f8addada4a2276de44089e4954` |
| Independent ROM tool | `fddae49838a0ee023590c6a4a9f0ae6526318717b9af815996ad10bc75c737e1` |
| Independent protocol tool | `aed2a51e0830426fb9fdf7adc82486de164baf4174f97dc7887ad4ebb1b7b6db` |

The source acceptance is bounded as above; final composite acceptance requires the separately frozen verifier repair.
