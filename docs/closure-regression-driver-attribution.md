# Closure driver and C digest attribution

The closure probe now sends released frames between Main/Rules/Options
presses, asserts the native Main return row instead of assigning it, and
updates its separately attributed C-only digest. Independent review of this
checkpoint is pending. This direct component journey does not prove a normal
whole-machine journey, human gameplay or native scheduler phase.

The old driver sends four identical held Down words. The current native menu
producer accepts only the first edge, so the original probe stops at code12,
row1. `closure_press_setup` supplies one zero-held frame before each requested
press. It leaves the fixed four/five-step navigation counts and every expected
action, page, adjustment, commit and transition assertion intact. After
Options return, the probe requires row0 rather than setting it. It still runs
the same Team Select, Player Setup, introductions and6000 gameplay updates,
samples66 images, requires eight transitions and all existing semantic guards,
and compares two complete runs. No configuration is injected by the checked-in
test; it starts from the corrected native factory state.

That migration exposed the separate Philadelphia startup self-test defect
documented in `match-initialization-fixture-repair.md`. Fixing that isolated
precondition lets the unchanged selected away28/home19 pair reach gameplay.
No team is substituted in the real closure session.

## Controlled reproduction

Evidence is retained in `build/closure-migration-v1`. Private builds use
matched headers and objects. Historical primary objects are read-only;
52c2899 snapshots and all-current9f5b47c snapshots were compiled entirely from
source during gameplay85 attribution. Alternate tipoff/helper files are
compiled separately against their matching headers. Every current canonical
tipoff variant needs the same isolated startup test fix; it does not change
the match state. The old tipoff's self-test used the away roster and does not
fail this particular pair.

The private `legacy_profile_probe.c` adds only a labeled pre-init historical
Simulation/three-minute configuration override. It is a counterfactual, never
a production default or native input journey. All hashes, render sampling,
6000 updates and semantic guards are unchanged. Current runs apply no override.

| Source / configuration | Aggregate | Gameplay | Render | Session |
| --- | --- | --- | --- | --- |
| Primary / historical defaults / primary pack | `fdbdd69c21271f89` | `2a077c2ee0cc8f28` | `0d2ad7230908d429` | `54b25da22b72ea34` |
| 52c2899 with historical tipoff / legacy / candidate pack | `fdbdd69c21271f89` | `2a077c2ee0cc8f28` | `0d2ad7230908d429` | `54b25da22b72ea34` |
| 52c2899 with historical tipoff / factory | `7f99d2502faa1c5d` | `2a077c2ee0cc8f28` | `d52188a89ebcc7e4` | `350ddcf318f91f59` |
| 52c2899 canonical tipoff / legacy | `51e05fa9d83da3b1` | `5a25371337ed4163` | `cebb9b11b4f7164e` | `54b25da22b72ea34` |
| 52c2899 canonical tipoff / factory | `43120bb45e70aa8b` | `3fe4cb90be1bc85e` | `e263922583c6b2d8` | `350ddcf318f91f59` |
| Current with pre-layout tipoff/helper / factory | `43120bb45e70aa8b` | `3fe4cb90be1bc85e` | `e263922583c6b2d8` | `350ddcf318f91f59` |
| Current / legacy | `ea1707afe4948974` | `2ca9a17163baf7c4` | `4695c8f77b58ac24` | `54b25da22b72ea34` |
| Current / factory | `d26e6deec1fdc18e` | `6e6bc3bc0c311ae1` | `e9be982573713866` | `350ddcf318f91f59` |

All eight diagnostic cases produce byte-identical first/second outputs,
including owned states, sessions, complete telemetry, five-field projections,
66 raw pixel samples/BMPs and UI-input logs within each case. Across revisions,
owned layouts differ; no cross-layout byte-equivalence claim is made.

The primary and52c2899/historical-tipoff/legacy controls match every sampled
pixel and all6000 gameplay projection rows, not just their aggregates. Thus
the required release-frame repair and candidate-pack-compatible Setup code
can still reproduce the old golden exactly. Running the old primary code
directly with the candidate pack fails at its old Rules transition contract;
that failed cross-version attempt is retained and not counted as a comparison.

The historical-tipoff legacy/factory comparison changes only samples0/1
(Main and edited Rules) among all66 images and leaves all6000 gameplay
projection rows identical. The edited rule is45-to44 historically and0-to1
from factory; this also changes the final five-word session projection.
The original Rules44 native witness remains unchanged and remains scoped to
its original configured journey. No new exact-native image claim is attached
to the factory Rules1 sample.

The canonical team/rank and configuration matrix then separates their effects.
`completion-mode1-attribution-audit.md` establishes the original team-context
and sorted actor-rank stores; `setup-config-native-contract.md` establishes
factory configuration. Later controller/input/F34F work does not change this
closure before the layout fix: the52c2899 factory and current/pre-layout cases
match every sampled image and all6000 gameplay projection rows exactly.

## First layout-dependent difference

The current pre-layout and corrected runs have identical complete telemetry
through index2304. At index2305 the first difference is exactly six words:
play12-to16; inbound target(-377,224)-to(-394,119); direction4-to2; and the same
target change on actor2. The ball position, owner, clock, RNG and every other
telemetry field agree. This is the C39C layout1 edge-constructor correction
already established by nine original-ROM cases and independent audits.

All16 pre-game images and the first20 gameplay samples remain identical;
changed images are samples36..65 only. The full runs preserve eight
transitions,65 render changes and2910 motion frames; the corrected path has
13646 upper-resource changes and71 possession changes. The existing minima,
comparison of repeated results, hashing order and sample cadence are unchanged.

`matrix-comparison.json` binds full comparisons and repeatability;
`first-layout-telemetry-difference.json` records the six-word first divergence.
The final fresh probe passes with only its expected aggregate changed to
`d26e6deec1fdc18e`. Original code12/code60/code81 logs, previous golden and
counterfactual sources/binaries are retained. Fullv14 still failed; this
standalone result is not a full-suite or whole-game completion claim.
