# Backdrop producer source work

The new `nba_setup_producer_work` continuation closes the previously unexplained
26487 CPU cycles with source work, not a residual delay. It composes the
unchanged FB30/FB46 continuations with the actual routing, palette/fill/graphics
helpers, map-row publisher and in-place map transform.

The component starts at `$80:EC68` and finishes after `$80:EDF8` RTL. This costs
550552 CPU cycles and 3493454 intrinsic master clocks. The following caller
instruction `$81:D018` JSL `$80:EEC6` adds eight CPU cycles and 54 intrinsic
master clocks, giving the common 550560 backdrop-to-header producer cycles
for all four native journeys. DMA servicing, refresh and interrupts remain
external clock work. No repeated-Rules phase or display parity is claimed.

## Source-derived decomposition

| Source work | CPU cycles |
| --- | ---: |
| Five unchanged codec components, excluding their final RTLs | 524073 |
| Routing and final backdrop RTL | 619 |
| `$80:8AD2` fixed-source fills | 328 |
| Five codec RTLs at `$80:C682` | 30 |
| `$80:8BA1` graphics publication | 222 |
| Four additional `$80:86DA` empty-queue calls | 136 |
| `$80:8A02` palette publication | 208 |
| `$80:8CD0` map-row publication | 6580 |
| `$80:EA4B` in-place map transform | 18356 |
| Caller JSL at `$81:D018` | 8 |
| Total backdrop entry to header entry | 550560 |

The local component subtotal, excluding codecs and the caller JSL, is 26479.
The transform visits 416 words after the six-byte map header. The two map
publications each visit 32 rows, with widths 32 and 13 words. Across the
component there are 73 DMA requests and 27726 payload bytes: two8192-byte
fills, two16-byte fills, two960-byte graphics transfers, one6336-byte graphics
transfer, palettes160/14 bytes, 32 rows of64 bytes and 32 rows of26 bytes.

## Continuation and source boundary

`generate_setup_producer_work.py` generates 390 static C source states from
the canonical ROM and pinned static disassembler. It has no native trace,
snapshot, opcode-playback or measured timing input. The generated C uses direct
source labels and known call returns. Unknown/queued helper paths stop as
unsupported instead of being treated as completed work.

The public component owns a resumable source continuation and the two frozen
codec continuations. Both codec implementations share the same native wrapper.
Until its signature branch selects a body, both receive the same accepted
prefix bus result, with an explicit equality check for every proposed bus
event. Only one event is exposed and charged. The unsupported body drops out
at the source branch. This preserves both formats without changing either
frozen API or reading the signature early. A synthetic selector-zero route
exercises FB30 as the second resource, in addition to the native selector34
route's FB46 second resource.

The caller supplies native mode, M=X=decimal=direct-page=0, FastROM, live
WRAM/IO mirrors, supported noncrossing resource streams, and an immediate
empty publication queue. `peek` exposes one read/write/idle and its instruction
completion marker; `accept` resolves the actual read and advances source work.
The complete state can be relocated after every bus event, including while
one of the nested codecs is active. Long indirect reads/writes latch all three
pointer bytes at their actual bus accesses.

The diagnostic default caller derives layout words from ROM `$81:B8C2` using
the source `$80:E986/$E9B8/$E9CF` conversions. Selector34 comes from
`$81:D00A/$D00D`. It initializes ordinary typed call operands and zero scratch;
it does not load a captured WRAM snapshot, native phase or elapsed time.
Separate native-register diagnostics prove identical work and DMA payloads,
without claiming that full initialization and menu dwell have been ported.

## Preserved behavior

The generated code retains the original separate low/high fill DMAs, the
shift/ADC carry dependencies in map-row setup, the per-word transform, and
the selector>=35 fallback at `$ECC7/$EDF9`. A synthetic selector35 run reaches
the same resources/output as selector0 while performing the source's eight
additional routing CPU cycles. No source quirk is normalized to shorten work.
The frozen codecs retain their documented caller widths, zero-count table
handling and unconsumed FB30 terminating bit.

`$420B` is a DMA request, not an atomic elapsed-time charge. The new native
capture shows service under the following instruction's source PC: every
DMA byte has the same CPU-cycle count, two after the trigger write, with
four master clocks between its source read and PPU write. The diagnostic
probe applies payload effects at the request for byte validation only. A
production hardware adapter must separately preserve pending-DMA servicing,
alignment, refresh and interrupt eligibility. No observed service time is a
runtime input or inserted delay.

## Native evidence and verification

