# Reset WRAM and table initialization checkpoint

This new candidate continues normal cold bootstrap through the first DMA fill,
WRAM clear, ROM table copies, DA72 and AB7E children. It stops **before the
write at80:8145 enables NMI**. Full S1/03DB, NMI execution and production wiring
remain open. The previous1807-identity first-fill packet is unchanged; this
candidate requires separate independent acceptance.

## One carried state and source scope

`NbaBootstrapTables` is the same `NbaBootstrapFill` state, not another hardware
owner or a snapshot conversion. `nba_bootstrap_tables.c` compiles the unchanged
first-fill implementation with its CPU entry points bound to the expanded,
concrete source continuation. There is no memcpy/reinitialization at80C0 and
no duplicate WRAM, queue, RNG, port latch, timer, SPC or master clock. The build
receipt explicitly includes that frozen implementation as a dependency.
Initialization accepts only canonical ROM plus the declared zero-RAM NTSC
software profile. No runtime register/ARAM/response/timing input is added.

The static generator now contains333 concrete instruction/width states. Besides
the prior source it follows80C0..8144,80DA72..DA90,80AB7E..AC1A and80AC89..ACC1.
It emits named source continuations and live bus effects, not a generic opcode
interpreter or recorded instruction stream. Only the reached normal bootstrap
entry and its call/return widths are claimed; direct arbitrary helper entries
and unrelated branches are not a supported API.

| Source | Owned effects |
|---|---|
|80C2..80C8 |Descending STZ$00,X,8192 actual writes at1FFF..0000, including queue/scratch0100..02FF and previous stack bytes |
|80CA..80DB |Six bytes from ROM80:8006 copied to0006,059E and7E8FEE |
|80DD..80ED |Actual LDX0/BEQ skips the alternate copy; no invented table |
|80ED..80FB |Sixteen bytes from ROM80:971F copied to7E3425 |
|DA72..DA90 |DBR7E is installed/restored through real stack operations;062C=1,07F2=2,07FE=08FE=0 |
|8105..8111 |0562=8F; clear0564..059D |
|8115..812A |Three overlapping word-write pairs construct callbacks80800C at05C2,05C5,05C8 |
|812D..8136 |05CB,0031/33/35/37=0;0039 remains the actual earlier clear result |
|AB7E..AC0C |A/X/Y=0 from8138..813C selects the normal allocation initialization, not native arguments |
|AC0D..AC1A |Words2640..2E70 becomeFFFF, including byte2E71 |
|AC89..ACC1 |E100 words at2000..21FC every four bytes; original buffer field assignments and byte0566=1 atACBC |
|8141..8144 |SEP20/LDA81 complete; next STA4200 at8145 remains unexecuted |

AB7E's byte-indexed free-list construction includes index0, computesF8 after
it, and then replaces32EA byFF. It is kept literally. AC89 first reads05E5
even though the subsequent load replaces A. ACA6 copies05F3 into05DF and ACA9
copies prior05DF into05F5; these are not an abstract swap of05DF/05F5. The
earlier contract draft incorrectly expected05DF=2200; actual source says2000.
Only that test expectation was corrected. The original source continuation
and all native comparisons already matched. The failed contract build/run
directories remain retained; they are not successful evidence.

The future graphics publication queue must remain a view into this same RAM:
records0100..02FF, head word0035, tail word0037 and budget word0039. These bytes
also have decoder/scratch owners later. No independent queue array is created.
The zero values here are outcomes of actual reset writes, not a license to
clear records/cursors on later transitions. The new0566 publication request
is not consumed in this checkpoint. No queue/DMA timing prediction beyond the
actual first-fill route is claimed.

## Fresh normal evidence

`build/native-bootstrap-tables-v1` is an isolated hidden Mesen cold boot with
no inputs, state writes, save states or ROM patches. It observes12 boundaries
through8145 and retains full raw WRAM/ARAM/VRAM and public state at each.
Manifest SHA256:
`3cdcb9e20e68b6db04db8927c601d072884ddfb1f93b4ec88100e1809e3057c5`.
The explicit observer limits are100000 CPU and100000 SPC instruction entries;
they bound the capture, not the modeled execution time.

The fresh10-source /W4 /WX candidate
`build/bootstrap-tables-probe-v2/bootstrap_tables_probe.exe` has SHA256
`83513b87598257ca2c7ebed2c1ed19501f5c66ba483029db974066639d15be49`.
`build/bootstrap-tables-verify-v1/report.json` reports exact comparisons:

