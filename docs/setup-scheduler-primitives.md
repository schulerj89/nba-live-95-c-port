# Bounded Setup queue and epoch primitives

The latest verifier revision is recorded in Git history.
The independent audit rejected freeze-v2's verifier integrity despite its
passing bounded primitive replay. The new freeze retains source behavior,
repairs the verifier/probe boundary, and has a fresh native replay plus32
local tests and32/32 auditor-authored mutation cases. Historical identities
and results below identify the earlier bounded source revision.

Date: 2026-08-31. These primitives are implemented, freshly compiled and
bounded-native compared, but **not production-wired**. They are outside
`nba95_sources.txt`. Repeated Rules entry and partial-DMA scanout remain
FAIL. No change was made to the production Setup renderer/transition
driver, pack, native fixture, or original input schedule.

`include/nba_setup_scheduler.h` and `src/nba_setup_scheduler.c` provide two
pieces needed by a future source-derived resource-task scheduler:

* `$80:86B0-$86BE` epoch-load/wait readiness, preserving incoming M width,
  with 16-bit counter wrap and main
  continuation held until NMI has returned. `$80:84A3-$84AA` increments
  epoch only when `$059C=0`. The caller must maintain interrupt phase and
  run publication before the increment and callback/audio work afterward.
* `$80:821A-$83CE` queue publication and budget/cursor state for native
  DMA record types1, `$FD`, `$FC`, and source-translated `$D6`. Records
  retain native source addresses; publication sinks resolve ROM/WRAM
  resources. No RGB frames, captured commands or captured frame timings
  enter this module.

The queue's head is published only at its native service exit, not between
jobs. Failed budget checks preserve the stopped job's budget and cursor.
The `$80:8332` branch is decoded M=1: `CMP #$D6`, not the old mixed-width
Ghidra listing's `CMP #$F0D6`. VMAIN action `$FF` means leave unchanged;
special low-fill `$00` and increment32 `$81` are restored to `$80` after
the DMA, matching the native branch instructions. The sink represents
publication intent; this module neither simulates DMA bus progress nor
accounts for CPU/refresh/NMI elapsed work.

Native quirks are retained and commented. `$821A` tests only the low byte
of the palette byte-count, while the later DMA reads the full word; a
count256 therefore skips this service. The palette budget uses the actual
`SEC; SBC size; SBC #$50` sequence, including borrowing into the second
subtraction. These edge values have explicit C/source checks below, not
claims of natural gameplay observation. Future integration must preserve
confirmed original bugs as requested by the user.

The wait routine also preserves caller M width. Header `$EF1A` uses M=0,
but `$8959` uses M=1. The natural capture at labels867/1497 has epoch397
while the byte load is141. This caller behavior is retained explicitly;
an apparent high-byte-only epoch change cannot release an M=1 wait. A
consultant's actual-ROM review caught this generalization defect in the
first primitive; its earlier four-header-only comparison never covered it.

## Evidence and verification

Fresh natural capture:
`.analysis/worktrees/completion-scheduler/.analysis/native-scheduler-v3/`
under the primary repository. The isolated Mesen launch completed exit0
and passed sentinel, observed private home/output, fresh saves and settings
checks. V2 added queue bytes, hClock, DB and before-wait full WRAM. V3 also
observes actual CGRAM address and VRAM increment/remap at each DMA. Native
controller input and normalization are unchanged. The false diagnostic
fill-exit hook `$808AD1` is replaced by actual RTL `$808B34`.

The native-v3 manifest SHA256 is
`e8bf2695885be8514b0de302e5c467966fe11ca6c6a985d0ada7d888339d6e43`;
its6235559-byte `scheduler.jsonl` SHA256 is
`1ce0158650dc48ff7a35a6238d6dce9af8b3d720cfbea56e3d8d596376ad0543`.
All7080 events from immutable primary
`.analysis/setup-scheduler-20260830/native-v1/scheduler.jsonl` retain every
old observed field and master-clock value (except the sequential event
index, necessarily shifted by22 newly observed fill returns).
V3 also preserves all7102 V2 events/old fields exactly. All eight original
entry/after-wait128KiB WRAM dumps equal V2 and V3 byte-for-byte.

