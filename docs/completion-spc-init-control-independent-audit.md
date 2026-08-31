# Independent SPC initializer and F1 control review

Source behavior passes this bounded review. The verifier-v3 composites are
rejected pending a separate metadata repair: the initializer accepts six of six
new malformed native-state views, and F1 accepts eight of eight. These are host
evidence-validation gaps, not original-game bugs. No frozen source, native
artifact, earlier rejection, or production behavior was changed.

## Identities and fresh reproduction

All paths below are relative to the auditor worktree unless a sibling is named.
Every file identity in scheduler `spc-init-freeze-v3.json` (50) and
`spc-control-freeze-v3.json` (49) was independently rehashed. Their SHA256s are
`97e501c17da39eed47777d91f52bc3c1205ed7f31a74d2294cb3c459da095bea` and
`3416006eadff4fa2a5fae969f80112464bbfb64f2d2445e67b555ce2b817ad29`.
All 24 objects in each original v1 freeze were also rehashed and retain exactly
the same identities in v3. Original v1 manifest SHA256s are
`ea4a57b9e6e926ae6334762c605c1869ff7b5aef0a2bb6deb4d39e69aee4db8e` and
`1e3ebad4a88fdf3df2451deaa54b2aa6d4429e4dea81348833240755d5892317`.

Fresh `/W4 /WX` builds use only the copied frozen module, header, probe, shared
resident types, and build script. They do not reuse scheduler object files:

| Component | Frozen C SHA256 | Fresh executable SHA256 |
| --- | --- | --- |
| SPC initializer | `f6fbf9c175a0b8204e471b738c165b3f43d82f2add4aedf7f22b6a11ccb292b9` | `ef771a50cf158f7ee902c0c9a0943d88b3809abbe387ded2601a19af6058076f` |
| F1 commit | `3fb6f35612eb0079ceabeceabefa24af73c0af809ff35f6095becec041cde101` | `e40771186979f6cb5143b4f3f8ee8e76daf8910d4c0fdc84411c4bc53944faf6` |

Evidence is in `build/spc-init-audit-v3` and `build/spc-control-audit-v3`.
`build/spc-init-control-independent-summary.json` records individual hashes and
the eight fresh instruction/write/endpoint files that are byte-identical to
the scheduler's retained v2 baselines. The local initializer regression 21/21,
F1 regression 32/32, v3 metadata 19/19 and 11/11, and earlier process protocol
4/4 per component pass. These results do not supersede the new failures below.

## Original initializer contract

The 49 concrete source PCs compile to actual bytes in the unchanged ROM upload
at file offset `4687`, mapped to ARAM `0380`. The canonical ROM SHA256 is
`2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`.
The complete 1,264-byte payload matches all four native ARAM snapshots. Native
capture is scheduler `.analysis/native-spc-init-v1`, not an injected register
or memory trajectory.

`0380 CLRP`, `0381 LDX #FF`, and `0383 MOV SP,X` reach `0384 MOV F1,#30`.
The C prefix accepts ten machine cycles, then refuses the F1 write itself.
Resuming at `0387` is a separate supported entry with P=0; it requires the
external owner to have advanced and committed F1 normally. The reviewed source
does not supply that owner or silently connect the two test seeds.

The clear body descends through `0000..007B`, clears the `0100` and `0200`
pages with wrapping Y, and descends through `0300..037F`. `03A3..03B5` builds
pointer `0870`, byte count `8F`, and page count `F7`. In particular `03AC SBC`
computes `FF-70=8F`, so `03BC` clears `0870..08FE`; **08FF is not written**.
`03C2..03D2` then clears `0900..FFFF`, wrapping the pointer high byte to zero.
The comment at C lines 89–90 preserves and explains this original omission.
The all-zero native fixture cannot dynamically distinguish a missing write
from a zero write; the original bytes and nonzero tests establish that detail.

`03D4..03D8` stores `20` to direct-page `04` and `6C` to the DSP address
latch/underlying `F2` ARAM. At `03DB MOV F3,A`, two fetch cycles are accepted;
the DSP **read** preceding the eventual store remains pending. Repeated
acceptance refuses mutation of both continuation and bus. Normal writable,
normal-speed hardware and the canonical resident upload are explicit caller
preconditions. Later DSP/timer initialization (`03DD..043F`) is excluded.

Fresh native comparison covers 192,818 instruction-entry register states,
including each completed duration, 64,394 accepted CPU write records and their
positions, and the complete clear ARAM endpoint. The native F1 write is an
additional observed write that the prefix deliberately does not accept.
The clear slice takes 835,242 accepted SPC machine cycles, not predicted SNES
master-clock elapsed time.

The independent tool `tools/test_spc_init_rom_audit.py` decodes a small bounded
subset of actual ROM bytes in an audit-only Python reference. It does not use
the C source table or link an interpreter into the port. Four control-prefix
and four nonzero clear prestates compare 771,272 instruction-entry register
states, 257,576 ordered writes, and all eight complete endpoints. All 49 PCs
are covered. Initial PS values vary, and the four clear tests leave distinct
nonzero values `96/F7/D4/35` at `08FF` with no write there. This independent
diagnostic makes no cycle or natural-reachability claim. Tool SHA256:
`19435249559de03294747c1b14dd3ded1bfb44bd1caedda446faa4e74f91bd41`.

