# Independent brightness and golden-delta audit

**PASS, bounded scope.** The repaired production converter and Python snapshot
renderer each reproduce all 1,536 independent Mesen channel/brightness samples.
This verifies RGB555 INIDISP scaling followed by default RGB expansion; it is
not proof of complete transition timing, color math, or a hardware SNES capture.
The auditor did not edit either renderer or generate expected RGB from them.

## Native experiment and provenance

`tools/mesen_ppu_brightness.lua` leaves the original ROM program, CPU registers,
PC, and WRAM unchanged. The ROM executes normally. At startFrame it controls
only `$2100,$212C,$212D,$2130,$2131,$2133` and CGRAM backdrop color zero. This
disables layers/color math and sets one R/G/B channel to each of 32 levels,
with each of 16 brightness values: **3 × 32 × 16 = 1,536 cases**.

The canonical run is `.analysis/ppu-brightness-native-20260830-portable/`.
The launcher copies Mesen into a private directory and places actual portable
settings beside it before startup. Lua `getScriptDataFolder()` independently
confirms that home. An empty private save directory, fixed controller ports,
zeroed power-on state, disabled frame skipping, default unfiltered color,
rotation off and standard 256×239 output are documented in the manifest.
The verifier rechecks the persisted nested settings, including controller
types and overscan. Earlier exploratory runs used global settings and are not
the accepted corpus; command-line string save-folder overrides were ineffective.

`getScreenBuffer()` samples the synchronous rendered PPU frame at endFrame.
The five points are `(0,7),(255,7),(128,119),(0,230),(255,230)`; output rows
7..230 are the normal 224 visible lines. No expected conversion formula occurs
in the Lua source or its acceptance decision. This records **7,680 observed
native samples**, not 7,680 independent converter calls.

All three rejected attempts are retained, alongside all 43 native hardware
writes. Cases 996/997 observed conflicting native TM writes; case 1020 observed
CGRAM changes and nonuniform samples. The same input was retried only because
independently observed register/CGRAM state or raster uniformity failed the
requested experimental setup. All rejected reason bits and write conflicts are
recomputed by the verifier. Expected RGB equality is never used for selection.
Register callbacks include mirrored PPU banks and every CGADD/CGDATA command;
actual backdrop values at those commands expose intermediate CGRAM changes.

Identity:

- ROM SHA-256: `2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`.
- Installed Mesen: `2.1.1+137ae7ce3bf3f539d007e2c4ef3cb3b6c97672a1`;
  executable SHA-256: `d2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b`.
- Canonical Lua SHA-256: `8b48f17041263848f82f5c73401b96da5367ec3422efcdf57519c28e4017aed1`.
- Raw accepted trace SHA-256: `49c20ca7c7103e32805ee4f2d20f8e06983b395753e2db6f90b52888c80a1ba2`.
- Compact witness SHA-256: `30c0c45cf2820d488870df58e9a4f7a378925828c4f5e92e51ce44dad57dd782`.

`tests/fixtures/ppu-brightness-witnesses.json` retains all accepted row fields,
rejected attempts, observed native writes, source hashes and configuration.
Its expected pixels are a compact transcription of the raw Mesen data.
Fixture creation refuses an existing output path and precedes any C replay.

## Source review and verification

Independently inspected `SnesPpu::ApplyBrightness` and Lua `GetScreenBuffer`
at the exact installed Mesen commit, retained under
`.analysis/ownership-20260830/Mesen-installed-*.cpp`. The PPU operates on five-bit
channels before the video filter expands them. Reviewed the repaired
`src/nba_snes_ppu.c` and `tools/snes_ppu_oracle.py` against this order. The C probe
receives only color/brightness inputs and calls the production converter; no
native expected value is visible inside it.

Raw replay and compact-fixture replay both return PASS: 1,536 C comparisons,
1,536 separate Python comparisons, no mismatches. The previous eight-bit-first
formula disagrees with **1,122** native cases, exactly **374 per channel**.
At red18/brightness7 the observed channel is66; the old formula produces69.
This establishes why former C-versus-Python agreement was insufficient.

