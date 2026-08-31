# Typed DD97-E207 period formation composition

`include/nba_period_formation.h` and `src/nba_period_formation.c` provide a
source-backed composition for a later NbaTipoff adapter. Production state is
`NbaPeriodFormationState`: named actors, ball, contexts, channel/pose fields,
controllers, roster pointers, sort buffers and carried globals. It is not WRAM,
an instruction interpreter or an opaque native-state replacement. The probe
accepts one2110-byte PFC1 typed DD97 input; it accepts no child-return files,
poststates, snapshots at later boundaries, duration values or port responses.

The source caller DCA6-DD97 must already have completed, including the period2
anchor flip and owner reset used by the first appearance child. The separately
accepted entry-prefix helper is outside this module. Normal source CPU width
and decimal preconditions are M=X=D=0, DP0. The assets must be the validated
original-resource pack; the diagnostic pins the89,438,786-byte pack SHA256
951f82331c4bb6ce8f381da519ee8bfdf517bf8c13f2cd6f20cfa9c34d5ed4df.

Call `nba_period_formation_begin`, then alternate `advance` and `resume` while
kind is CHECKPOINT. Each advance performs source work through one observation
boundary. Work, state and assets must remain unchanged while waiting. Repeated
advance at that boundary is immutable. The module itself executes every included
child against current C-produced state. It exposes COMPLETE atE207, REFUSED for
an unsupported child domain, or ROLE_STOP for the frozen role continuation's
unresolved record/assignment boundary. Those terminal results cannot resume.
Refusal is a typed precondition stop, not a simulated native failure or a guessed
child result. Child projections commit only on success; preceding parent work
remains visible. No production wiring or manifest edit is included.

Included call order:

1. Unchanged parentv2 DDA4-E183 writes and checkpoints, including all ten paired
   formation/appearance calls, E056 ball initialization, A60D cancellation,
   C37D layout0 publication, regulation ownership and opening/OT state81.
2. Actual87AAB2 period appearance, D85E assignment/roster/sort, and D5DB object
   sort children. Each receives transient typed fields projected from the same
   canonical state; returned fields are synchronized immediately.
3. Regulation E183-E1A4 actual BC9B transfer, animation cancel/install, AEC3 pose
   resolution, B649 attachment and B66A direction children. Opening/OT skips
   this regulation sequence.
4. E1AC-E1E5 predicted0918/091A from current ball integer XY. Opening/OT invokes
   BC9B for actor0/context0, then actor5/context1. Regulation already transferred
   inside its attachment sequence.
5. Unchanged role sourcev2, with verifierv3 acceptance, in the original context0
   then context1 order. Rebuild invokes all real B95C/RNG work; unresolved BF51
   indirect records and BAE4 assignment children remain explicit stops.
6. Accepted render tail D5DB, original FBFF gap sort and84A/84C increment through
   E207. It does not replace the ordinary per-frame FC80 single pass.

Canonical ownership and aliases:

| Storage | Sole owner and child mapping |
|---|---|
| Actor04/08 and fraction02/06/0A | `parent.actors[]`; integer and fraction words remain separate. No16-to8-bit rounding. |
| Actor3A/3C/42/44/46/48 | Parent phase/accumulator/lock fields; channel projections share these exact owners. |
| Actor30/32/38,18/1A,1C..26,B0 | Unique extra channel fields, including both complete three-entry queues. |
| Actor28/2A/2C and34/36/3E/40 | Unique mirror/resource/published-pose fields. Attachment synchronizes them; pose.direction aliases parent52. |
| Source4E/50/52 | Parent.direction/requested_direction/movement_direction, respectively. NbaTipoff's current names differ; adapters must use source offsets. |
| Actor60/64/72/7E/8E | Parent action/behavior/boost/flags/focal fields. Role reaction60 must not become a second independently stored timer. |
| Actor74/76/78/80/84/86/8A/8C/92 | Unique extra assignment/help/saved-mode/pair/anchor/role fields. |
| Actor16 and context3B/3D | `controllers.actor_assignment/count/cursor`; never a second actor/context copy. All controller records and their preserved tail words remain typed. |
| Context0A anchors | `input.anchor_x[2]`, used by parent and role projections. |
| D73E09DA..09EC | `assignment_sort_slots[10]`; later role09DA/09DE/09E2 are slots0/2/4. Assignment writes the complete sorted buffer before roles consume it. |
| RNG07F6 | One `rng` word used in source order by appearance and role children. |
| DP9A | Parent's internal list_cursor temporarily aliases the role pair pointer. It is not included in native gameplay parity; other child CPU residue is outside these typed APIs. |

Input liveness for the NbaTipoff adapter:

