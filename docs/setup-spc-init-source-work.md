# SPC resident initializer: control boundary and exact RAM clear

`nba_setup_spc_init` derives the uploaded resident's entry work and RAM clear
from source `$0380..03DC`. It has two explicit component entries so that the
unimplemented hardware owner cannot be silently bypassed:

- `$0380` executes CLRP, LDX `$FF`, stack assignment, and the fetch/read prefix
  of `$0384 MOV $F1,#$30`. It stops **before** the control-register write.
- `$0387` is the continuation after that control publication has actually
  completed externally. It executes the source RAM-clear loops and DSP address
  selection, then stops **before** `$03DB MOV $F3,A` reads DSP data.

No normal-state seed, timer/control completion, DSP response, fixed delay or
production audio change is included. The second entry is not authorization to
skip `$0384` in a normal journey. Its live register/bus state must eventually
come from the first component and real hardware owner, not the test snapshots.

The source is the verified resident payload at ROM `$00:C687..CB76` mapped to
ARAM `$0380..086F`. The implemented 93-byte interval's SHA256 is
`48f09baaf03a72214af61db54494649be7b17546656b341933d33b60a39c20ea`.
All 49 compiled source PC/byte strings are checked against the original ROM;
all 49 states occur in the new native trace. C uses a source-PC switch with
individual bus continuations, not an opcode interpreter or trace playback.

## Source effects and preserved quirks

The caller supplies ARAM, both directional port arrays, DSP-address latch and
live SPC registers. The shared bus type is the earlier frozen resident API;
the earlier implementation/header/freeze are unchanged. The hardware must run
at normal internal/external SPC speed with ARAM writes enabled. `$0380` permits
the caller's P flag because the first source instruction clears it; `$0387`
requires P=0. Source registers otherwise remain live inputs.

The new source emits opcode and operand fetches, read-before-write accesses,
pointer reads, ordinary reads/writes, internal cycles and instruction completion
one SPC machine cycle at a time. DBNZ Y decrements at its source operand-fetch
cycle and leaves PS unchanged. The descending lower clears and pointer carry/
borrow behavior are retained. Stores and increments retain their individual
read/write order rather than becoming block copies.

The clear covers these exact source-derived ranges:

| Source | Written range or effect |
| --- | --- |
| `$038B..038E` | Zero `$0000..007B`, descending X. |
| `$0391..0399` | Zero `$0100..01FF` and `$0200..02FF`; Y wraps in DBNZ. |
| `$039D..03A1` | Zero `$0300..037F`, descending Y. |
| `$03A3..03B5` | Derive pointer `$0870`, count `$8F`, remaining-page count `$F7`. |
| `$03BC..03C0` | Zero `$0870..08FE`. |
| `$03CA..03D2` | Zero `$0900..FFFF`, with the final pointer high-byte wrap. |
| `$03D4..03D6` | Driver byte `$04=$20`. |
| `$03D8` | Select DSP address `$6C` through `$F2`; no DSP data operation yet. |

The source's `FF-70` count is `$8F`, so **`$08FF` is not written**. This original
omission is preserved and commented at the source subtraction. A `memset` over
the entire upper range would change original behavior. The low un-cleared
`$007C..00FF` bytes also remain intact except the actual `$F2` address write.
Final pointer/count bytes are `$00/$01=0`, `$02=$8F`, `$03=$F7`, `$04=$20`.

The nonzero test fills all non-code ARAM with `$A5`, supplies the original ROM
payload, and runs the clear with no native prestate. It proves every expected
cleared/preserved byte, that `$08FF` stays `$A5`, and that no source write ever
addresses `$08FF`. The all-zero native fixture alone cannot distinguish this
omission and is not claimed as dynamic native proof of that quirk.

## Hardware boundaries and remaining normal ownership

At `$0384`, the source reads `$F1` before writing. The documented/read-observed
hardware register read returns zero. Its write would clear the four CPU input
latches, disable the timers, and disable IPL ROM mapping according to the real
hardware state. It must not clear the separate SPC output latches. The new
`control_publication` request exposes the pending address `$F1`, value `$30`
and instruction completion, but `accept` refuses it without state/bus mutation.
No elapsed cycle is charged for that unaccepted publication.

