# Active implementation checkpoints

The user requested execution, delegated fixes, screenshot progress and regular
commit/push checkpoints on 2026-08-31. Implementation begins from `7002fb1`.
This record tracks work actually started, distinct from the completion plan.

| Workstream | Agent/owner | Fresh worktree | Current responsibility |
|---|---|---|---|
| C1 | Controllers | `c1-pass-interruption-20260831` | Attribute and fix the real contact/pass failure, then independently verify runtime behavior |
| S1 | Scheduler | `s1-bootstrap-20260831` | Implement original reset/IPL upload into a persistent CPU/SPC owner and the honest resident DSP stop |
| QA/screenshots | Independent auditor | `checkpoint-qa-20260831` | Inspect baseline/current-port screenshots and independently review each candidate; no production edits |
| Integration/M0 | Root | `completion-owner` | Original modes/SRAM route inventory, shared interface review, accepted integration, commits and pushes |

The three agent slots are allocated to two implementers and one independent
reviewer who also owns screenshots. Screenshot work is a bounded assignment at
each checkpoint, not a separate fourth agent or an unattended scheduled monitor.
Root can review already captured images while the auditor checks source evidence.

## Checkpoint policy

1. Preserve a clean source/build baseline and the existing failing witness.
2. Implement a bounded fix through its real caller. Retain exact source, pack,
   build command and test results, including failures and native evidence limits.
3. Independently audit the candidate and rerun affected integration gates in a
   fresh root build. Do not combine unrelated unreviewed patches into that result.
4. Commit and push the accepted integration source; verify the remote commit.
5. Rebuild that checkpoint for the screenshot agent, inspect its actual output,
   and publish a new dated gallery only when capture and visual checks succeed.
   Link the exact source commit and retain previous galleries. Commit a small
   documentation receipt afterward if needed; a documentation-only commit does
   not require another identical source capture.

If a code candidate fails review, keep it in its private worktree and retain
the failure. A separately labeled WIP checkpoint may preserve source, but does
not promote it to accepted integration. Main and the desktop executable remain
unchanged until release review. ROMs, packs, builds and screenshots stay ignored.

The stable local gallery is primary
`.analysis/progress-screenshots/latest/index.html`. The starting baseline is now
published as `20260831T182933.314971Z` from clean commit7002fb1:13captures, fresh
40-source build and109source/header identities, with all13PNG/BMP pairs equal
and all13views unchanged from the previous gallery. Each was visually inspected;
the clock moves11:32to11:31 and the panel clears at2660. Manifest SHA256 is
`d2e7acb11ad0b483b8b4c994abeb6ccf4ba1b73e565f403b8e29acb6afad8758`;
executable SHA256 is `731003ac8c3a19b0ad058109c0d7c921dcfbc9959ab5007ea255b579e30e18ff`.
The ignored capture helper now refuses publication without QA tied to the exact
manifest, images and logs; four negative publication guards pass. Prior captures
and their script copies are unchanged. The agent has switched to independent
C1 source review. Native emulator
captures are serialized through root and use isolated configurations/save homes;
the screenshot pipeline uses the native C renderer and no emulator.

## Findings at implementation start

The C1 implementer identified an original owner-contact branch that can skip
ball drop, while the current port appears to drop unconditionally. Source/native
attribution and independent review are in progress; this is not yet an accepted fix.

The S1 implementer and root corrected a planning-only mnemonic error: SPC03DB
is `MOV $F3,A` (`C4 F3`), not `OR A,$F3`. Its read-before-write bus cycle still
needs the hardware owner. Frozen source was already correct and remains unchanged.

## M0 normal mode-entry reference checkpoint

All four isolated normal-input v2 captures passed independent bounded review:
Exhibition82:809A,Season81:C54F,Playoffs84:A2F3→A328 and Load Series81:BF99→82:D085.
Each ends300frames after first entry. Root rechecked artifact hashes, exit status
and isolation. The corrected inventory and capture sources are committed together;
the large captures and independent audit artifacts remain ignored. See
`retail-mode-state-inventory.md` for exact identities, retained warnings and the
failed v1 Load attempt. This checkpoint adds no production mode implementation
and therefore does not require a duplicate screenshot gallery.