Fresh standalone build is produced by
`tools/build_setup_scheduler_probe.ps1`, compiling the module and probe
from source with warnings as errors. It does not reuse game build objects.
Its build manifest ties executable bytes to the exact module/header/probe
and build script used. `verify_setup_scheduler.py` rehashes those sources,
the executable, canonical ROM, capture sources/artifacts, and validates
capture completion before comparing every expected field.

| Gate | Result | Actual scope |
| --- | --- | --- |
| Native queue service |PASS599 dispatches|587 normal exits,12 budget stops; exact final head/budget/palette size|
| Native queue publications |PASS126 jobs|102 mode1 copies,10 low fills,10 high fills,4 palette DMAs; exact order/type/source/size/observed PPU destination and VRAM increment/remap|
| Native header waits |PASS4 calls|loaded epoch and interrupt gating at observed boundaries; no prediction of producer elapsed time|
| Native byte-width waits |PASS16 calls|incoming M=1 preserved; exact native loaded-byte/resume protocol|
| Capture preservation |PASS7080 V1 and7102 V2 events|all old telemetry values and timestamps unchanged by new observation hooks|
| C edge/integrity tests |PASS22 tests|11 controlled C/source edges and11 verifier integrity tests; not natural ROM parity|

Report `.analysis/scheduler-primitives-v4.json`, SHA256
`8bce08d1e9bc4be4c0ca4406e11fe934f391cb04c9bd7d554e0d3cc905a19f47`,
records the exact final build identity, queue results and wait boundaries.
Executable SHA256 is
`3ee595c5c735daf27a13ad0b66ce689d25811d09fb40966ab151223cff87e9e0`.
Earlier reports and freeze-v1 are historical; they do not identify this
final revision. The first
engineering mutation test selected an unrelated NMI OAM DMA, outside
this queue's observed region, and correctly did not affect the queue
comparison. That test was fixed to select an actual queue DMA between
`nmi.before_publish` and the queue exit, assert membership, then delete it.
It now fails the semantic comparison as intended. No assertion was weakened.

Reproduction from the scheduler worktree (choose an unused report path):

```powershell
.\tools\build_setup_scheduler_probe.ps1
python tools/verify_setup_scheduler.py --native .analysis/native-scheduler-v3 --previous-native 'C:/Users/joshs/Projects/nba-live-95-c-port/.analysis/setup-scheduler-20260830/native-v1' --previous-native .analysis/native-scheduler-v2 --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --exe build/scheduler-probe/setup_scheduler_probe.exe --report .analysis/scheduler-primitives-new.json
python tools/test_setup_scheduler.py --native .analysis/native-scheduler-v3 --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --exe build/scheduler-probe/setup_scheduler_probe.exe
```

In each shell, stop after an unsuccessful command; do not allow a later
success to mask it. Source/artifact hashes and bounded reentry diagnosis
are retained in the historical scheduler report in Git history.

## Explicit remaining limits

This is not a completed NMI routine, constructor, general DMA engine,
native scheduler or frame/scanout timing implementation. `$D6` was not
observed in this native corpus; it has source/controlled C coverage only.
Other queue types are explicitly unsupported. Register side effects before
an exhausted-budget branch, OAM upload, HDMA, palette/VRAM data bytes, bus
phase, shared audio/RNG work, queue producers and all natural transition
callers are outside this primitive comparison. The four wait cases import
native boundary epochs; they do not compute those epochs from producer work.

An independent review is required before accepting the primitives for
integration. A complete repair needs the carried producer/interrupt phase
contract in the blocker record, followed by ordinary menu-path replay,
full live-resource comparisons and consecutive frame parity. Do not wire
this module by applying captured entry times, adding a visit rule or
advancing/skipping frame records to fit the existing witness.
