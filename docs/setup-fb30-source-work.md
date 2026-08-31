# FB30 source work continuation

The new `nba_setup_fb30_work` module emits the actual bounded work of
`$80:C62B` through entry to `$80:C682` for the backdrop FB30 resource
`$AE:A0AF`. It produces 960 bytes in 115554 CPU cycles and 736896 intrinsic
master clocks, including 21786 slow bus accesses. The four earlier FB46
resources and their frozen API are unchanged. These five codec calls account
for 524073 of the backdrop's 550560 producer CPU cycles. The remaining 26487
cycles still require source-derived routing/transform/helper work.

This component is not wired into production timing or `nba95_sources.txt`.
Root owns integration after independent audit. Repeated Rules epochs and
brightness/RGB parity remain unproved; sound sequencer/SPC continuation is
still necessary to predict interrupt phase.

## Source and continuation contract

`tools/generate_setup_fb30_work.py` statically translates 892 reachable source
states from the canonical ROM, known routine boundaries and explicit native
jump tables. Its only inputs are the ROM and a pinned static disassembler.
`src/nba_setup_fb30_program.inc` is versioned C source with direct labels,
branches and bounded native table targets. It does not contain a runtime
opcode decoder, captured instruction list or captured state/timing input.
The static source generator is not part of production execution.

`src/nba_setup_fb30_work.c` implements the bus recipes and local flags needed
by that source. `peek` exposes one read/write/idle; `accept` consumes the live
read result and advances after that bus event. Instruction completion is
explicit. The caller owns WRAM/IO effects and any refresh, DMA, NMI/audio/SPC
clock work. DP-indirect effective addresses use pointer bytes accepted at
their actual reads. Word RMW writes high byte then low; ordinary word stores
write low then high. The complete continuation can be relocated at every bus
boundary without hidden pointers or hidden memory reads.

Caller preconditions are native mode, direct page zero, M=X=decimal=0,
FastROM, ordinary live WRAM/IO mirrors, a noncrossing ROM stream, and an
empty immediate-publication queue. These are bounded source preconditions,
not generic CPU support. The instruction limit bounds invalid/nonterminating
inputs and is never a delay or prediction. FB46 and unknown formats stop as
unsupported in this separate module.

Canonical ROM SHA256:
`2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`.
FB30 `$80:BE6B..C5AA`, 1856 bytes:
`e6ebb2723eed4c6564a6b289c247f4998176ff7c06d21f6a09c9ce8d15aea0cc`.
Wrapper `$80:C62B..C682`, 88 bytes:
`ad561c575d19757d4e2fa41d6c267514e5957195eebb33c69b843807c9d3bc98`.
Static disassembler `snes65816.py`:
`b1864664d3ac0abcd439055e88bf7220cf66be7a6ae9dfc5d59b3186f1469a46`.
The build manifest includes both the generated C and generator source.

## Independent semantic and native checks

The separate semantic witness `analyze_setup_fb30_semantics.py` constructs
canonical Huffman symbols from cyclic unused-symbol ranks, then decodes
literal/repeat/raw-literal/end tokens. It has no timing model. The native
code counts at lengths 1..10 are `[0,1,0,1,3,15,18,24,38,52]`, giving 152
symbols. The payload has 888 literal tokens and repeat lengths 31/15/11/15,
then termination. Its SHA256 is
`0b5c2dafc1a19c277807c8d7c81425e8fb0c23623f694ee9574452f1492b3bd7`.
The 719-byte prefetched input window hashes to
`c8e1e58135576ccb0f9b4de0d60d48565a9ecaf528c4dd53c70f06b8b19af340`.

The C probe starts with zero scratch and ordinary typed operands, with no
snapshot, native clock, phase or visit count input. A separate diagnostic run
uses only the native entry registers and empty queue cursor to compare leaf
states. Both produce identical payload, source path and intrinsic work.
All 36418 first-call PC/A/X/Y/SP/DB/P instruction states, every instruction's
CPU duration and all 9935 CPU write addresses/values/order/bus positions match
the native trace. Four complete native exit payloads, canonical tables, fast
tables and cursor/prefetch state match the independent semantic witness.

The immutable `.analysis/native-codec-work-v1` capture records 20 codec calls
and preserves all 7102 scheduler events from `.analysis/native-scheduler-v3`.
Manifest SHA256:
`392e653f348441a2e80bb2f8f355b37a284fa34c58c3bf261418ce51dd05b52f`.
Instruction SHA256:
`687b57de35c91eb6414a730e664799c8ae8264c73e3c72f68bdefb1bf83ad366`.
Writes SHA256:
`c73001995ef34b1246386620f51c5f6cdfdb901cc76a1f4e58046e84dc04902b`.

The validator positively checks the four NMI hardware stack writes and the
extra `$80815A` hook before separating them from producer work. It also pairs
each `$2180` CPU write with Mesen's induced WRAM effect: the first 152 target
`$7E:0100..0197`, then 960 target `$7F:2000..23BF`. Neither effect is silently
discarded. Intrinsic timestamps have strict mixed-event chronology and
six/eight-clock domains. Native master checks remove observed NMI work plus
142 entry/vector/RTI clocks per interrupt, leaving 40-clock refresh quanta.
That is conservation using observations, not a forward phase prediction.

## Preserved source quirks and tests

- `$80:C44D/C44F` peeks at the terminating one bit without consuming it.
  The native end cursor is bit5740, prefetch pointer `$A37E`, buffer `$8300`.
- `$80:BEB5` stores zero for a zero-count threshold slot; it is not replaced
  with a mathematically normalized threshold.
- `$80:BEC0` clears carry before `$BEC1` skips a zero shift. A synthetic
  complete depth-16 tree consequently fails to close at `$BECD` and continues
  construction. The bounded test retains this source behavior and reaches
  the instruction limit. This is a source-derived synthetic edge, not a claim
  that the original game's resource triggers it. The accepted resource uses
  depth ten; synthetic depths one through fifteen complete.

Fourteen C cases cover live pending reads, DP-indirect address latching,
high-before-low RMW order, relocation after every bus cycle, both empty-queue
branches, invalid states/formats and instruction limits. Eighteen Python
tests with subcases cover strict source/artifact/settings identities, numeric
tokens, corrupted NMI/WRAM observations, timestamp/order attacks, and synthetic
code depths one through fifteen with raw literals, repeats and termination.

## Reproduction

Run from the scheduler worktree, choosing fresh output directories:

```powershell
python tools/generate_setup_fb30_work.py --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --decoder-root 'C:/Users/joshs/Projects/tools/snesrecomp-source-v0.2.0-alpha/recompiler' --output src/nba_setup_fb30_program.inc --check
.\tools\build_setup_fb30_work_probe.ps1 -OutputDirectory .analysis/fb30-build-new
python tools/verify_setup_fb30_work.py --native .analysis/native-codec-work-v1 --previous-native .analysis/native-scheduler-v3 --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --exe .analysis/fb30-build-new/setup_fb30_work_probe.exe --output .analysis/fb30-proof-new
python tools/test_setup_fb30_work.py --native .analysis/native-codec-work-v1 --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --exe .analysis/fb30-build-new/setup_fb30_work_probe.exe
```

The proof and freeze record exact source/build/evidence identities. Build uses
`/W4 /WX` and no shared game objects. Independent audit is required before
acceptance of this bounded component; no whole-transition parity is claimed.
