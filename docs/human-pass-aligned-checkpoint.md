# AD0E aligned-pass continuation checkpoint

This checkpoint translates the next bounded child of the existing human B-pass
work. It does not enable human play, change the production source manifest, or
replace any earlier checkpoint. The C module accepts native-entry state at
`86:AD0E` and runs the four early catch gates, the `86:AE10` animation/family
selector, its `85:F473` lane child, and `86:AED9` family/upper installation up to
the next explicit continuation. The complete pose/attachment stage at `86:AF1D`
is not executed here.

All 11 new source files live in the existing `completion-controllers` worktree.
The private build, original capture manifests, reference output, failed first
report, and final freeze are under `build/human-pass-aligned`. No commit or push
is part of this checkpoint. The previous seven freezes remain immutable.

## Concrete translated boundaries

| API | Entry and stopping contract |
| --- | --- |
| `nba_human_pass_aligned_prepare` | `86:AD0E` through the four early gates. Stops before `86:AD3D` when catch prerequisites hold; otherwise executes choose/install below. |
| `nba_human_pass_aligned_choose` | `86:AE10` through `86:AED9`, including the actual `85:F473-F5E3` child when the aligned receiver is not in mode 14. Chooses upper request and family; does not install them yet. |
| `nba_human_pass_lane_obstructed` | Complete `85:F473-F5E3` AA result over the native ordered actor list. Models both scan directions, skips receiver/ball/same-team actors, and preserves original early stops. Does not expose all volatile DP outputs. |
| `nba_human_pass_aligned_install` | `86:AED9` publishes actor `+C0`, then executes the existing source-derived upper installer to `86:AF1D`, or returns before the `87:B3BD` both-channel child / `86:AF30` commit. |

The upper implementation is the unchanged frozen action module's B47A wrapper,
which uses the existing asset-backed animation installer. The probe rebuilds
that unchanged dependency into a private object and links the frozen original
object set. It does not mix root integration headers or objects into this ABI.

## Original source and native evidence

Original ROM: `F:/Games/SNES/NBA Live 95 (USA).sfc`, 1,572,864 bytes, SHA-256
`2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`.
Mesen executable SHA-256:
`d2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b`.
The asset pack at the primary repository's
`.analysis/frontend-integration-20260830/nba95_assets_candidate.pak` has SHA-256
`951f82331c4bb6ce8f381da519ee8bfdf517bf8c13f2cd6f20cfa9c34d5ed4df`.

Fresh ROM bytes, Ghidra 65816 output, and recompiler output were generated for
`85:F473-F5E4`, `86:AD0E-AF30`, and the `87:B3BD/B47A` installers. B3BD is
reference-only in this slice. The original `84:C2FC` upper table and descriptor
headers for `2A,2B,2C,2F,30,31` are attested too. Reference manifest:
`build/human-pass-aligned/reference-v1/manifest.json`, SHA-256
`1e254add89dc14cbec7247f5d757b2f75dc66d68307b73f13f6a87094d693ada`.
It binds exact ranges, tool sources, commands, exit statuses, and artifacts.

Both original-ROM captures start in fresh isolated Mesen processes with private
settings, home and save directories, fixed zero RAM power-on, and ordinary
controller input. The Player screen starts `[2,1,1,1,1]`; two released left taps
select side 0, while the right route retains side 2. The ordinary court schedule
uses B taps with eight directions and neutral. Each run covers 2,400 court
frames. No ROM, PC, flags, ownership, or WRAM state was injected. There are no
controlled vectors in this checkpoint.

| Native route | Pass calls observed | AD0E gates | AE10 choice / AED9 install / upper child | F473 calls | Compared values |
| --- | ---: | ---: | ---: | ---: | ---: |
| left, selection 0 | 16 | 15 | 12 each | 3 | 94,815 |
| right, selection 2 | 9 | 5 | 5 each | 0 | 35,120 |
| total | 25 | 20 | 17 each | 3 | 129,935 |

All comparisons pass. Seventeen gate calls reach `AF1D`; three left calls at
court frames 1591, 1641, and 1760 stop before `AD3D`. The right route never
enters F473. All three observed F473 results are AA=1; no clear-result,
endpoint, or extreme-coordinate native behavior is claimed. Internal scan
branch PCs were not recorded, so this is not complete branch coverage.

Capture manifests:

- left `selection0-v1/manifest.json`:
  `e328d6908108e8b4fcfb315de15bc3e54270f3a3664709056e2f1e3e95528112`
- right `selection2-v1/manifest.json`:
  `4ddc9fffa7d0f2b756a23ef3478acc3f4306eec249ae1894a878ba9fee803477`

Every native boundary has the existing 7,936-byte sparse raw snapshot. For each
C mode, the verifier compares every declared output: all 11 actor records,
five controller records, two canonical context records, profile table, 13
globals, two selection words, and the 13-word native order list. The C probe
receives only the actual entry raw path, immutable original ROM and asset pack;
it never receives expected exit values. Choice/upper mode route=1 is a success
marker for that child's endpoint, not a claim that the whole initializer has
completed.