| Classification | Fields and implications |
|---|---|
| Consumed carried inputs | Period/tip/anchors; context links/team/active-roster slots/selectors; full24-entry roster address table; existing actor controller assignments; channel/base/queue state; appearance flags/alternate set/variant/resources; C6 word and RNG; controller records/counts/cursors; ready/dead-ball carry; role cadence/rebuild/ball assignment and ball anchor distance; leading sentinel; render camera0860,12-entry draw permutation and3FEB basket XY; frame counter words. Supply real source-owned state. |
| Parent writes before child consumption | All actor IDs, integer formation positions/directions/targets, velocities, action/behavior/reset timers, phase/accumulator/locks, list links and A6; ball ID/fractions/coordinates/velocities; the11-object collision list. Initial values remain exposed for intermediate carry checkpoints but are not required to compute these final outputs. The API does not demand invented canonical initial IDs. |
| Child writes before subsequent consumption | Active roster/statistic pointers, assignment triples/order/role fields and10sort slots; pose outputs for the attached actor; predicted ball XY; collision-list ordering/links; render depths. |
| Preserved output/carry | Actor fractions are never reset; ball fractions are reset by E056 and then preserved by attachment. Original09BA/09B0/09B2 carry survives. A6 remains the source pair index. Other actors' published pose34/36/3E/40 remains carried, though this composition does not consume it. |
| Required missing normal ownership | The draw list originates at80FBE9 during new-game86DA89 and thereafter changes, including normalFC80 passes. Basket3FEB XY cannot be reconstructed from post-DD97 anchors: a captured period1-to2 restart retainsX=-336 after context0 becomes+336. This module requires carried draw/basket state explicitly. |

D7B8's carried roster address table is not replaced silently by fresh ROM lookup.
The module validates all24 typed addresses against the asset-derived original
team/slot records before calling the bounded assignment child. Thus callers with
unrepresented or inconsistent normal roster initialization refuse atE0AC.

CPU scratch is excluded from gameplay storage. In particular attachment's47
argument is only a descriptor-output observer; its initial value is never read
by the included install path. Twelve role CPU temporaries are not canonical
inputs. Each value that can influence a modeled output is written before use;
a preflight verifies each initial focal scan finds a candidate. Cases requiring
an unmodeled carried92 nearest pointer refuse before the role calls rather than
choosing a fabricated actor. DP9A is retained for parent bookkeeping/role alias
synchronization but deliberately excluded from native value comparisons after
child APIs that do not expose CPU residue. This is not CPU/DP/register parity.

The appearance child still has its accepted CPU-actor domain: human palette
updates and unsupported channel/resource domains refuse explicitly. The role
child still has its accepted live81/82/canonical-bijection domain and unresolved
record/assignment stops. Frame/interrupt/video timing, sound/DSP/SPC state,
normal draw initialization and the full user journey are not claimed here.

Evidence:

- Fresh `/W4 /WX` private compilation uses no shared object files. The30small
  dependency snapshots include byte-identical appearance/support sources from
  owner commit979c042769e9cfeca99e65e92eb325ccdc596efa and accepted render-v2
  source hash closure b9362eab95705e0b0c4fcab5a8f6ce85b846de9f32a28a1dfc90dcdc79e90e96.
  Extra core/font/renderer sources are link dependencies, not additional period
  logic. Their hashes and provenance are pinned in the private manifest.
- The diagnostic field map has1029unique named fields covering2106bytes, with
  no overlapping ownership. All125 corresponding native checkpoints match1028
  gameplay values each:128500comparisons. Four independent normal-entry traces
  use controlled expiry; only the DD97 before-state is input to C. DP9A alone is
  excluded for the stated CPU-residue reason. The three scalar widths are not collapsed: byte, word and32-bit address
  domains are checked individually.
-47 controlled composition cases include all20period0..4/tip0or5/anchor+/-336
  combinations. An independently authored actual-ROM coordinate oracle checks
 1600fields. Actual-ROM role execution checks8651alias fields from current
  C-produced E1E5 states, covering deeper planner/rebuild and explicit stops.
  These are source-only checks, not natural reachability or timing claims.
- Actor sub-byte fractions, full queue arrays, output-only IDs and original
  stale ready/dead-ball words have focused carry checks. Eight malformed-entry
  refusals and32reachable verifier corruption cases pass. Child-domain refusals
  preserve the last completed checkpoint; repeated boundary calls are immutable.
- Root's earlier raw diagnostic informed call order and adapter review, but its
  prior C reports/output images do not drive this probe or expected data.
  Source/native/freezes remain immutable. Independent composition audit pending.

Reproduction from the scheduler worktree, using fresh output directories:

```powershell
& tools/build_period_formation_probe.ps1 -OutputDirectory .analysis/formation-rebuild
python tools/verify_period_formation.py --rom 'F:\Games\SNES\NBA Live 95 (USA).sfc' --pack ../completion-owner/build/full-extraction-v1/nba95_assets.pak --exe .analysis/formation-rebuild/period_formation_probe.exe --output .analysis/formation-native --native ../completion-owner/build/period-restart-attribution-v1/period-0-ready1-children-v2 ../completion-owner/build/period-restart-attribution-v1/period-1-ready1-children-v2 ../completion-owner/build/period-restart-attribution-v1/period-2-ready1-children-v2 ../completion-owner/build/period-restart-attribution-v1/period-3-ready1-children-v3
python tools/test_period_formation.py --rom 'F:\Games\SNES\NBA Live 95 (USA).sfc' --pack ../completion-owner/build/full-extraction-v1/nba95_assets.pak --exe .analysis/formation-rebuild/period_formation_probe.exe --native .analysis/formation-native --coordinates ../completion-auditor/tools/test_period_formation_rom_audit.py --output .analysis/formation-tests
python tools/test_period_formation_protocol.py --rom 'F:\Games\SNES\NBA Live 95 (USA).sfc' --pack ../completion-owner/build/full-extraction-v1/nba95_assets.pak --exe .analysis/formation-rebuild/period_formation_probe.exe --native ../completion-owner/build/period-restart-attribution-v1/period-0-ready1-children-v2 --output .analysis/formation-protocol
```
