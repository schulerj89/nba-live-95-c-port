# Controller checkpoint, 2026-08-31

This is a bounded controller-state implementation checkpoint, pending independent
review. **Human gameplay remains disabled.** It does not establish an equivalent
natural C gameplay journey or supersede the incomplete status in
`controller-ownership-model.md` or `team-context-initializer.md`; the broader historical audits remain in Git history.

The implementation worktree is
`C:\Users\joshs\Projects\nba-live-95-c-port\.analysis\worktrees\completion-controllers`,
based on `52c28996cdf693e0ae45aef714b47f698abd3ee1`. No assets were replaced,
no ROM was patched, and no agent commit was made. Artifacts below are under this
worktree's `build/controller-contract/`, not a copied primary `.analysis` tree.

## Implemented and connected

`nba_controller.c/.h` owns the five native 64-byte records, five previous
selections, ten actor assignments, two controller counts and two cursors.
The routines translate the complete named bounded memory-state contracts:

| Native routine | Implemented state effects | Current C caller |
| --- | --- | --- |
| `86:E208-E24B` / `86:E24C-E389` | Fresh initialization and allocator, including unchanged selection, override, sequential assignment and direction flags | `nba_tipoff_init` calls the initializer using effective neutral selections; the public adapter also supports saved selections |
| `85:EF3A-EFEC` | Held, previous, changed, newly pressed, both direction tables, stamina/L/R boost and Traveling event effects | Actor behavior sweep publishes once for each represented pad; the complete native sweep is still missing |
| `86:BC9B-BD1E` | Team-count gate, matching-team round-robin transfer, cursor advancement, old-owner clearing | Free-throw scene state1, corresponding to `87:9CDE-9CE6` |
| `86:D25A-D349` | Previous controller `0A00`, designated receiver transfer from `0944`, or team round-robin transfer | Ordinary `cpu_commit_ball_acquisition`, before the existing `D34A`/`BAA2` continuation |
| `87:9075-9086` | Clear five processed latches | C actor behavior sweep boundary |

The module's assignment array is a projection at mutation boundaries; actor
`+16` remains the gameplay authority. Mode11, human free-throw aim and the human
inbound helper consume the same records/counts instead of old duplicated
mode11-only fields. Input conversion explicitly separates host `NBA_BTN_*`
from native SNES bits. Ball attachment, ball launch and shot startup no longer
erase controller assignments or assign pad0 to whichever actor owns the ball.

The `D25A` prefix was found by a native record-write trace: ordinary control
moved at store `86:D31C`, not through `BC9B`. A subsequent natural pass exercised
the separate designated-receiver store `86:D2BF`. Reusing only `BC9B` for all
acquisition would miss that native branch and its cursor behavior.

Session adds `controller_selection[5]` and `controller_flags[5]`. Fresh single-pad
native `81:A489` records selections `[2,1,1,1,1]` and flags `[0,0,0,0,0]`; the
C defaults now use these observed values. Selection0 is visitor/left,
context1/group5; selection2 is home/right, context0/group0; selection1 is neutral.
The existing `player_one_side` retains its old left0/right1 meaning and cannot
represent neutral. It must not be used as a native selection or controller ID.
`nba_session_begin_match` and its A1 reset are unchanged.

Player Setup supports one-step right -> neutral -> left transitions, saturated
endpoints, left priority for simultaneous arrows and the assigned-only L/R
direction flag (`81:A7AB-A843`). Fresh native OAM gives controller x40,109,174
for left,neutral,right. Neutral hides the arrow and clears the label glyph
publication. The legacy side remains unchanged while neutral is selected.
The main-program left automation therefore requires two separated Left presses;
root owns that integration. Five-player UI, controller persistence and game-mode
swap behavior are not implemented here.

## Original quirks retained

These are source-confirmed behavior, not invitations to normalize the original:

- `86:E28B/E367`: compare masked current selection with unmasked saved previous
  selection, then copy the whole current word to previous. High-bit behavior is
  intentionally asymmetric.
- `86:E294-E2C0`: unchanged selection under override retains the record's group
  while the allocator has cleared assignments and counts. No cleanup was added.
- `86:D280-D2C8`: a valid designated `0944` transfer has no team-group check and
  does not advance the cursor. Its bit `$10` veto remains. The ordinary
  round-robin branch is team restricted; the designated branch is not silently
  repaired to enforce that same restriction.
- `86:BCAB` repairs a cursor >=5 to0. `86:D2DC` does not. The C boundary rejects
  out-of-domain host indices rather than emulate arbitrary memory reads; this
  is an explicit representation limit, not a claim of corrupt-state ROM parity.
  Similarly, a corrupt nonzero count without a matching record returns failure
  instead of reproducing the ROM's unbounded scan.
- `85:EFA3-EFA6` uses the wrapped subtraction sign after CMP `$80`. It is not
  replaced with a conventional unsigned >= comparison. Opposing-direction
  combinations retain the original two complete direction tables.

No original-game bug has been silently fixed. Comments identify the native
routines for these quirks. Additional gameplay or UI mismatches remain open.

