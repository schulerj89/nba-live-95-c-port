# Rules reentry resource and scheduler audit

Status: bounded resource/cadence repairs are under review. Complete Rules
reentry remains **FAIL**. This work follows checkpoint `e1bc0d4`; it does not
replace the independent first-opening/return pixel witnesses with C output.

## Correct native owners

| Behavior | ROM owner / state | Portable implementation |
|---|---|---|
| Main / Rules construction | `$81:BA8E-$BD22` / `$81:CF62-$D31E` | `setup_begin_page_transition`, captured resource publications |
| Visible exit | `$81:C41E` → `$80:EBF9-$EC67`, `$80:EB27-$EB9E` | transition PPU/IRQ contract |
| Incoming layers | `$80:EA99-$EADD`, `$80:EADE-$EB26` | transition PPU/IRQ contract |
| Backdrop / header resources | `$80:EC68-$EEC0` / `$80:EEC6-$EF8D` | `NBSPPU3` native VRAM publications |
| BG2 cadence | `$81:F9FC` → `$87:89D5-$89E8`; `$168F` modulo 3, `$0613` counter | `setup_advance_steady_bg2` and live exit carry |
| Counter reset | `$80:EB91` writes `$0613=0`; `$80:89BD` writes `$0613=$FFFF` | captured blank-stage state; phase scheduling still incomplete |
| First Rules text / later lower rows | `$81:D117-$D15B` rows 2–8 / `$81:D21D-$D261` rows 8–12 | scoped runtime value-glyph publication |
| Font DMA queue | `$81:A1EE/$81:A28E`; canvas begins `$7F:2360` | raw DMA coverage plus value-bit replacement |

The old `$80:A2BF` graphics-builder label was false. Original bytes and a
correct M/X decode identify an eight-slot audio-loop tail, executed eight
times per frame with X=0,2,…14. `$80:A3B8` was not reached in the sampled
transition windows. A PC appearing in a screen's execution census did not
prove it owned the screen. Old documentation's “nothing inferred” claim was
therefore not supportable. Main input's real branches are `$81:BDA8/$BDD5`
and `$81:BDEA/$BE2D`, independently decoded by the configuration workstream.

Reference corpus: `.analysis/transition-ownership-20260830/phase-probe/`.
`reference/bank80.c`, `bank81.c`, `bank87.c` are fresh generated recompilation
from the canonical ROM with explicit M/X entry states. The
`gameplay100-closure-ghidra` bank80/bank81 listings support the constructors
and graphics routines. Native `native-producer-v1/producer_calls.csv` and
`native-repeat-v1/counter_calls.csv` provide caller, scanline, cycle and
pre-call `$168F/$0613` state. These are natural controller journeys, with no
CPU/WRAM/PPU state injection.

## Repairs and their limits

1. The outgoing Rules entrance carries the existing BG2 phase rather than
   replaying the first visit's increment pattern. Native first A dispatch470
   begins phase1; second dispatch1100 begins phase0. Phase is not reset during
   forced blank. The first 52 second-visit frames1100–1151 now match pixels
   and BG2 state exactly.
2. A second visit preserves the preceding Main constructor's whole live VRAM
   and CGRAM, including currently invisible data. The initial capture is no
   longer substituted for that previous production state.
3. Current Rules values populate the outgoing canvas. Current Main values
   populate the incoming font canvas. Their glyph changes publish when the
   native font DMA writes their bytes, not eagerly at every return frame.
   The public Rules helper is shared with the configuration workstream; it
   uses ROM-derived 2bpp glyph cells, with 19-line shadow overlap. This is
   not yet a translation of the original general proportional-font builder.
4. Snapshot differences omit a DMA write when its value equals the old
   snapshot. That omission left changed glyph bits uncleared on a live
   edited page. `NBSPPU3` records all observed destinations, including zero
   writes, so a native clear really clears the edited canvas.

`NBSPPU3` keeps the 16-byte trace header and 34-byte state record. Each frame
has 16-bit VRAM/CGRAM counts; VRAM records are address16/value8/scope8.
Scope0 is an unmodified native publication; scope1 is the first Rules font
generation; scope2 is the complete Rules/Main font generation. CGRAM records
remain address16/value8. Only the glyph bits differing from the baseline are
substituted, preserving native staged text and unrelated VRAM. Options
remains on version2 and is outside this new coverage claim.

The source font transfer sequence is checked: Rules has two groups of
4096/4096/2048 bytes from `$7F:2360/$3360/$4360` to VRAM byte
`$8370/$9370/$A370`; Main return has 4096/1024 from the first two sources.
All observed VRAM changes must have a recorded DMA owner. Fixed-source
fill bytes must agree with the resulting native snapshot. Parsers reject
missing, reordered, duplicate, incomplete, unsupported and unattested jobs.

## Reproducible capture and extraction

Use a new output directory; never overwrite a native capture:

```powershell
.\tools\capture_setup_transition_exact.ps1 -OutputRoot '<new-folder>' `
  -Menu rules -SimulationThreeMinute -HoldMenu -ResourcePublications
.\tools\capture_setup_transition_exact.ps1 -OutputRoot '<another-new-folder>' `
  -Menu rules -SimulationThreeMinute -TargetRow 2 -TargetRights 1 `
  -RepeatVisit -ResourcePublications