The new read-only `.analysis/native-producer-work-v1` capture loads exact copies
of the frozen scheduler and codec scripts. All prior scheduler, codec
instruction, codec write and codec boundary JSON files are byte-for-byte
unchanged. It adds 48 producer/codec boundaries, eight full producer WRAM
snapshots, 6519 residual instruction observations and 65218 data-bus events.
Its private Mesen/settings/save environment, sentinels, exit code, complete
artifact inventory and script revisions are attested and rechecked.

The composed C run matches all155750 first-scope PC/A/X/Y/SP/DB/P instruction
states and every instruction's CPU duration. All40003 CPU writes match
address, value, order and bus-cycle position. All27726 DMA source bytes and
PPU-write bytes match in order; each is tied to its source request. The
complete6336-byte produced scratch span matches all four native endpoints.
All four intervals independently conserve550560 producer CPU cycles after
removing recorded NMI work plus19 source entry/vector/RTI CPU cycles per NMI.

For155677 non-DMA-service instruction intervals, native master time also
conserves the source intrinsic work after recorded NMI work plus142 clocks
per NMI and40-clock refresh quanta. The73 intervals that service DMA are
explicitly outside that elapsed-time check. Their byte effects and source
CPU work are checked, but their service/alignment time is not predicted.

The verifier positively validates the three excluded caller-JSL stack writes,
the codec observers' NMI prologue convention, and all induced WRAM effects.
It requires strict mixed-event chronology and bounded intrinsic clock domains.
Thirteen Python integrity tests with subcases, eleven C continuation cases,
and nine in-memory trace-corruption cases pass. Corruptions cover reversed
event order, backward intrinsic timestamps, register/cycle/write changes,
DMA trigger association/payload and excluded caller-stack writes. Original
native evidence is never changed by these tests.

Canonical ROM SHA256 is
`2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`.
New native manifest SHA256:
`a8ff3873d3cc3e756b50017b9fcd2111102de90a4b5bba62b3b96a7edee38eae`.
Residual instruction SHA256:
`19e792b2f7a660e133260e0f75f83d00ab6c61bac25a61eaef769773c46539a5`.
Data-bus SHA256:
`f22a27457c49ad4dfcb029677fcf1f8c3b9fe3c7bcccc8883191e183a59c590b`.
Boundary SHA256:
`fcfcab1935192ccc71a175ae462fa524900088fd97d532992963f27992147082`.

Source routine hashes:

| Inclusive source bytes | SHA256 |
| --- | --- |
| `$80:EC68..EDFC` | `18b058c5a5593798d191c985e2e3116beeb05b92bfc9a8b30bd5385f2185f96c` |
| `$80:8AD2..8B34` | `f0bc43ef1ae47e1e21f73185586f049a50bfe0cd636ffb9308a3ebe99c90372f` |
| `$80:8A02..8A41` | `59db9b85254400a48a14005cd77311fd13b0529e1439985d38e9217698350391` |
| `$80:8BA1..8BCF` | `beab9087c553d3f52cb48f84628a8a74ff2a6dcaa002da093905efa2e15575db` |
| `$80:8CD0..8D98` | `a8e87a242a2b89a12c001d8f5e39b2da249cc6a412a2af97a809741ad2d29055` |
| `$80:EA4B..EA78` | `b0f8f5d6ecd534948d2dd12f213c98b233dca7e2066c6679dfac8d070aeaf04e` |

## Reproduction and open work

From the scheduler worktree, choose fresh output directories:

```powershell
python tools/generate_setup_producer_work.py --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --decoder-root 'C:/Users/joshs/Projects/tools/snesrecomp-source-v0.2.0-alpha/recompiler' --output src/nba_setup_producer_program.inc --check
.\tools\build_setup_producer_work_probe.ps1 -OutputDirectory .analysis/producer-build-new
python tools/verify_setup_producer_work.py --native .analysis/native-producer-work-v1 --previous-native .analysis/native-codec-work-v1 --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --exe .analysis/producer-build-new/setup_producer_work_probe.exe --output .analysis/producer-proof-new
python tools/test_setup_producer_work.py --native .analysis/native-producer-work-v1 --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --exe .analysis/producer-build-new/setup_producer_work_probe.exe
```

The component remains outside production configuration pending independent
audit. Root owns integration. Header440CPU work, pending-DMA service/alignment,
refresh/NMI scheduling and source-derived audio/SPC state carried from normal
initialization/menu dwell remain separate requirements. This component does
not establish loaded epochs72/15/71/15 or after-wait73/16/72/16, nor fix Rules
brightness/RGB parity or the Custom-return hidden-VRAM discrepancy.