## Native evidence and exact comparison scope

Original ROM: `F:\Games\SNES\NBA Live 95 (USA).sfc`, 1,572,864 bytes,
SHA256 `2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`.
Mesen source executable:
`C:\Users\joshs\AppData\Local\Microsoft\WinGet\Packages\SourMesen.Mesen2_Microsoft.Winget.Source_8wekyb3d8bbwe\Mesen.exe`,
SHA256 `d2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b`.

`capture_controller_contract.py` launches a private portable Mesen copy with an
explicit per-process environment, empty private saves, zero power-on RAM,
controller1 only, no frame skipping, no patches and no injected machine state.
It retains executed Lua/runner/isolation-helper copies, initial and persisted
settings, observed Lua home, process exit0, completion sentinel, source hashes,
artifact hashes, natural menu inputs and full WRAM at each captured leaf entry
and exit. Captures and existing reports are never overwritten.

Final selected captures:

| Capture | Court frames | Init / alloc / transfer / input / acquire calls | Compared words | Manifest SHA256 |
| --- | ---: | --- | ---: | --- |
| `selection0-v1` | 400 | 1 / 1 / 0 / 137 / 0 | 25,292 | `3ab955f14928dd3290cdcdb3ffabf210dcf278655dd0c92508cc47e8acb13013` |
| `selection1-v2` | 400 | 1 / 1 / 2 / 0 / 0 | 716 | `e33b68dffd0e5c9d0f0cee2921f015d7a60eb64ddd7114e500986b6e6c14d664` |
| `selection2-v4` | 1,400 | 1 / 1 / 2 / 660 / 6 | 121,916 | `cafc65fe454947db99436719672d345bbbc9cafd011ee2571303a20473c031ad` |
| `selection2-live-pass-v1` | 1,400 | 1 / 1 / 2 / 660 / 7 | 122,096 | `5dab38d9de0e560e2ead1cbffa171fa147b4234254dcc5c8080790f6b44f9b6a` |

All selected comparisons passed with no skipped calls, field mismatches or
tolerances. The left capture predates the unconditional pre-court transfer
hooks and acquisition hooks; its zero counts are missing capture coverage,
not proof that the ROM did not call those routines. Neutral has no human input
publication by design. The right live-pass route adds B at court900..902 and
observes the designated transfer at court960. Other route inputs are recorded
in each immutable executed Lua. Some input entries/exits span an NMI/frame
boundary; the verifier reports these crossings and compares them without
filtering (5 in right-v4, 3 in live-pass).

Each ordinary comparison includes all160 controller words, ten actor assignment
words, five previous selections, two counts and two cursors. Input adds boost,
event and event actor; acquisition adds previous controller. These are repeated
word comparisons, including nested initialization/allocation results, not a
percentage, unique-state count or full-WRAM claim. CPU/DP scratch state is not
the port's execution representation. The probe imports native prestate only;
expected poststate is read independently from the paired native raw exit.
The verifier validates identities, sizes, hashes, process completion, pairing,
exact field sets/list sizes, integer types and values.

Unchanged-selection reallocation, override/high-bit/multiple-controller cases,
all simultaneous buttons, active alternate direction, all Traveling and
free-throw gates, and every transfer-cursor branch are **not all naturally
witnessed** by these four captures. Full bounded source translation plus these
witnesses does not establish full branch coverage or natural C routing.

Fresh original-byte references are in `reference-v3/`: actual bounded recompiler
output, Ghidra listings for banks81/84/85/86/87, source copies, commands, logs,
original bank bytes and a hashed manifest. Tools used are
`C:\Users\joshs\Projects\tools\snesrecomp-source-v0.2.0-alpha\recompiler`,
Ghidra11.3 at
`C:\Users\joshs\Downloads\ghidra_11.3_PUBLIC_20250205\ghidra_11.3_PUBLIC\support\analyzeHeadless.bat`,
and JDK21 at `C:\Program Files\Microsoft\jdk-21.0.12.8-hotspot`.
The generator uses a private project, raw original banks, the65816 processor,
explicit16-bit M/X context, and independently specified entry seeds.
`reference-ui-v1` used a wrong `A7AA` seed; it is retained as rejected evidence.
`reference-v3` seeds the actual `A7AB` instruction (`89 30 00`, BIT `$0030`).
Prior reference versions are retained, not relabeled as the final evidence.

Routine raw-byte SHA256 values in the final reference manifest:

| Routine | SHA256 |
| --- | --- |
| Initialize | `2d21f00265fb230041e02f7362e493f01e930ab8fe788e01af51727464a57686` |
| Allocate | `56ae3614bd7626f8fbc687bfeb68d9f77a5cefa22151fe555da22d8824c58e5d` |
| Publish input | `7a90f1c5c18bda16ca7ff04d86b5c636e74f8d6407be14a28352a50225e27e54` |
| BC9B transfer | `6513ba4f85d56083f32a5853ddffb2f7c30272e09baddaed8afdfb5d05a035c5` |
| D25A acquisition prefix | `4c53cdeba8ec7b3b4aede2436c44567fe5a1ccfa3ad79cb2916b846acc53ca9e` |
| Human E2AC (investigated, not implemented) | `baed539851ed3011d55c1599a869a2463adc4073e7eec680f99ac1f0151fb456` |