Non-lane modes additionally compare DP `00,47,49,4F,51,BE,C0,B2,AA,AE`.
F473 compares AA plus its seven saved/restored words
`B6,BA,BE,C2,9A,9E,A6`. Its volatile `AE,B2,92` values and CPU registers are
outside that leaf's declared API. The enclosing AE52 caller restores B2/92 and
overwrites AE with the selected family; the enclosing choice/gate comparisons
cover the declared state after that restoration.

## Earlier failure attribution and preserved source behavior

`native-aligned-attribution-final-v1.json` connects the original saved boundaries to the
two remaining fields in the earlier, explicitly partial whole-initializer
adapter. At left court 270, actor 9 passes to actor 7 with distance 110, fine
direction 4, movement direction 2, profile byte 85 and receiver anchor 281.
The four gates bypass AD3D. F473 returns AA=1 at events 25/26; `AEBA-AEBF`
chooses request `2A` and family 0 at event 27. By `AF1D` event 30, the native
upper state is `2A` and family is 0, and both remain unchanged at the later
initializer resume. The new gate, choice and install comparisons reproduce
those words. The old adapter's `2F`/family1 result was a missing implementation,
not an original-game bug. Its failure files and source remain unchanged.

The source translation preserves these less obvious original contracts:

- `AE19/AE1C` execute two SBC operations without a second SEC or a mask. The
  first subtraction's borrow affects the second; the full word is retained.
  The ordinary aligned witness uses `4-2-2=0`; borrow/extreme cases are
  source-derived and unwitnessed here.
- `F4A5/F4CD` expand endpoints by 24 using the sign of wrapped subtraction.
  `F539/F556/F5A4/F5C1` test XOR of subtraction signs. In ordinary ranges this
  includes the lower endpoint and excludes the upper endpoint. The code does
  not substitute an inclusive geometry test.
- The native order list occupies odd addresses `34D1..34E9`, with null sentinels
  and the ball among 11 entries. Actor `+14` selects a list cell. A failed X
  test at `F539` immediately restarts backward from the source; at `F5A4` it
  immediately returns clear. A failed Y test continues. Unsorted/extreme
  inputs are not silently repaired or rescanned as an unordered set.
- `AEDD` publishes family before the relative-negative branch to `AF30`.
  Near `2C` promotes to `2F`; distant stationary non-inbound requests stop at
  the actual `B3BD` entry. No arbitrary owner reassignment is used.

These contracts have PC comments in the new module. Endpoint/overflow/borrow
behavior is source evidence, not a newly claimed naturally reachable bug.

## Integrity checks and first failure

The verifier uses the accepted strict recipe: exact boolean/integer types,
attested source versions, executable/ROM/script command and route environment,
runner duration limits, initial frame anchors, every raw/event metadata word,
and all artifact identities. Actual persisted settings, Lua home and final
saves are compared before invoking the shared isolation helper on a copy. C
output JSON requires every declared field, vector size, integer value and row;
zero comparisons cannot pass. Earlier DF7A/AB2D/action events are checked for
ordering but are not sent to this new probe under reused mode names.

Sixty mutation checks reject altered metadata, omitted sources/artifacts,
raw/row contradictions, out-of-range words, uniform clock shifts, removed
aligned/lane stages, malformed or incomplete C outputs, float values in every
output family, and wrong assets. These tests derive from the independently
authored earlier pass audit cases, expanded here by the implementer. This is
not independent acceptance of this new checkpoint.

The first right report, `selection2-initial-v1.json`, failed because the first
verifier incorrectly required a lane call in each route. The preserved initial
verifier and left report show that requirement. The corrected verifier permits
explicit zero lane coverage while still requiring actual AD0E comparisons and
complete paired children. No C mismatch occurred. The initial probe executable
and objects are retained; the final rebuild adds precise source comments only
and is warning-free.

Final reports are `selection0-final-v1.json` and `selection2-final-v1.json`.
Final probe SHA-256:
`d02f567ca7b9b75868d441f353e04d19ade7e311e5efab7071fd47e8c2b94db6`.
Verifier SHA-256:
`6da5958ab01843047c3b4c5f1fc7d0b5abc1e7af352aa0d533571ee32d899d80`.
Mutation tool SHA-256:
`73852e3b27d0738db6d79bad8c8eca40aea9ab781838ff65ad2cee4a85d14cf1`.

## Remaining continuations

`AD3D` catch geometry/profile/RNG/F5E4 handling is not implemented here, including
the actual `[00]+42` pointer read. Do not replace that pointer with an assumed
profile pointer. `AF1D` still requires `87:AEC3` pose and `87:B649` attachment,
followed by the `AF30` state commit and later stack-return work. `B3BD` and the
relative-negative `AF30` branch are explicit, naturally unwitnessed stopping
routes. No airborne ACA9/AFC4 branch, whole AB2D return, DF7A caller closure,
normal human enablement, or production behavior is accepted by these vectors.
