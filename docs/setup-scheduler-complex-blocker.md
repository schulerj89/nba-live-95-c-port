# Rules reentry scheduler: narrow complex blocker

Date: 2026-08-31. Owner task: `01a05629-d51a-7a00-bb98-a441b8ae518a`.
Scheduler worktree: `C:/Users/joshs/Projects/nba-live-95-c-port/.analysis/worktrees/completion-scheduler`, branch `work/completion-scheduler-20260830`, base `52c28996cdf693e0ae45aef714b47f698abd3ee1`.

This record requests read-only diagnosis, not a permission to fit timings or
replace the native witnesses. Production `nba_setup_screen.c` is unchanged
in this workstream. Bounded epoch/queue primitives are being implemented in
separate files; they are not production-wired and cannot themselves fix this
blocker. Root is concurrently fixing headless held/release input and native
configuration setup. There is no newly passing C reentry result.

## Exact unresolved contract and first divergence

The production Setup transition driver advances captured per-frame PPU and
resource records. It does not maintain the master-clock phase of executing
resource tasks. The first and repeated native Rules constructors perform
equivalent header work but arrive at the epoch load on different sides of
NMI. In the header's M=0 caller, `$80:86B0-$86BE` waits for a change from the
16-bit `$0564` value loaded at `$86B4`, not one host frame. The routine itself
preserves incoming M; `$8959` calls with M=1 and compares only the low byte.
`$80:84A3-$84AA` increments that counter only
if `$059C=0`. Publication precedes the increment; callbacks/audio and RTI
follow. Main execution cannot resume at the increment while NMI still runs.

The retained independently audited C reentry failure is first mapped state
at native1176/C873 (brightness), first visible RGB difference at1180/C877.
It is downstream of the following causal header wait difference:

| Header invocation | Entry label/line/epoch | Before wait label/line/epoch | After wait label/line/epoch |
| --- | --- | --- | --- |
| First Rules |541/198/71|542/2/72|543/247/73|
| First Main return |883/160/15|883/212/15|884/246/16|
| Repeated Rules |1171/172/71|1171/224/71|1172/245/72|
| Repeated Main return |1513/39/15|1513/91/15|1514/245/16|

Each header does two fixed-source 4096-byte DMAs (source `$00:0016` to
VRAM low port, then `$00:0017` to high port), followed by a14-byte palette
DMA. In first Rules, the second fill begins line224 and delays pending NMI
until line249; NMI increments epoch before the later wait load. In repeated
Rules, both fills and palette complete in time to load epoch71 at line224.
The rings are empty and DMA operands agree in these compared intervals.

No-NMI header entry to pre-wait is exactly440 native CPU cycles. Its master
durations are70526,70526,70566 for invocations2,3,4. First header includes NMI
and takes90134 master clocks/3435 CPU cycles. Counting CPU cycles alone, or
multiplying them by one constant bus speed, is insufficient.

Independent agent `config_audit` personally inspected the raw trace/ROM and
confirmed that after subtracting chronological NMI entry-to-exit intervals,
backdrop entry to header entry has identical550769 CPU cycles for both Rules
visits, but3830930 versus3830998 master clocks. Included NMI work differs:
375808 versus353564 master clocks (57573 versus54210 CPU cycles). Both Main
returns have550788 non-NMI CPU cycles, with3831112 versus3831118 master
clocks. This supports carried phase plus producer/DMA and variable interrupt
work as the dependency; it does not supply a finished production cost model.

Consultant interim refinement, independently inspected on the same retained
evidence: all Rules-backdrop NMI entry-to-epoch-after-guard intervals are
exactly3048 master clocks; `$05C2/$05C5` callbacks are `$80800C` throughout.
The22244-master-clock difference lies entirely after the increment, in
controller polling `$80:CB8F`, sound-driver `$80:A137` and interrupt return.
Do not attribute it to queue service or a varying Setup callback. Full
cost/last-synchronizer diagnosis is still in progress.