## C checks and visual limits

Build and probe outputs are private to this worktree. The unchanged candidate
pack is read from
`C:\Users\joshs\Projects\nba-live-95-c-port\.analysis\frontend-integration-20260830\nba95_assets_candidate.pak`,
SHA256 `951f82331c4bb6ce8f381da519ee8bfdf517bf8c13f2cd6f20cfa9c34d5ed4df`
(263 assets, 89,438,786 bytes).

Passing checks include the full build; controlled controller runtime/UI checks;
all4096 host/native button round trips; timeout/resume; natural and deliberately
delayed C tip possession; foul-out substitution;61 mode11 native leaf calls;
22 catch-core and17 tip-bridge native calls; and both existing64-word production
initializer identity cases. The identity sources are explicitly historical:
`C:\Users\joshs\Projects\nba-live-95-c-port\.analysis\controller-ownership-20260830\selection2-v3`
and `C:\Users\joshs\Projects\nba-live-95-c-port\.analysis\controller-ownership-20260830\selection2-teams2-pause-v2`.
Their original isolation caveats remain. No expected output or gameplay hash
was rebased. This is not a report that the entire `build.ps1 -Test` suite passed.

Two embedded CPU fixtures initially failed after incorrect ownership wipes
were removed. Their declared prestate was repaired: the shot fixture now
explicitly sets controller-1 instead of zero/pad0; the CPU acquisition fixture
sets `0944=-1` instead of3 (which requests a real designated controller transfer)
and publishes all actor groups/CPU assignments. Expected outputs were unchanged.
Earlier failing build logs are retained. These are C fixture repairs, not
native evidence and not concealed tolerance changes.

Fresh C Player Setup images are in `ui-v2/`; native RGB and OAM remain in the
selected capture directories. The neutral glyph fix follows `81:B748-B75E`
calling `81:A2B8` then `81:A1E7`: row1 metadata gives WRAM934E,320 bytes, and
VRAM byte8370. Assigned native row bytes equal pack asset257's publication
(SHA256 `6b919a6491067db86c4c06d0a09bcaa9696700e9c06c578d8e332866b975092e`);
neutral is320 zero bytes (SHA256
`7b6436b0c98f62380866d9432c2af0ee08ce16a171bda6951aecd95ee1307d61`).
This clears glyph data, not rendered pixels or a masking rectangle. The old
`ui-v1/comparison.png` retains the pre-fix visible neutral label as failure
evidence. C title/background palettes, label palette phase and transition
timing still differ visibly from native. No full-image match is claimed.

## First missing contract and handoff

For a normal selected-human journey, the **first intentional C/native failure
is the controller initializer result**: C uses effective `[1,1,1,1,1]` and
assigns no human, while the original uses the saved left/right selection.
Do not hide that mismatch by setting `cpu_vs_cpu=false` or assigning the ball
owner. Saved UI selections remain available for the eventual enable boundary.

The next substantive implementation is `87:9106-929E` and `84:E2AC`:
the real caller uses native controller/processed latches and newly pressed
bits, then action branches for B pass/switch (`84:DF7A`/`84:E141`), X jump
(`84:E3EA`), A defense action (`84:E432`), Y owner/off-ball paths
(`86:BD1F`/`86:AB2D`), and A shooting (`86:B335`/`86:B625`). The movement path
at `87:91C3-922A` has free-throw, recovery, flags, receiver, inbound and mode
gates before `85:A82C`. The current C sweep publisher does not implement this
dispatch/order, and still calls the existing CPU behavior dispatcher. Its
normal initializer gate prevents falsely presenting that as human play.

Also outstanding: pause/Select requesters `095E/0960` reset/publication and
requesting-pad ownership instead of legacy UI side; real multipad polling;
pause Player Setup reallocation; natural free-throw and inbound input timing;
full human offense/defense actions; lifecycle transfer callers; complete
source/ROM scheduler and RNG alignment; UI palette publication; independently
audited normal C journeys. Neutral currently still reaches legacy pause
compatibility behavior, which must be replaced before enabling human controls.

Reproduction starts with `build.ps1 -RomPath <original ROM> -AssetPack <candidate>`
inside this worktree, then `tools/build_vector_probe.ps1 -Name
controller_contract_probe` and `controller_runtime_probe`. For each capture run
`python tools/verify_controller_contract.py --capture <capture> --probe
build/controller_contract_probe.exe --rom <original ROM> --output <NEW report>`.
New native capture uses `python tools/capture_controller_contract.py --output
<NEW directory> --selection 0|1|2 --rom <original ROM> --mesen <Mesen source>
--court-frames 1400 [--live-pass]`. Do not overwrite any evidence directory or
call these vectors full-game acceptance.