| Comparison | Count |
|---|---:|
|CPU instruction/width/register/master-clock states |58765 |
|CPU data reads/writes |27348 |
|DMA byte reads/writes |131072 |
|Observed SPC instruction/register/cycle states |20650 |
|Observed SPC writes and IO reads |10394 |
|Resident/F1 public scalar fields |84 |
|Final CPU/DMA/PPU scalar fields |22 |
|Full WRAM at80BC,80C0,80CA,80FD,8101,8141,8145 |7×131072 bytes |
|Resident and F1 ARAM |2×65536 bytes |
|Final first-fill VRAM |65536 bytes |

The unchanged source-clock/DMA guard independently reconstructs187416 CPU
cycles,88491 SPC machine cycles,1359 refresh events and65536 DMA bytes. It
checks complete CPU instructions, exact opcode/operand fetches, each bus sample
and completion, directional latch publication, SPC cycle deadlines and the
partial final SPC continuation. All12 native boundary metadata rows bind to
their source hooks, observed instruction rows and raw state counters.

CPU8145 matches native master1853726/cycles187416. The continuously advanced
source SPC is at tick176986,PC03CA,phase6, with its indirect store's write still
pending. Native SPC observation is lazy: its last reported tick is165782.
Thus no final combined CPU/SPC state or callback-master equality is claimed.
The additional source SPC cycles are supported by the accepted initializer
and explicit clock/indirect-pointer contracts, not relabeled as native events.
The first three IPL instructions and hidden DSP evolution retain the previous
source-only/exclusion distinctions. Exact DSP register reads remain unresolved;
no value has been manufactured for03DB's read-before-write atF3.

## Contracts, negative tests and reproduction

The fresh `bootstrap-tables-contracts-v3` executable passes2965 assertions and
10802 explicitly checked bytes. It verifies8192 ordered clear writes, including
512 queue/scratch writes; boundary zeros; actual ROM copies; allocator/link
contents; descriptor and OAM buffer initialization; no4200 enable; and inert
source-stop behavior. It starts only from ROM and does not inject state.

The previous local protocol/profile/DMA suites are reused with the new reader:
21/12/20 rejection cases, each with an accepted baseline. Both independent
tools are unchanged:9 clock/fetch/summary and3 native-boundary corruptions.
The new table-specific suite covers5 new hook PCs,5 full WRAM boundaries,
ordered clear writes, final queue/head/tail/budget/OAM bytes, table ROM reads,
0566 publication and the source-stop PC. Original artifacts remain immutable;
negative tests change only parsed in-memory views.

```powershell
./tools/build_bootstrap_tables_probe.ps1 -OutputDirectory build/bootstrap-tables-fresh
python tools/generate_bootstrap_tables_cpu.py --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --decoder-root C:/Users/joshs/Projects/tools/snesrecomp-source-v0.2.0-alpha/recompiler --output src/nba_bootstrap_tables_cpu_program.inc --check
python tools/verify_bootstrap_tables.py --native build/native-bootstrap-tables-v1 --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --exe build/bootstrap-tables-fresh/bootstrap_tables_probe.exe --decoder-root C:/Users/joshs/Projects/tools/snesrecomp-source-v0.2.0-alpha/recompiler --output build/bootstrap-tables-fresh-native
./tools/build_bootstrap_tables_contracts.ps1 -OutputDirectory build/bootstrap-tables-contracts-fresh
./build/bootstrap-tables-contracts-fresh/bootstrap_tables_contracts.exe 'F:/Games/SNES/NBA Live 95 (USA).sfc'
```

Run `test_bootstrap_protocol_tables.py`, `test_bootstrap_profile_tables.py`,
`test_bootstrap_tables_dma_protocol.py` and `test_bootstrap_tables_protocol.py`
with the same native/ROM/executable/decoder arguments and fresh output paths.
The frozen independent tools accept those arguments plus
`--verifier tools/verify_bootstrap_tables.py`.

## Next real boundary

8145 writes4200=81, enabling NMI and auto-controller reads. Those effects need
their real hardware owner before the source can continue into9B73. The current
IO last-write mirror alone is not an NMI implementation. Future source work
must carry interrupt pending/sample state, source CPU entry/return, controller
and publication effects alongside continuous SPC execution. If03DB is reached
during a CPU bus cycle, the stop must preserve its already consumed subcycles;
no parked CPU, repeated elapsed clocks, canned DSP read or scene transition
may replace that dependency. This checkpoint is an additional prerequisite,
not the requested full timing repair or a Rules reentry acceptance.