Eight focused protocol test methods reject missing/extra/reordered rows,
wrong grid values, Boolean numeric fields, wrong geometry/filter/controller
configuration, malformed or missing probe output, nonzero probe exit,
lost/reclassified rejected attempts and hardware conflicts. Deliberate
changes to each RGB output channel fail the comparison. These are synthetic
test-method challenges, kept separate from the native witnesses.

Reproduce after building the dedicated probe:

```powershell
.\tools\build_vector_probe.ps1 -Name ppu_brightness_probe
python tools/verify_ppu_brightness.py --vectors tests/fixtures/ppu-brightness-witnesses.json --probe build/ppu_brightness_probe.exe
python tools/test_ppu_brightness_verifier.py
```

The corpus does not cover addition/subtraction/halving color math, windows,
forced-blank behavior, sub-screen mixing, interlace, other filters, all palette
indices or native scene upload ordering. Samples validate five raster points
per controlled frame, not an exhaustive scan of every active pixel. The
independent natural Rules-transition evidence in `docs/ppu-brightness.md`
connects this shared conversion defect to a real scene.

## Initial Setup golden-delta checkpoint

**PASS for nine initial Setup goldens only.** Independently inspected
`tools/audit_brightness_golden_delta.py`, its source inputs, all retained BMPs
and logs. A separate auditor script reads the raw native samples directly,
obtains old expected hashes from Git commit
`2723af610aab0ec63263a6449fa6a161a155f974`, and maps every old pixel channel to
its observed native channel at the unchanged C brightness. Ambiguous mappings
are rejected. This uses no repaired C or Python conversion formula as expected
output.

All nine before images match the committed goldens. Only frame105 (20 pixels)
and frame118 (3,263 pixels) change; every changed channel follows the native
table. Frames104/125/128/130/146/162/166 are unchanged. Brightness and four scroll
fields remain identical. Fresh independent binary runs of frames105/118
reproduce both deltas; deliberate pixel corruption is detected. Source hashes
match before and after the audit. The reusable root tool now requires a new
output directory, all nine frame keys and pre/post source identity checks.

Independent report:
`.analysis/brightness-golden-audit-20260830/independent-fresh/auditor-report.json`.
Root hardened replay: `initial-setup-v2/report.json` under that audit directory.
These results authorize only the two conversion-caused hashes in that initial
Setup table. They do not authorize other golden updates, prove the native
transition's timing/content, or justify the prior full-frame parity comments.

## Options open/return golden-delta checkpoint

**PASS for the existing 13 Options-open and 10 Options-return frames.** The
auditor inspected `tools/audit_menu_brightness_golden_delta.py`, both pack
identities, executable identities and all 23 retained before/after BMPs and
logs. A separate script imports neither root's mapping code nor either repaired
converter. It obtains the original hashes from Git `2723af6`, reads the raw
Mesen samples directly, and checks every output channel. All old images match
their committed hashes; all five exposed brightness/scroll fields remain
unchanged. Source hashes were verified before and after this audit.

Only Options-open frame198 (25,170 pixels) and Options-return frame343 (5,312
pixels) change; every changed channel equals the native-table result. The other
21 frames are unchanged. Fresh separate binary executions of both changed
frames reproduce those results. Independent report:
`.analysis/brightness-golden-audit-20260830/options-independent-fresh-v2/auditor-report.json`.
The reviewed post-change executable SHA-256 is
`d3b5f30bc224faaa8e1a03a96aea5c3a095ac9f1332a4eaa5de36a08f15c4d52`.
This authorizes only those two Options hash changes, with the same conversion
scope limitations as the initial Setup review.

The same independent computation preserves five Rules-return failures:
frames319/320/329/345/350 change state/phase and contain 16,771/16,771/10,774/
1,779/156 pixels outside the brightness mapping. Those hashes are **not**
authorized by this gate. Frames351/352/353/424/450 remain unchanged; frame382's
2,191 changed pixels follow only the native brightness conversion. A separate
native route audit is required for the failed Rules-return frames.

Executable paths in reports identify files at audit time, and their SHA-256
identities remain authoritative after subsequent builds. The earlier
`93064d...` brightness-only binary was preserved by the integrator as
`build/nba95_port-brightness-only.exe`; do not rerun that historical report
against a later binary merely because it occupies the original path.