C1 now has three independently source-confirmed contact corrections in its
private candidate, with9,216controlled cases passing and936baseline failures.
The real63,800-frame loop has completed; regression review is still in progress.
S1 has a first executable reset/IPL upload with matching resident entry/F1 ARAM
and an explicit stop before an untranslated CPU reset helper. Its fresh native
capture/replay review remains underway; neither candidate is integrated yet.

## C1 contact repair checkpoint

This update supersedes the C1 pending status above. Independent QA accepted
freeze b3653771a3384bfa705cf4255d424a912b726fc7f1095dcd3ab6ef3533ef5253.
Root verified all581identities, copied the11files unchanged and rebuilt the
actual contact probe, full CLI, HUD runtime and new-match runtime from source.
The9,216source cases, actual41876interruption/41908recovery, six positive and
29negative guard cases, HUD checks and two C new-match return journeys pass.
The fresh63,800-frame trace matches the reviewed candidate exactly.

The final combined CPU regression passes gameplay/state/pass/receiver/motion
checks, then fails the existing frame600RGB golden with the same pre-C1 image.
That failure remains unchanged; the next screenshot checkpoint also examines
600/1300 before any separately reviewed golden migration. Natural original
contact coverage remains absent. See `pass-interruption-integration.md` for
the precise scope, identities and retained failures.

The screenshot agent will publish the usual13views and additional before/contact/
recovery frames from this exact committed source. C2 ordinary receiver-caller
capture is underway. S1's first bootstrap source replay matches the native
checkpoint, but QA found verifier omissions; a new verifier revision is required
without changing the frozen source/evidence. FullS1 through03DB remains open.
The existing Max task is providing read-only advice on D1 draw-order/shared
render-state integration, with implementation retained in the delegated workers.

## C1 screenshot publication receipt

The screenshot agent published the clean facd818 build after inspecting all13
standard views and seven supplemental captures, plus two retained pre-HUD
expected images. Root inspected the contact/recovery images and opened the
gallery. Standard run `20260831T190716.018342Z` and supplement
`20260831T190856.946784Z-c1-supplement` remain under the primary repository's
ignored `.analysis/progress-screenshots`; `latest/index.html` links both.
All129 local links resolve. No captured image, ROM, asset pack or executable is
committed. Each image uses a fresh documented scene-entry process, not a
continuous title-to-match journey or measured display-FPS test.

| Receipt | SHA256 |
|---|---|
| Standard manifest |18ba1e264746eb9984af47354234172b49ff8da794b9d30798154541870d5a18|
| Supplement manifest |c3cda87c26f0639b2f7e5f1d3920890145377fb06e6c80fe80d132a771305452|
| Fresh40-source screenshot executable |a39b82067d3a398d6ca76c8b32c7c8f18688d575270cb5a5a7756cad15f1eba2|
| Independent screenshot report |d5d77c33638b2e133830ea7374aecf1f5fdc073b0ac907d95893a8d7f54d9751|

The thirteen standard images are pixel-identical to the prior7002fb1 gallery.
The supplement shows contact near the right edge and subsequent recovery;
visible time57.4→56.9 agrees with the independently checked C trace. Images do
not establish native contact parity. Human controls and missing HUD children
remain explicit limitations. The initial HTML encoding failure was retained;
the corrected pages did not change images, logs or manifests.

QA separately supports migration of only the600/1300 C RGB anchors: their old
images retain the static panel, and all changed pixels lie within
x32..239,y144..207. No migration is included in this receipt. The other three
anchors and the complete regression need their own results.

C2 now uses `c2-receiver-integration-20260831`, based on facd818. Root D1 uses
`d1-culling-20260831` from the same base. Their candidates remain separate from
accepted integration. S1 verifier revisions preserve frozen code and evidence;
additional metadata omissions remain under review. Max's read-only D1 advice
is recorded in `completion-draw-consultation-20260831.md`.

