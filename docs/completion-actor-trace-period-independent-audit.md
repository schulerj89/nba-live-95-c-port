# Independent actor execution / period observation audit

Accepted for the bounded host diagnostic and test corrections below. This is
not native scheduler phase, whole gameplay, Rules reentry, human dispatch,
camera trajectory, or full CPU regression acceptance. No original-game bug
was changed; the defect was the port's parity-only execution report.

Reviewed the owner's immutable `build/actor-trace-period-freeze-v1.json`,
SHA256 `a0ba08edaf1515ce6ebdcf3485edf5d451fedfc691c0485653d620267e401f97`.
All 429 identities (2,962,865,544 bytes) independently rehashed. Review copies
are under `build/actor-trace-attribution-v1/review`; later mutable owner
oracles are not part of this verdict. Auditor evidence is under
`build/actor-trace-audit-v1`.

## Source and caller findings

Only `src/nba_tipoff.c` and `include/nba_tipoff.h` differ between the paired
complete source/header snapshots. The appended flag is cleared at initialized
`nba_tipoff_update` entry (9024), before pause and period returns; it is set
immediately after the existing `actor_update_tick` increment in
`cpu_update_all_actors` (7092). The unchanged function dispatches slots 0..9
after its even-tick guard. `nba_tipoff_capture_telemetry` (9328) projects that
observation into the existing delta 2, mask 03FF and ordered slot fields.
The new flag controls no gameplay branch. The free-throw early return at
`cpu_update_possession` is correctly outside this physical pass.

The initialized-update qualification matters: a null/uninitialized object
returns before the diagnostic reset. No telemetry validity for such an object
is added here. The appended field occupies previous zero padding in this
matched MSVC layout; this is not a portable ABI/serialization promise.

The capture's actual `$85:963D` callback exists in
`tools/mesen_tipoff_capture.lua:356`. This review approves truthful **C**
execution reporting, not equivalence between native callbacks and a complete
C frame schedule. No ROM, native fixture, asset, arithmetic quirk, or original
gameplay branch was edited in this checkpoint.

The camera model at test lines 315..318 now distinguishes a lifecycle return
from entry into the camera wait. It retains an already pending wait. The
scheduler section retains ordinary even-tick phase, exact delta/mask/order,
and possession non-rephasing for eligible adjacent calls. The shot section
at 1027 excludes cross-period transition classification after current-state
carried-ball assertions, so rebuilding mode 13 into mode 1 is not described
as a physical release. No other test oracle was changed in the frozen copy.

## Independent execution

Built all 40 frozen sources, with matched headers and the frozen diagnostic
driver, into private `/W4 /WX /O2 /MD` objects. No owner build objects were
reused. New executable SHA256:
`67510454c5d129457d3c45458eee558f8febfaa5a1f070b81f0d1d93fd69f7e0`.

- Fresh 63,800-frame replay: every trace byte and every actual PHYSICS
  observation matches the frozen corrected run. Trace SHA256
  `d16bd996fad1044603c9694597a36452023a56560e364233a0dc4b762c98c6ab`.
- Streaming old/new comparison: exactly 1,189 scheduler objects differ,
  frames 48224..49412; every other serialized byte is identical. All 31,305
  existing counter increments match the new flag and complete projection.
  This trajectory has no free-throw diversion differences.
- Exact frozen scheduler AST: 63,800 rows / 114 possession changes pass;
  original parity-only trace fails at 48224; seven corruptions reject.
- Camera model versus actual owned wait state: 63,581 observations pass;
  original 49413 failure reproduces; eight corruptions reject.
- Exact shot AST: 77 starts / 76 releases pass. The separately evaluated
  frozen inbound section still fails its stale layout-1 oracle at frame 506;
  this retained failure is not hidden by this acceptance.
- Fresh camera probe passes both pending and absent waits through a complete
  controlled period presentation/restart while preserving all earlier cases.
  Fresh observer probe passes 2,000 paired updates, matching callback counts,
  all published fields, observer/no-observer state bytes, and pause behavior.
- Fresh initialization probe passes all 841 pairs; all serialized owned-state
  records match the previous accepted fixture byte-for-byte, SHA256
  `68164c0eed462138d257f21dc60fc2bdd86699125e1da64cc0acfa8bba09f38f`.
  Host pointer identity is not included in that owned-state assertion.
- Fresh unchanged gameplay85 three-digest probe passes 48,000 frames; closure
  still passes `d26e6deec1fdc18e`, 8 transitions / 65 renders / 71 possessions.
- Fresh all-source executable passes all 303 EA-motion and five text-phase
  native image comparisons using the explicit existing capture directory.
  `build.ps1` adds only that `--native` argument. It does not alter phase
  mappings, expected pixels, cold-boot timing, input or audio acceptance.

Exact frozen test functions/AST were evaluated with only explicit audit output
and immutable input path substitutions. Read-only Python import dependencies
are copied and hash-recorded in `dependency-identities.json`; none replaces
the reviewed test functions. Initial audit-copy build failure (missing
`main_observed.c`) and initial import failure (missing transitive helper) are
retained. Corrected private outputs use `compiled-v2`, `fresh-probes-v2`, and
`section-replay-v2`; original owner failures/evidence remain unchanged.

## Reviewed source identities

| File | SHA256 |
| --- | --- |
| `src/nba_tipoff.c` | `7b770a13ced06625e7e00b4e168e708865d29c283ef3fd8b9d1164fe9d9b5ea5` |
| `include/nba_tipoff.h` | `f50935738653eddd6a7bf1fc6e3840732179b4d4850359e10dd0af0694e3e290` |
| `tools/test_cpu_gameplay.py` | `d1a8961fcfc24799e4afcb6053ce0cdb54b3d74d4a3c8ed6f21aca51ff4926e0` |
| `tools/camera_handoff_runtime_probe.c` | `6b7396b839514827d5f85f5741aaede0961474bdc019f79b5bc274cfd4506128` |
| `tools/differential_observer_probe.c` | `4cc3a594090630c61760bb76afff09348bec891efd18f611ffe6ff0b18c52d49` |
| `build.ps1` | `a70bf34465707b815ae1cf7605c07e96967faa5e8d87317afee8ca2afca57e02` |

Remaining claims stay excluded: whole-native timing/state, normal human play,
Rules reentry, stale HUD, later inbound/image oracles, and full-suite closure.
