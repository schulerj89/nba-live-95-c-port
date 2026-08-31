# Normal reset/upload/F1 checkpoint

S1 implementation, based on integration7002fb1, 2026-08-31. This is a working
standalone composition from canonical ROM and a declared power-on profile.
**Full S1 through03DB and production timing remain open.** The first unimplemented
CPU continuation is80:80BC's call to the8A57 reset fill. No game source manifest,
NbaGame scene/init code, existing component or frozen evidence was changed.

## Implemented source and state ownership

`nba_bootstrap` owns persistent CPU/SPC continuations, WRAM/ARAM, separate
directional and staged port latches, timer prescalers/output, IPL visibility,
master/SPC clocks and refresh phase. It must live outside NbaGame.scene, which
nba_game.c305 clears. Its public power-on API takes only immutable canonical ROM
and a named software comparison profile. It verifies the complete ROM SHA256;
there is no API to supply native registers, uploaded ARAM, a port response,
captured timing or expected after-state.

The CPU source generator compiles143 concrete reset/upload states from original
00:800D..801F,80:8020..80BB and80:AB06..AB7D. Runtime has source labels and bus
continuations, not an opcode interpreter. Initial A/X/Y/DB/DP/SP/P/emulation
come from the pinned CPU power-on implementation. Reset CLC/XCE, stack setup,
native widths and JML then execute. The original upload descriptor and byte
tokens are read through ROM/WRAM transactions by these source instructions.

The fixed64-byte IPL firmware has a source-specific C continuation for its
own concrete PCs. It clears its original lower RAM range, emits AA/BB, handles
CC startup, echoes upload tokens, stores every payload byte, reads the zero
length/entry descriptor and reaches0380 with source-produced registers.
ROM00:C687..CB76 supplies1264 bytes to ARAM0380..086F. No memcpy of that payload
initializes the SPC. The full destination is initially zero and the normal
bootstrap performs1264 original IPL store instructions.

The existing accepted resident initializer starts directly from that same live
PC/register/bus state. At0384's final cycle the new owner charges the cycle,
advances timers, calls the accepted F1 commit and completes that exact MOV.
It never clears a refusal flag or calls begin0387 with replacement state.
Registers and instruction/cycle counts continue through0387 and the original
RAM clear. The F1 operation clears CPU inputs and staged inputs while retaining
SPC outputs, timer prescaler history and the original pending-update behavior.

The previously circulated description “03DB OR A,$F3” is wrong. Original ROM
offset0046E2 contains **C4 F3**, `MOV $F3,A`. The frozen accepted C has always
implemented its read-before-write access correctly. No old evidence was edited
to hide this documentation error. That unresolved DSP read is still beyond the
current CPU stop and is not supplied a canned value.

## Clock and hardware contract

The explicit profile is the pinned Mesen reference's NTSC master21477270 Hz,
SPC base32000 plus UI-default adjustment40, normal internal/external speed,
zero power-on RAM, no random PPU state and no prior save. It is a software
comparison profile, not a claim that every physical SNES has identical clocks
or initialized RAM. Sources are pinned to Mesen commit
`b9fa69ddc6d0a331fb103fdb5eef6904305703c2`; that source pin does not assert the
installed emulator binary was built from that exact revision.

The complete25-file reference closure is retained under
`build/s1-source-reference-v1`, including CPU/memory/PPU, SPC/timers, DSP and
the UI clock default. Its manifest records exact bytes, upstream paths and
the earlier read-only local origins. The header's shorthand “DSP remains reset”
refers only to the reset FLG preventing ARAM writes here; private DSP evolution
is unresolved, not frozen or asserted identical. SPC due work is run eagerly
by this owner, while the native observer catches it up lazily as discussed below.

The CPU owner resolves current slow/FastROM and IO bus speeds, including the
source420D write. Reads sample after speed-minus-four clocks, then complete the
last four clocks; both sample and callback-completion clocks are recorded.
Refresh uses the source538-minus-phase position and40-clock stall. CPU data bus
callback times and instruction clocks are checked directly, without a fitted
total. The source186-clock startup and two SPC reset-vector reads are explicit.

SPC due work uses the profile's exact integer clock ratio and two input-clock
ticks per normal machine cycle. CPU writes first catch the SPC up, then use the
reference's first-half immediate versus second-half staged input publication.
A staged update commits after the next SPC cycle's read/write/semantics. The
accepted “already visible input” helper is not used as a CPU write adapter.
Timer prescalers run even while their individual enable bits are clear.

Mesen batches SPC execution at CPU port accesses and frame catch-up. Consequently
its CPU-master timestamp on an SPC callback is **not** the SPC oscillator
deadline. Fresh native resident-entry/post-F1 callbacks both occur at CPU master
1021632, inside later reset DMA, but their SPC ticks are68326/68348. The source
composition produces those same SPC ticks and states at profile deadlines
715638/715868. The verifier compares SPC cycle positions separately from CPU
callback clocks; it does not claim those differing callback-master timestamps
match or introduce an offset. All actual CPU port/bus observations through80BC
match directly. The later concurrent CPU/DMA continuation still needs work.