## Accepted startup component and narrow image migration

`bootstrap-accepted-component-integration.md` records the independently accepted
normal reset/upload/F1 source through CPU80BC, all fresh root checks and exact
byte preservation. This does not wire an incomplete bootstrap into the game.
The newer DMA child remains separate. Existing v1/v2 verifier failures are kept;
v3 rejects all12 independently demonstrated corruptions.

The separately reviewed600/1300 image migration is committed atf5a3bd6. Its full
rerun passes those images and earlier behavior checks, then stops at3480's
unchanged expected image. See `hud-two-golden-migration.md`. No other golden or
behavioral assertion was changed; the complete suite remains failing.

## Wired culling repair

`court-culling-integration.md` records independently accepted source tests and
fresh root verification of the actual culling helper. The new63,800-frame trace
matches the candidate exactly; only22visibility fields differ from C1, with all
other serialized state unchanged. The contact regression still passes. The
existing panorama exit20 and later3480 image difference remain explicit open
failures. The screenshot agent will publish the exact committed build, including
the changed visibility window. This is one D1 correction, not full graphics
integration or a full-game acceptance milestone. The exact fc1e73a gallery is
now published:19 inspected images, all standard views and late before/after
pairs unchanged in RGB,132 links verified. Dated identities and the explicit
clipping limitation are recorded in `court-culling-integration.md`.

## Accepted receiver component and graphics dependency

The 13 independently accepted C2 deliverables are now copied exactly, with fresh
root native child/parent, controlled arithmetic and verifier checks passing.
`receiver-prepare-integration.md` records the bounded acceptance and retained
runner setup failures. No production manifest, gameplay caller or human policy
changed. Runtime receiver integration still needs the actual catch gate and
source-owned shared graphics state.

The user's requested Max consultation remains read-only and addresses this queue
dependency. `completion-graphics-queue-consultation-20260831.md` records the
partial court decoder/cache cuts, carried cursor history, canonical WRAM aliases
and required producer/consumer boundaries. It does not justify a fresh court
queue or a captured $012C seed. The controllers agent separately fixes the
reported ball/body alignment; accepted direction work waits for combined visual
verification. Startup readers need additive protocol repairs for newly found
terminal-register/boundary-output omissions; frozen source/evidence is retained.

## Combined pass-render correction

`pass-render-integration.md` records the independently reviewed body-resource and
draw-direction changes, exact composition and fresh combined checks. The reported
hand/ball separation is corrected by restoring the original body pose, with no
ball-coordinate or physics changes. All63,800 JSON rows were compared to current
culling integration; only enumerated draw/appearance fields differ. Body checks,
original direction cases and contact recovery pass. Full regression and the
exact-commit screenshot receipt remain separately recorded gates. These port
defects are explicitly excluded from the original-bug catalog; confirmed
original behavior remains preserved and commented.

The744809a checkpoint is pushed and its exact-build gallery is published:
33inspected images,195valid local links,13standard views unchanged from the
culling checkpoint, plus before/after pass and later-pose comparisons. All
nine pass ball projections are unchanged. `pass-render-integration.md` records
dated manifests and the independent screenshot receipt. The full CPU regression
still stops at the same pre-existing3480 image after the earlier behavior and
600/1300 checks; no golden was changed. Main/desktop remain unpromoted.

## Base startup verifier integrity repair

`bootstrap-boundary-v4-integration.md` records four exact accepted verifier/test
files, fresh root8-source replay,11new+21protocol+12profile corruption rejections,
and all seven execution artifacts unchanged. The v4 reader replaces v3 as the
80BC acceptance entry point; old false-acceptance evidence remains retained.
No C or production behavior changed. The separate first-fill component now has
bounded independent acceptance with its new reader; reset-table source and
later NMI work remain separate reviews, not runtime integration claims.
