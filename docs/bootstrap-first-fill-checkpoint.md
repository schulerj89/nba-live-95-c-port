# First reset DMA fill checkpoint

This separate candidate extends the independently accepted reset/upload/F1
checkpoint through the first8A57 helper and its return at80:80C0. The accepted
1013-identity packet is unchanged. No NbaGame wiring, production source manifest,
existing audio/SPC module or root file was edited. Independent acceptance of
this new candidate remains pending; **full S1 through03DB remains open**.

## Source and owner

`NbaBootstrapFill` contains one carried bootstrap CPU/SPC/clock owner plus its
first DMA continuation and64KiB VRAM. Normal power-on still accepts only the
canonical ROM and explicit NTSC zero-RAM software profile. It does not accept
native registers, ARAM, port replies or timing inputs. The initializer reuses
the accepted cold-state construction and prepares the expanded static CPU
continuation before any execution. There is no scene-state initialization.

The expanded static generator contains174 concrete states: the previous reset
and AB06 uploader plus80BC and8A57..8A92. The source stores A=0 at0016, observes
the signed0561/0562 display state, selects the immediate branch, and programs
channel1: mode09, fixed source000016, transfer length0000=65536, VRAM word0.
The queued branch8A93 and other DMA modes/channels refuse execution.

Both VRAM byte writes read0016. They do not alternate0016/0017 even though
8A69 stored a word. VMAIN80 selects increment after the high-byte port; every
word/address wraps as the original hardware source specifies. Constructor
DMA registers43x0..43xB begin FF, as in the pinned reference, and actual CPU
instructions overwrite the required fields. VRAM address/read-buffer state is
owned explicitly; IO bytes are last-write mirrors, not a substitute PPU.

After420B, the next real CPU cycle clears the start delay. The following CPU
access is suspended while DMA synchronizes, performs its overhead, executes
each4-clock read and4-clock write, and synchronizes its return. SPC/timer work
and refresh continue during every one of those clocks. The native DMA callback
PC is the suspended REP at808A90, not the initiating STA at808A8D. The current
CPU cycle during DMA is completed-cycle count+1, which the verifier also binds
to actual native bus records.

The pinned `SnesDmaController::RunDma` uses a byte-sized loop index when adding
8×index to its synchronization counter. For65536 bytes that index wraps to0;
the final counter is24. The candidate preserves that **reference software**
behavior, and the new native binary observation agrees. This is not classified
as an original NBA95 game bug or asserted to be a universal SNES hardware law.
No elapsed total or capture-derived cost table is used by the implementation.

## Fresh original-ROM evidence

`build/native-bootstrap-fill-v1` is a new hidden, isolated cold boot with the
same explicit profile, no inputs, save state, RAM/register seeds or ROM patches.
It observes seven full-state boundaries through CPU80C0. All earlier captures
are preserved. Manifest SHA256:
`76247a684753dc094855e024e1b88301968ca76e8e1a45191abf13f6554f7e2a`.

The fresh10-source /W4 /WX probe is
`build/bootstrap-fill-probe-v2/bootstrap_fill_probe.exe`, SHA256
`6c5cff8db4e97137fbb8ef74554bc6764c75625c076419fb4115612276d8b9ce`.
It runs from ROM only. `build/bootstrap-fill-verify-v2/report.json` verifies:

| Native comparison | Exact matches |
|---|---:|
| CPU instruction/register/width/master-clock states |28434 |
| CPU data accesses |16285 |
| DMA reads and writes |131072 |
| SPC instruction/register/cycle states |15400 |
| SPC writes and IO reads |8644 |
| Resident/F1 public scalars |84 |
| Final CPU/DMA/PPU typed fields |22 |
| Final WRAM |131072 bytes |
| Entry and post-F1 ARAM |2×65536 bytes |
| Final VRAM |65536 bytes |

The source-clock guard reconstructs all95152 CPU cycles,60179 SPC machine
cycles and924 refresh events. It checks ordered unique fetches, sample and
completion times, DMA synchronization/byte positions, marker binding and final
continuations. All seven native boundary metadata records bind to their actual
source hooks, instruction rows and raw snapshot counters.

Native after-DMA REP completion at8A92 matches master1260618/CPU95146. The
80C0 return matches master1260660/CPU95152. The final native SPC observer is
still at120358 ticks while the continuously scheduled source owner is due
through120362. The two final source clear cycles are therefore not relabeled
as native evidence. Their indirect read/write relation is checked against
the source pointer reads and A value. SPC callback-master equality, hidden DSP
state, DSP RAM reads and a combined final CPU/SPC state comparison are not
claimed. The first three IPL instructions remain source-only observations as
documented in the accepted checkpoint.

## Contracts and corruption cases

The fresh contract executable passes44 assertions and262144 VRAM-byte checks.
One case is the untouched normal zero fill; three are explicitly isolated
hardware tests that change0016/0017 and prefill VRAM only after normal source
has reached DMA. These tests prove the fixed-source distinction and wrap; they
are not native initialization, production inputs or normal-journey evidence.

The protocol suites pass21 original local rejection cases,12 adapted profile
cases and20 new DMA/endpoint cases, each with an accepted baseline. The last-SPC
completion mutation is adapted to this checkpoint's completed store rather
than falsely assuming the old checkpoint's partial BPL. Both independent tools
are reused unchanged: nine old clock/fetch/summary corruptions and three native
boundary corruptions all reject, with accepted baselines. No original C/native
files are modified by these parsed-view tests.

## Reproduce and remaining source work

```powershell
./tools/build_bootstrap_fill_probe.ps1 -OutputDirectory build/bootstrap-fill-fresh
python tools/generate_bootstrap_fill_cpu.py --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --decoder-root C:/Users/joshs/Projects/tools/snesrecomp-source-v0.2.0-alpha/recompiler --output src/nba_bootstrap_fill_cpu_program.inc --check
python tools/verify_bootstrap_fill.py --native build/native-bootstrap-fill-v1 --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --exe build/bootstrap-fill-fresh/bootstrap_fill_probe.exe --decoder-root C:/Users/joshs/Projects/tools/snesrecomp-source-v0.2.0-alpha/recompiler --output build/bootstrap-fill-fresh-native
./tools/build_bootstrap_fill_contracts.ps1 -OutputDirectory build/bootstrap-fill-contracts-fresh
./build/bootstrap-fill-contracts-fresh/bootstrap_fill_contracts.exe 'F:/Games/SNES/NBA Live 95 (USA).sfc'
```

Run `test_bootstrap_protocol_fill.py`, `test_bootstrap_profile_fill.py` and
`test_bootstrap_fill_dma_protocol.py` with those same native/ROM/executable/
decoder arguments and separate fresh outputs. Reuse the frozen independent
tools with `--verifier tools/verify_bootstrap_fill.py`.

The next CPU-owned interval starts at80C0's actual first8KiB WRAM-clear loop,
then copies source tables and calls DA72. DA72 initializes062C/07F2/07FE/08FE;
later source clears epoch/callback/queue fields and calls AB7E. That helper's
allocation/list initialization precedes8145 enabling NMI. These are new source
children, not a license transition or a delay. Normal03DB requires their real
concurrent CPU/NMI continuations and an explicit stop at the unresolved DSP
access. CPU subcycle state must be retained if that future hardware stop occurs
inside a bus access; no restart of already-consumed clocks is permitted.