## F1 hardware contract

The source reference is the frozen Mesen source set, separately pinned from
the actual captured Mesen binary. This review does not assert that the source
commit is the installed binary's build revision. `Spc.cpp` lines 274–329 calls
`IncCycleCount` before the write callback, gates underlying ARAM writes with
`WriteEnabled`, then applies F1 effects. `SpcTimer.h` lines 45–51 clears only
stage2 and output on a disabled-to-enabled edge. Both files were read and
rehashed; their SHA256s are respectively
`889890c164301011787dbb9345a32163f8737e3f44129507eff16e8a7389956c`
and `a6facbe791582f410b384ee359fe727ce8925f696b0a28340cbd5cd4795eb68e`.

The C commit correctly acts on already-advanced carried state. Bits 10/20
clear the selected visible CPU inputs and staged inputs, leaving SPC output
latches, underlying F4..F7 ARAM, and the pending-update flag untouched. Rising
timer-enable edges clear stage2/output even with global gating disabled;
repeated enable and falling edges preserve both. Stage0, stage1, previous
stage1, targets, and global gating survive all F1 values. Bit80 selects the IPL
read overlay at FFC0..FFFF, without changing underlying ARAM. Bits08/40 have
no control effect but remain in the full byte stored to ARAM when writes are
enabled. Write-disabled hardware remains a valid effect-only API input.
Comments explicitly retain the non-obvious staged-input/pending and edge rules.

The native proof contains two actual same-clock commit pairs: `0384/F1=30`
at SPC clock 68348, and `03EC/F1=01` at 1738896. All 70 exposed fields and
both complete ARAM endpoints match freshly compiled C. The latter preserves
timer0 stage0=16 while clearing its old output on the enable edge. Lua does
not expose `NewCpuRegs` or the pending-update flag; the five such input fields
are deliberately synthetic sentinels, not recovered native state. Original
`Spc.cpp` lines 489–490 excludes them from the Map serialization used here.

`tools/test_spc_control_source_audit.py` independently exercises all 256 F1
values × 8 incoming enable masks × 2 write gates × 2 pending flags: 8,192
controlled cases with nonzero timer history, distinct latch directions, raw
outputs above 0F, and mixed global enables. It compares every byte of the
hardware/bus structs, checks IPL boundaries, and verifies null-call immutability.
The DLL builds directly from frozen C and verifies ctypes layout sizes with
compiled sizeof helpers. Tool SHA256:
`d54c56a8d9b25fee67c11933c2bda0f395b4cdc371ece7f08d2ea9bca036b062`.
The first private DLL command had a trailing `/Fo` quoting error; its failed
log/batch are retained beside the successful `compile-v2` run.

## Required verifier repair

The unchanged independent tool
`tools/test_spc_init_control_boundary_audit.py` intercepts parsed state views,
with explicit hit assertions. It does not alter hashes or raw fixtures. Run it
with `--kind init|control`, the corresponding copied v3 verifier, original
native directory, canonical ROM, fresh executable, and a new output directory.
Original results are retained under each audit's `independent-boundaries`.

* Initializer verifier SHA256
  `75274d418fddb913c4b6cddc21e9da8019bd496325211d8506d1d849f449f447`:
  `native()` checks normal hardware in `pending_dsp.state` but never binds its
  PC, cycle, or source registers to the last instruction/read event. It accepts
  missing PC, PC=0, cycle=0, A=0, PS=256, and an invented state key. Require the
  attested state schema/domains and the actual source relation: pending PC is
  `03DD`, cycle is the `03DB` instruction clock plus six captured SPC clocks,
  and A/X/Y/SP/PS remain those at `03DB`. This checks the observed pending read;
  it must not accept the read into the C continuation or fabricate DSP effects.
* F1 verifier SHA256
  `d93c093596ce46fb55f7732fd063433196d74d8adbc57ab16bb1f659f31dd104`:
  equal before/after key sets allow required source registers and DSP fields
  to disappear from both sides. It accepts missing PC/A/PS/DSP counter,
  PC=0, PS=`false`, A=256, and an invented state key. Require the attested
  state schema and domains; actual callback PC must equal source_pc+3 at
  both snapshots (`0387` or `03EF`). Keep valid write-disabled/timer prestates;
  do not add artificial normal-hardware limits to the effect-only C API.

These revisions should be new verifier/helper/test files and new freezes.
Keep all v1/v2/v3 source and rejected evidence intact. Source function searches
found only module definitions and standalone probe callers, with no production
enabling. Neither component proves normal upload completion, cross-clock CPU
input visibility, DSP/timer advancement, normal SPC state creation, Rules
reentry, or whole Setup scheduler phase.