The native witness shows the pending write occur after the 10 accepted prefix
cycles, then reaches `$0387`. That observation validates the boundary and
required value; it is not an implementation of the timer/IPL/port owner. The
initial IPL/upload acknowledgement in SPC output `$F4` remains untouched by
both components.

After the clear, `$03DB MOV $F3,A` first reads DSP data before writing `$20` to
the selected register `$6C`. The new module stops before that read and refuses
acceptance. No DSP read value is assumed and neither the DSP write nor its
effects occur. The two instruction fetch cycles already completed are retained.

Remaining source `$03DD..043F` performs the later DSP writes, timer target
`$FA=$10` and control `$F1=$01`, sample pointer/directory setup and output
`$F5..F7` clearing. It then jumps to `$0447`, the previously bounded resident
poll entry. These instructions and hardware owners remain unimplemented here.
The timer owner must carry divider/output/enable state from actual reset and
upload; the DSP owner must carry its actual registers/execution state. The
CPU/SPC clock visibility adapter, subsequent timer/voice service and menu dwell
are still required before normal sound-state or transition-phase prediction.

## Fresh evidence and limits

`.analysis/native-spc-init-v1` is a new cold-reset run of the unchanged original
ROM with isolated all-zero power-on settings. Lua performs observations only.
It records every one of the 192,818 SPC instruction states through the first
DSP-read instruction, all 64,395 writes (including the unresolved native `$F1`
publication), three IO reads, and four ARAM/SPC-state boundaries. Binary records
retain PC, registers and SPC clock positions without a truncated text log.
This capture completed; the earlier resident observer timeout attempts remain
untouched in their own directories.

The verifier uses native prestates only in two labeled isolated differentials.
It never supplies a captured timer/DSP response or elapsed time to the module.
The control slice matches four instruction states and 10 accepted cycles with
no writes. The post-control slice matches 192,814 instruction/register states,
64,394 writes and their cycle positions, and the complete native 64KiB ARAM
endpoint. Its accepted work is 835,242 machine cycles, including two fetches
before the still-pending DSP read. These totals are source checksums, not delay
constants. No master-clock/CPU phase prediction is made.

The verifier checks binary record lengths, raw chronology, source-PC association,
bus scope clocks, exact source/build/capture identities, persisted settings,
the full 1,264-byte uploaded source at all four boundaries, source registers,
cycle positions, full ARAM effects, and unchanged directional port latches.

Twenty-one local cases cover nine corrupted native/C views, five identity/
schema mutations, poisoned clear boundaries, pending control side effects,
invalid entries/direct-page state and truncated/extended binary input. Repeated
acceptance of either pending hardware request must leave both source and bus
unchanged. Changing only the observed pending DSP read to `$FF` produces the
same C traces/endpoints, proving that response is excluded.

| Evidence | SHA256 |
| --- | --- |
| Native manifest | `1371fb4252c297223f8ca0aa80ae96d381c14cbcdcda5eade0cf312b8ab6f523` |
| All native instruction records | `3f48e646d41434fb8a13e338d4e8c5d54994dad5214045f430927af4a1f560a0` |
| All native write records | `57028134affd8a13f4b01488febda6dedabaf374136353b6fdcef64d68a9a8c3` |

The hardware-cycle reference is the previously retained Mesen source commit
`b9fa69ddc6d0a331fb103fdb5eef6904305703c2`, especially its SPC addressing modes,
DBNZ, stores, SBC and control-register handling. Its source revision is not
claimed to identify the installed Mesen binary. The fresh installed-binary
trace independently validates the exact instruction/write cycle sequence.

Reproduce with fresh private output directories:

```powershell
.\tools\build_setup_spc_init_probe.ps1 -OutputDirectory .analysis/spc-init-build-new
python tools/verify_setup_spc_init.py --native .analysis/native-spc-init-v1 --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --exe .analysis/spc-init-build-new/setup_spc_init_probe.exe --output .analysis/spc-init-proof-new
python tools/test_setup_spc_init.py --verifier tools/verify_setup_spc_init.py --native .analysis/native-spc-init-v1 --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --exe .analysis/spc-init-build-new/setup_spc_init_probe.exe --output .analysis/spc-init-tests-new
```

Independent audit is required. All prior freezes, production `nba_spc.c`,
`nba_audio.c`, build manifests and integration remain unchanged.