Further consultant correction: the raw `nmi.entry` hook is `$815A`, but
the native vector `$FFEA` targets `$8156: JML $80815A`. Subtracting only
logged entry-to-exit misses8 CPU cycles of interrupt entry,4 of vector JML,
and7 of RTI,19 per interrupt. Subtracting these as well gives **550560
producer CPU cycles in all four backdrop-to-header intervals**: Rules
550769 minus11*19; Main550788 minus12*19. First header3435 minus2976 logged
NMI cycles minus19 is440. Earlier raw-interval numbers above remain valid
interval sums, but must not be called complete non-interrupt work. This
strengthens an exact bounded producer work model; it still provides no
single master-clock multiplier or release timing rule.

Separate remaining hidden-VRAM failure: first Custom return870 differs at25
byte values /30 completed write positions. That split fixed-source DMA
phase failure must not be masked by the visible RGB match.

## Evidence-led hypotheses already checked

1. **Visit flag or Custom-specific delay.** Rejected: entry registers, empty
   queue, actual transfers and the wait counter show the extra epoch arises
   when DMA crosses NMI. No visit counter owns this wait.
2. **Same header CPU work means same host-frame delay.** Rejected: the three
   non-NMI calls have440 cycles but different master duration; first call
   services NMI before the load. Header elapsed-frame count is an outcome.
3. **Queue contents or transfer sizes differ.** Rejected for the causal
   header interval: same two4096-byte fills and14-byte palette DMA; empty ring.
4. **Different backdrop algorithm/path accounts for repeat difference.**
   Independent non-NMI subtraction shows identical550769 CPU cycles for the
   two Rules calls, while interrupt cost and starting phase differ.
5. **An epoch wait primitive alone repairs production.** Rejected: it must
   receive the epoch at the actual load after real producer work. Feeding
   captured header phases to it would only replay evidence, not predict a
   normal runtime path.
6. **Existing Ghidra/recomp master-cycle accounting is a sufficient timing
   oracle.** Rejected: generated code adds block cycles times one memory
   speed, while DMA/refresh/NMI work requires distinct accounting. Also the
   captured-address Ghidra listing has mixed-width errors: at `$80:8332`,
   bytes `C9 D6 F0 42 BD 01 01` mean M=1 `CMP #$D6`, `BEQ $8378`,
   `LDA $0101,X`, not the listed `CMP #$F0D6`. Correct widths and raw bytes
   must govern any further source derivation.

No empirical skip, phase offset, cropping, extra blanking or replacement
native golden has been tried or accepted.

## Precise expert question and falsifiable next check

What is the smallest source-equivalent portable task/work contract that
predicts the epoch loaded at `$80:EF1A -> $80:86B4` for all four invocations
from real constructor entry state, without captured timing playback or an
arbitrary CPU interpreter in production? Inspect `$80:EC68-$EEC0`,
`$80:EEC6-$EF1D`, decompressor `$80:C62B`, immediate DMA `$80:8AD2-$8B34`,
palette `$80:8A02-$8A56`, NMI `$80:815A-$859B`, and their actual callers in
Main `$81:BA8E` and Rules `$81:CF62`.

Determine whether proven synchronization boundaries allow a smaller task
continuation model, or which exact decompressor/interrupt work and bus
state must be carried. State confidence, missing observations and a
falsifiable next capture/check. A useful proposed contract must predict
loaded epochs72/15/71/15 and after-wait73/16/72/16 without using invocation
number or its captured scanline as production inputs. Independently replay
the unchanged natural input journey after implementation. Queue/wait leaf
PASS is explicitly insufficient.

## Retained artifacts, identity, and reproduction

Let `R=C:/Users/joshs/Projects/nba-live-95-c-port` and
`W=R/.analysis/worktrees/completion-scheduler`. Original ROM is
`F:/Games/SNES/NBA Live 95 (USA).sfc`,1572864 bytes, SHA256
`2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`.