Full DSP state is deliberately unresolved. Before the first F3 access, pinned
DSP reset FLG is E0; EchoStep29/30 cannot write ARAM. DSP reads and private
register evolution are not replaced with fabricated results. Elapsed unresolved
DSP steps are recorded, and a DSP access stops rather than returning zero.
The raw SPC capture contains DSP RAM reads as well as SPC CPU reads. The current
bus comparison explicitly covers **all SPC writes and F0..FF IO reads**, plus
complete instruction/register positions. It does not relabel all those raw RAM
reads as CPU fetches or claim full DSP access parity.

## Fresh results and exact scope

Private8-source /W4 /WX probe SHA256:
`db5fd1c28adda3eaa85e060716b6ae63f83a69b102b35e50a1339163f80bb245`.

Fresh native `build/native-bootstrap-v2` uses the original ROM, no inputs,
patches, memory/register seeds or loaded state, and a private portable emulator
and empty save home. It continues the CPU naturally into reset DMA until the
actual SPC0387 callback. Manifest SHA256:
`5ccf888c0570cc6e71efe94039a73d6c319db8a1c86ef3a9edff0a615a8b939a`.
Earlier v1 stopped before DMA and did not observe resident entry because of
lazy SPC catch-up; it is retained unchanged, not relabeled as a full witness.

`build/bootstrap-verify-v1/report.json` freshly runs the C probe with ROM only:

| Comparison | Result |
|---|---|
| CPU instruction entry PC/registers/widths/cycles/master clocks |28405 exact states through the instruction before80BC |
| CPU data reads/writes, addresses/values/callback clocks |16259 exact operations |
| SPC instruction states and SPC cycle positions |9616 exact observed states |
| SPC writes and IO reads |6594 exact operations/positions |
| Resident entry and post-F1 public scalar state |84 exact fields, including directional latches/timers |
| Full resident-entry and post-F1 ARAM |2×65536 exact bytes |
| Full CPU80BC WRAM |131072 exact bytes |

Mesen installs the SPC observer after the first three IPL instructions; their
initial cycle/register evolution is source-derived by C, not injected from
the observer's first visible prestate. The unobserved initial source interval
is explicitly separate from the9616 natural instruction comparisons.

C stops before80BC at master719846, CPU cycles95048, after528 refresh events.
The SPC has independently completed F1 and is partway through the accepted
clear at038E phase2. The native CPU80BC state has exactly that CPU clock/WRAM;
its lazy SPC bookkeeping has not caught up yet. No claim of complete CPU/SPC
instruction scheduling after80BC, elapsed DMA, NMI,03DB, audible audio or Rules
epochs follows from these results. Independent acceptance is pending.

The local protocol suite passes a valid baseline plus21 deliberate parsed-view
corruptions: missing identities, settings, invalid numeric process status,
native chronology/PC/cycle corruption, C fetch/write/clock/byte-domain changes,
premature instruction end and malformed idle accesses. C contracts check18
assertions plus all65536 initially-zero ARAM bytes; that is not65554 distinct
algorithms. Source generator regeneration matches the canonical ROM. Every
earlier accepted source file remains unchanged.

## Reproduce with new private output directories

```powershell
./tools/build_bootstrap_probe.ps1 -OutputDirectory build/bootstrap-fresh
python tools/generate_bootstrap_cpu.py --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --decoder-root C:/Users/joshs/Projects/tools/snesrecomp-source-v0.2.0-alpha/recompiler --output src/nba_bootstrap_cpu_program.inc --check
python tools/verify_bootstrap.py --native build/native-bootstrap-v2 --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --exe build/bootstrap-fresh/bootstrap_probe.exe --decoder-root C:/Users/joshs/Projects/tools/snesrecomp-source-v0.2.0-alpha/recompiler --output build/bootstrap-fresh-native
./tools/build_bootstrap_contracts.ps1 -OutputDirectory build/bootstrap-contracts-fresh
./build/bootstrap-contracts-fresh/bootstrap_contracts.exe 'F:/Games/SNES/NBA Live 95 (USA).sfc'
python tools/test_bootstrap_protocol.py --native build/native-bootstrap-v2 --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --exe build/bootstrap-fresh/bootstrap_probe.exe --decoder-root C:/Users/joshs/Projects/tools/snesrecomp-source-v0.2.0-alpha/recompiler --output build/bootstrap-protocol-fresh
```

Only after coordinating the one native slot, the optional capture command is
`python tools/capture_bootstrap.py --rom ROM --mesen PINNED_MESEN --output NEWDIR`.
The runner hides its private child process and preserves all existing runs.

## Next concrete child

80:80BC JSL8A57 uses A=X=Y=0. Its real immediate branch sets mode09, source0016,
size0000 (65536 bytes), destination VRAM0 and starts DMA at8A8D. Even though the
fill writes zeros over zero-initialized VRAM, every bus operation and elapsed
hardware phase remains required. The next implementation must translate that
source child and DMA/VRAM ownership, then the remaining reset CPU work and NMI
as reached, without parking the CPU while the835242-SPC-cycle clear runs.
The source-only accepted clear proof remains useful, but it cannot close the
normal concurrent03DB milestone by itself.