```

The runner copies Mesen to a private portable home, sets the controller and
frame-skipping configuration explicitly, and supplies a **per-child process
environment**. It never mutates the shared PowerShell process environment.
The Lua script records the folder and profile it actually received; the
runner verifies those observations. This fixes a demonstrated race between
parallel PowerShell runspaces. The ambiguous attempts
`native-coverage-hold-v1` and `native-coverage-repeat-v1` were marked INVALID;
their files are not accepted production evidence.

Authoritative tracked-script capture:
`phase-probe/native-publications-tracked-hold-v1`. Its raw resource jobs,
segments, VRAM snapshots/differences and PPU states are byte-identical to the
earlier valid `native-coverage-hold-v3` read-only probe. The manifest attests
ROM, private executable, immutable script, settings and raw resource files.
The normal extractor requires `.analysis/setup_rules_publications` and fails
if its attested jobs are absent. The local durable junction points to the
immutable tracked hold capture; `Path.resolve()` preserves the manifest's
actual original directory. Explicit `NBA95_RULES_OPEN_CAPTURE` and
`NBA95_RULES_RETURN_CAPTURE` may select another validated capture.
Set those in the extractor child's environment, not the shared parent.

The existing `extract_assets.py` entry point performs validation and builds
the new trace when these attested publication files are present. No RGB,
PNG, recorded audio, or new asset ID enters the pack. For root integration,
rebuild the current full pack; do not promote this worktree's old version30
whole pack over newer independently integrated assets.

Local comparison pack:
`nba95_assets_rules_publications_candidate.pak`, SHA256
`1f8984f556f03d167a18bc7ccfd85cb26d19c1870c812b198ff0ffe4914fa38a`.
Only entries145 and155 differ from the frozen checkpoint pack. They contain
132,008 and103,784 VRAM publications respectively. Entry145 is534,016 bytes;
entry155 is420,252 bytes. Capture-derived RGB remains evidence only.

The DMA observer samples `$420B` submissions and endFrame progress. It
supports the observed contiguous mode1 and fixed-source mode8 addressing,
increment1/remap0. For a split fixed-source DMA, current PPU word address
counts completed bus writes. An earlier explanation that Mesen decremented
remaining size before the bus write was unsupported and is withdrawn; source
review places that decrement after CopyDmaByte. General mode1 bus-phase
parity remains unverified. The observer rejects other addressing contracts rather
than guessing. A single submission with multiple VRAM channels is rejected. Split mode1
DMA cannot be exported: its pending low-byte bus phase is not independently
observed. The repeated-return diagnostic has such a split at1511, which
remains outside the production trace contract. No general SNES DMA emulator
is implemented.

## Verification and remaining failures

No witness or golden hash was replaced in this workstream. Focused existing
147-frame opening, both171-frame return,137-frame held-menu and all four
UI snapshot gates pass with the local candidate. Existing27 verifier tests
and11 publication-verifier tests pass. The C preflight rejects malformed
NBSPPU3 streams before mutation;40 positive/corrupt-native-stream decoder
checks cover PPU domains, flags, address order, duplicates, scopes, CGRAM
bounds, frame population, truncation and trailing data. The original reentry gate still fails
158 RGB/state checks, first1176/C873 brightness. The new synthetic resource-verifier tests check write intent and
malformed evidence; they are deterministic engineering tests, not ROM parity.

The natural repeated journey and the actual C executable use the already
documented fixed input schedule, without a timing search. The resource report
is `.analysis/transition-ownership-20260830/reentry-publications-report.json`.
`reentry-byte-comparison.json` independently corroborates every VRAM result
with direct65,536-byte dumps from a private read-only observer; its entire
telemetry trace is byte-identical to the unmodified executable's trace:

| Comparison | Full 64KiB VRAM | RGB | Scope |
|---|---:|---:|---|
| First Custom return830–962 |132/133 exact|133/133 exact|one natural row2 OFF return|
| Second opening1101–1246 |136/146 exact|97/146 exact|**FAIL**, constructor scheduling|
| Second return1461–1592 |122/132 exact|132/132 exact|remaining hidden publication timing differences|

Each denominator is the stated frame range, not all transitions, inputs,
configurations, routine branches or internal states. RGB compares all57,344
visible pixels per frame. VRAM compares all65,536 bytes independently of
visibility. First return830 and second opening1100 are dispatch snapshots;
other state rows are native post-dispatch observations.

First Custom return870 still differs at25 VRAM bytes (30 different
completed write positions, five of which already contained zero) because the native
fixed-source clear is interrupted at a different bus position at endFrame.
No compensation envelope masks that difference. Second opening first visible
divergence is1180. Native header `$80:EEC6` starts first541/scanline198 and
repeat1171/scanline172. Its palette upload reaches `$80:EF1A` → `$80:86B0`
at first542/scanline2, but repeat1171/scanline224, just before the next NMI.
The latter therefore resumes one frame sooner. It is not evidence for a
“second visit” or “Custom” delay rule.

A complete fix needs a portable scheduling contract for actual producer
work, queued DMA, NMI boundaries and interacting tasks. Derive any work
budget from decoded instruction paths and observed boundaries; do not fit
frame counts. The current implementation still replays a captured schedule,
so its constructors and reentry routine are **not complete**.

Fresh tracked repeated capture `phase-probe/native-publications-tracked-repeat-v1`
reproduced all1,178 raw RGB files and22 resource/state files byte-for-byte
from the previous valid probe. See `phase-probe/tracked-repeat-reproduction.json`.
The unchanged input/source observations are independent of C output.

A second fresh held capture, `phase-probe/native-publications-tracked-hold-v2`,
uses the final multiple-VRAM-channel rejection guard. All12 raw resource/state
files and548 synchronous RGB files equal the previous valid capture exactly.
`resource-guard-capture-reproduction.json` records that comparison. The strict
normal-default extractor path still reproduces entries143/144/145/153/154/155
byte-for-byte. Raw memory-difference rows are validated before clipping to a
production range; duplicate, reordered, out-of-range and malformed rows are
rejected even outside the selected frame interval.