| File | SHA256 |
| --- | --- |
|R/.analysis/transition-ownership-20260830/phase-probe/reference/bank80.c|7aee1b321de7cb6094dc4529269392f84303a4bd8c43727fbb1d2c68a3520e48|
|R/.analysis/transition-ownership-20260830/phase-probe/reference/bank81.c|145591f4d904f6fd9b30c51877542b1260078a9a4f2305518a0340f961b098fb|
|R/.analysis/gameplay100-closure-ghidra/gameplay100_bank80_listing.txt|77df90a3689c2dfa6b2b94fca16481ed70ae231c8415a22b32eea2fd91c095b3|
|R/.analysis/setup-scheduler-20260830/native-v1/manifest.json|aeef6ad31ddc66055c5ab709e60d0f060357813cd29bc717fb8a229accf616d6|
|R/.analysis/setup-scheduler-20260830/native-v1/scheduler.jsonl|df11518835e80b52d5ed807ed36af667d50a5af73cb619a7ce5c6473ebbe10e2|
|W/.analysis/native-scheduler-v2/manifest.json|fcadf46241fb7637196ef093b833dbabfb3c6b044d1053018af446d29b111a8b|
|W/.analysis/native-scheduler-v2/scheduler.jsonl|191426aed98817bf15af3babb37275d778eef202b659a29fc3f78eb09aa0da88|
|W/src/nba_setup_screen.c|23072ed13bc4a79bb805b200503fb82de69cd0af94487a6dd000dd90273436d9|
|W/.analysis/native-scheduler-v2/capture.lua (frozen script)|491b4cb95a3bd5bf3c4f9162419b4a02c11cb3c2028ecb2f376bf5a974778ab4|
|W/tools/capture_setup_scheduler.py|cb593c851adcdf42ece73886800718dd64d0574f6e8b28bbe79a2198b4bcd91c|

Native-v2 completed with exit0, sentinel, private portable Mesen/home/save
directory and observed environment/settings checks. Canonical Mesen2.1.1
binary SHA256 is `d2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b`.
Both captures use natural controller-only Simulation/3min normalization then
A470, row2Right640, Start830, A1100, row2Right1270, Start1460. Labels rebase
after normalization; execution is continuous. No CPU/RAM/PPU/ROM injection,
savestate load, timing override, screenshot playback or global settings edit.

V2 adds actual `hclock`, DB,512-byte queue at NMI publication, palette/source
operands and four complete `header_XX_before_wait.wram` dumps. Existing full
entry/after-wait128KiB dumps remain. It corrects the diagnostic-only false
fill-exit hook `$808AD1` to the actual `$808B34` RTL. Hook additions do not
alter emulated work; old field invariance must still be checked explicitly
before broadening claims. Manifest acceptance denotes launch/capture
integrity, not a fully audited scheduler model.

Fresh reproduction from W, choosing a new unused output directory:

```powershell
python tools/capture_setup_scheduler.py --output '.analysis/native-scheduler-new' --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --mesen 'C:/Users/joshs/AppData/Local/Microsoft/WinGet/Packages/SourMesen.Mesen2_Microsoft.Winget.Source_8wekyb3d8bbwe/Mesen.exe'
```

The existing C failure replay is `tools/test_setup_rules_reentry.py --exe
<fresh-exe> --rom <ROM> --pack <candidate>`; root is repairing its driver and
configuration preconditions before rerunning. Do not treat failure before
menu entry as evidence for this scheduler bug. Historical actual C and
independent full-VRAM evidence live in
`R/.analysis/rules-resource-independent-20260830/` and
`R/.analysis/transition-ownership-20260830/` (see
`docs/rules-publications-independent-audit.md`).

| Inclusive canonical ROM range | Bytes | SHA256 |
| --- | ---: | --- |
|8086B0..8086BE|15|956b492ae0fa65228db2be9c20fdbc48d28c774d19024167a236b17cbb31cb13|
|8084A3..8084AA|8|373496bf0aa90883aead59c65335eef35b63471023ab85e4a50895c1ca67750c|
|80EEC6..80EF1D|88|6108dd4daa115203258e92d1909c229dd91e6731294fd0ad8371d4321c4f3956|
|80EC68..80EEC0|601|5395b36c7c5cdfceaedb295ea0d4bef9b949798cc1092237df41c7f351b112a6|
|808AD2..808B34|99|f0bc43ef1ae47e1e21f73185586f049a50bfe0cd636ffb9308a3ebe99c90372f|
|808A02..808A56|85|51e899c4a4be96c8fd2e0f2d1866988ff373d61ee7dfa0042ca08a61f08318ef|
|808332..80833D|12|6f1b582c2617ba114de48cc729fa7a4b7ad1ca20fb64e251e59edc61cd846df7|
