# Known original-game bugs and preserved quirks

This catalog records original NBA Live 95 behavior that the port deliberately retains. It separates demonstrated arithmetic/indexing defects from unusual behavior whose purpose is unknown. **It is not a list of bugs that players can currently encounter in the port.** Most entries are preserved in audited standalone components that have not been enabled in normal gameplay.

Initial review on 2026-08-31 against owner commit `dc10166f18f505af0259c981a59f525e4ead663e`; the later HUD endpoint entry records the bounded integration-branch repair. The original USA ROM is SHA-256 `2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`. This is a bounded catalog of investigated behavior, not an exhaustive ROM audit.
 Historical source and audit reports removed from the working tree during the documentation cleanup remain available in Git history.

“Native” below means execution of the original ROM in the recorded emulator. A **controlled native** case changes declared test inputs; a **source-only** case checks original instructions with diagnostic inputs and does not prove normal gameplay reaches them. None of these labels establishes developer intent, audible symptoms, or complete game parity.

| Category | Entries | Current preservation status |
|---|---:|---|
| Confirmed arithmetic/indexing defects | 2 | One accepted standalone component; one unintegrated candidate whose source review passed but verifier packet was rejected |
| Original quirks retained, not classified as defects | 12 | Seven entries in accepted standalone components; one in the bounded integration-branch HUD repair; four visible production artwork/transition quirks |
| Excluded or unresolved claims | Listed at the end | Not counted as original-game bugs |

The listed SPC, period, appearance, role, human-dispatch and catch components are absent from `nba95_sources.txt` at the reviewed commit. The launch candidate resides in the controller worktree. Existing production code may implement related routines, but the bounded acceptance reported here must not be transferred to a different implementation without verification.

## Confirmed arithmetic/indexing defects

### 1. A negative multiply can be one unit too small

**Trigger and behavior:** opposite-sign operands whose magnitude product has low word `FFFF`. For example, `255 × -257` returns `FFFF0000` (-65,536) instead of `FFFF0001` (-65,535). This is a concrete arithmetic error. No visible pass trajectory or normal-game occurrence of those exact operands has been established.

**Original cause and preservation:** `$85:F7A5/F7A8` complements the low word and branches past its increment when the complement is zero. The correct bank is **85**, not 86. `nba_human_pass_launch_multiply` explicitly reproduces and comments on that decrement in the launch candidate.

**Evidence and status:** independently checked original-ROM arithmetic and a declared controlled-native operand case establish the result. Natural launch captures do not establish this edge. The candidate source passed review, but its verification packet was rejected because four forged multiply-return routes were accepted. It is **unintegrated and not an accepted final packet**; preserving the defect in candidate C does not remove that rejection. See the independent launch audit.

### 2. A catch-preparation table index reads executable bytes as a timer

**Trigger and behavior:** the `$86:AF66` receiver-preparation branch with raw pass band `30`. Its lookup reaches `$86:AFC4`, beyond the five data rows, and reads instruction bytes `A6 8E` as word `8EA6`. Adding `24` hex produces timer `8ECA` (36,554), rather than another ordinary table timer. The investigation stops before the following child, so it does **not** establish a player becoming stuck or a particular delay in seconds.

**Original cause and preservation:** `$86:AF6E` indexes table `$AFA6` with the unmodified six-byte band. The comment and six literal results in `nba_human_pass_catch_receiver` retain the opcode-derived value in [nba_human_pass_catch.c](../src/nba_human_pass_catch.c). The component stops before `$86:AF83 → $86:B468`; it does not invent the child's result.

**Evidence and status:** source-only original-ROM cases cover all six bands, including this overrun. The natural catch captures do not enter this branch. This is an **accepted standalone component**, not enabled human gameplay. See the independent catch audit and integration status.

## Original quirks retained, without a demonstrated defect classification

### 3. Sound initialization leaves one upper-RAM byte untouched

**Trigger and behavior:** the resident sound initializer clears `$0870..08FE`, then `$0900..FFFF`, leaving `$08FF` unchanged. If that byte already contains a nonzero value, it survives. No audible consequence or intended use of the surviving byte has been established, so this omission is not promoted here to a confirmed player-facing bug.

**Preservation and evidence:** the count calculation at SPC `$03A3..03B5` uses `FF-70 = 8F`, and the `$03BC..03C0` loop omits the final byte. The corresponding subtraction is commented in [nba_setup_spc_init.c](../src/nba_setup_spc_init.c). This uploaded code maps ROM `$00:C687..CB76` to ARAM `$0380..086F`. Nonzero source-only tests, including an independent original-ROM diagnostic, prove preservation of `$08FF`; the all-zero native fixture cannot distinguish a missing write from a write of zero. See the source description, independent source audit, and accepted initializer status. **Standalone; unresolved hardware/DSP continuations remain.**

### 4. A period restart preserves old ready state, dead-ball coordinates and player fractions

**Trigger and behavior:** continuing to another period rebuilds formation positions without doing the new-game bulk clear. The existing inbound-ready word `$09BA` and dead-ball coordinates `$09B0/$09B2` survive. Player sub-unit XYZ fractions also survive even while integer positions and velocities are reset. This does not establish that retaining them causes a broken inbound or a visible positioning error.

**Preservation and evidence:** `$87:9797 → 8C86 → $86:DCA6` bypasses the separate new-game `$86:DA3F..DA47` clear. `$86:DF4B..DFB1` does not clear player fraction words; `$85:C37D` does not clear the ready/dead-ball words. Both exclusions are commented in [nba_period_restart_v2.c](../src/nba_period_restart_v2.c). Controlled-expiry native captures, reached through normal cold boot/menu input, witness naturally carried ready `1` remaining `1`, and a separate ready `0` case remaining `0`; ready itself was not seeded. Nonzero fraction preservation also has source-only tests. See [source and capture scope](period-restart-source-helper-v2.md), native attribution, and [accepted parent audit](completion-period-restart-v2-independent-audit.md). **Standalone parent; whole restart integration is separate.**

### 5. Formation writes the same pair index into both teams

**Trigger and behavior:** formation assigns actor field `+$A6` the values `0,1,2,3,4` for each team. It does not write zero to every actor and does not write the unique ten-player ID there. The purpose of this field is not inferred here.

**Preservation and evidence:** `$86:E053` returns to `$DDA7`, skipping `$DDA4`'s initial zero load; `$DDA9/$DDAC` store the carried pair index. The source comment in [nba_period_restart_v2.c](../src/nba_period_restart_v2.c) retains this behavior. Native parent comparisons and original-ROM formation cases support it; see the [parent source explanation](period-restart-source-helper-v2.md) and [accepted audit](completion-period-restart-v2-independent-audit.md). **Standalone parent; no player-visible defect claimed.**

### 6. Alternate lower-body animation still uses the canonical phase-count table

**Trigger and behavior:** on the bounded stationary CPU appearance path, an actor using alternate lower-body animation cadence still checks its lower phase against canonical table `$84:C218` when the lower state changes. The validation and cadence tables therefore need not be the same. A resulting visual glitch has not been demonstrated.

**Preservation and evidence:** original `$87:B630` selects `$84:C218`, while `$87:AB5F` selects alternate cadence. The explicit comment in [period_appearance.c](../src/period_appearance.c) prevents silently changing the count table. Independent source-only cases cover both lower tables and carried states; forty native period calls support the bounded appearance path but do not independently witness every alternate-table consequence. See the source audit and [final verifier acceptance](completion-period-appearance-support-verifier-acceptance.md). **Accepted standalone CPU appearance child.**

### 7. Airborne human movement can update a controller-relative word instead of the player's boost timer

**Trigger and behavior:** after the earlier movement gates, nonzero full-word player height and a negative wrapped `live_state - 0080` result can leave the player's boost timer unchanged while decrementing `controller_pointer + 72` hex. That address can lie beyond the selected 64-byte controller record. We retain the proven address behavior; its intended purpose is unknown. Existing investigation notes call it the carried-X bug, but this catalog does not infer broader memory corruption or gameplay symptoms.

**Preservation and evidence:** `$87:91D7` loads the controller pointer; `$91EB` can bypass `$91ED`'s actor reload. `$85:A850 → AAE8` then bypasses another reload, so `$85:AB06/AB13` reads/writes the controller-relative word. [nba_human_dispatch.c](../src/nba_human_dispatch.c) comments on the route and exposes that word separately. A no-seed original-ROM L+X capture witnesses player boost `5` retained in three calls; distinct nonzero controller-word changes and fifth-pad addressing have separate controlled source tests. See the repair explanation and independent acceptance. **Accepted standalone human stage; normal human play remains disabled.**

### 8. Role geometry keeps its coarse and wrapped direction rules

**Trigger and behavior:** the role geometry routine maps relative vector `(0,1)` to direction `1` with distance `0`. A zero-vector direction `8` leaves both actors' existing pairing-direction words (`+86`) unchanged; these are not their displayed facing words. Extreme 16-bit coordinates retain wraparound, including negating `8000` back to `8000`. These are quantization and storage rules, not proof of an unintended visible turn or distance error.

**Preservation and evidence:** `$85:F34F`, especially `$F37F`'s equality branch and `$F394`'s truncated shift, is translated literally in the commented `pair_geometry` function in [nba_period_roles.c](../src/nba_period_roles.c). `$85:BC79` preserves the pairing directions on direction `8`. Independent original-ROM controlled cases exercise the edge rules; the four native period captures cover a narrower early-return route. See the role source audit and its [final verifier acceptance](completion-period-roles-v3-acceptance.md). **Accepted standalone period-domain role component.**

### 9. Role selection retains carried scratch, distinct tie rules and cadence state

**Trigger and behavior:** the initial focal scan replaces its candidate only on the original wrapped strict-lower comparison. If nobody wins, the old scratch pointer `$92` survives. A later fallback promotes the **last** equal eligible candidate instead. Early cadence returns preserve rebuild state; they do not mean “reset the planner.” Selection therefore depends on scan order and carried state, not just a fresh nearest-player calculation.

**Preservation and evidence:** [nba_period_roles.c](../src/nba_period_roles.c) comments on `$BCDF..BCE1`'s carried-pointer publication and `$BCED..BD06`'s cadence path. [nba_period_roles_v2.c](../src/nba_period_roles_v2.c) preserves `$C086`'s last-equal choice. Native captures witness the early-return cadence behavior; deeper selection and edge cases have controlled original-ROM coverage. Unrepresented record reads and assignment children stop explicitly. See the source audit, [final acceptance](completion-period-roles-v3-acceptance.md), and integration scope. **Standalone; no claim of general live-play planner parity.**

### 10. The shot-clock graphic advances at exact multiples of 60

**Trigger and behavior:** when the original publisher runs with a positive shot counter below the game clock, raw values 1–59 select numeral frame1, raw60 selects frame2, and raw120 selects frame3. Zero or a negative counter selects frame0. A positive counter at least as large as the game clock leaves the graphic unchanged. This is an exact source endpoint rule; no incorrect visible clock or timing fault is claimed.

**Original source and preservation:** `$87:BA6D` increments the graphic index before `$BA6F` subtracts60, and `$BA72` repeats when the result is zero as well as positive. The original `$87:BA9F` pointer table selects numeral maps `$AF:ED1B` for frame1 and `$AF:EC83` for frame2. The `shot_clock` function's `$87:BA6D/BA72` comment preserves that rule in [nba_gameplay_hud.c](../src/nba_gameplay_hud.c). **Enabled in the integration branch's bounded HUD repair; full HUD timing remains unverified.** See integration scope.

**Evidence limits:** an independent source-only diagnostic executes the actual original bytes for602 boundary/domain cases. The map glyphs use original indexed characters matching the captured first-court VRAM. Existing lifecycle captures contain no `$87:BA5E/BA9E` observations or raw60 input, so they do not establish this endpoint occurring during ordinary play. See the source-check evidence and bounded draft/report. No emulator capture was added for this entry.

### 11. Withdrawn: center-court fragmentation was a port layout error

The earlier classification as original artwork was incorrect. Correct team
tile bytes were combined with Orlando's map and shared CHR on standard-floor
teams such as New York and Chicago. `$85:8BBF-$8C4E` selects two distinct court
layouts; Boston, Milwaukee and Orlando share the parquet branch. The port now
selects the native map, common CHR and upload offsets together. Fresh Ghidra,
recomp and original-game PPU evidence, plus every-frame Tipoff checks, are in
the [court layout correction](court-logo-complaint-audit.md). This is a fixed
port error and is no longer an original-game quirk.

### 12. Team Select briefly exposes clipped name/rank rows during its reveal

**Trigger and behavior:** while Team Select enters, portions of its BG3 team
name/ranking rows appear clipped and slide into place before logo objects are
released. A still image can resemble corrupted menu text. Consecutive normal
input frames show the same transient on the original hardware path, so the port
keeps it as a transition quirk instead of hiding the pixels.

**Preservation and evidence:** the normal-power-on capture reaches Team Select
at `$82:809A`, remains forced black through setup frame518, then reveals the
scene at519. The clipped rows occur inside the source-owned vertical release;
names/logos settle around570-575. The production renderer comment and
`nba_team_select_render` retain those BG3 rows, while the deterministic frontend
test protects the later per-layer exit. See the frontend route checkpoint.
**Visible production transition quirk; retained without defect classification.**

**Evidence limits:** the consecutive capture proves the transient exists on the
ordinary original route and the source control flow identifies its scene owner.
It does not establish developer intent or exact whole-frame parity for every
entrance pixel. Abrupt team-name changes after a deliberate Left/Right selector
press are normal selection updates and are not a separate bug entry.

### 13. Orlando's ratings text crosses the center-court oval

**Trigger and behavior:** on the team-ratings presentation with Orlando as the
home team, the dark center-court oval remains behind the five rating rows and
visibly crosses the `BALL CONTROL` row. The overlap can look like a damaged
court logo or text layer, but the original game presents the same composition.

**Preservation and evidence:** normal original frame2300 in the retained
`player-intro-full-20260823` capture and the current production ratings frame
show the same oval, text baseline and rating-ball placement. The ratings
renderer keeps the full source court and comments this overlap beside the row
composition. **Visible production composition quirk; retained as original.**

**Evidence limits:** this proves the ordinary Chicago-at-Orlando ratings scene,
not every possible team pairing or animation frame. The separately corrected
`+2,+4` variable-logo offset concerns the gold team plates at the top of the
scene and does not alter this center-court overlap.

### 14. Player Setup first reveals partial construction layers

**Trigger and behavior:** after Team Select withdraws and the 67 presented
black frames finish, Player Setup begins with only a narrow portion of its
vertical background and logo motif at the right edge. Subsequent frames slide
and brighten the layers into place before the title, team logos, and controller
assignment settle. Individual early frames can resemble clipped or misplaced
artwork.

**Preservation and evidence:** consecutive ordinary-input native frames 767-769
show last-black, first-pixel, and the next partial reveal. Native frames 797 and
850 show the continuing slide and settled screen. Aligned production frames
320-322, 350, and 403 preserve the same construction phases, while exact pixel
hashes guard the C route's boundaries. See the historical frontend route report in Git history
and [player-setup.md](player-setup.md). **Visible production transition quirk;
retained without defect classification.**

**Evidence limits:** the comparison establishes the normal Exhibition path and
phase ownership. It does not claim exact whole-frame parity for every palette,
team pairing, or controller assignment during construction.

## Claims deliberately excluded

- `255` (or a crowded three-digit string that can resemble another number)
  beside Parish, Benjamin, or Duckworth in Starting Lineup was a port error,
  not original behavior. The ROM stores `$FF` for these three records but its
  visible cards show `00`. Their earlier two-scanline text offset and yellow
  divider were also host layout/color errors; the all-roster visual smoke now
  guards the corrected native placement and orange divider.
- Colored blocks, stray `SET OPTIONS` text, and wrapped setup tiles during the Setup -> Team Select exit were a port renderer error, not original behavior. The outgoing layer-window repair removes them and a continuous-scene pixel regression now guards the affected frames. This is distinct from entry 12's later source-authentic clipped Team Select name/rank reveal.
- The stale captured HUD panel/clock was a port error addressed by the bounded HUD repair. Remaining HUD timing/statistics gaps, the CPU restart failure near frame49,412 in the earlier baseline, and repeated Rules-entry failure are port investigations, not established original-game bugs.
- The former frame41,876 unreleased-pass failure was attributed and repaired at the C1 checkpoint: original owner-contact routing can skip the ball drop, and the guard must retain the actual interrupted passer identity. The independently verified correction and real C recovery are in the C1 integration report. It is not counted as an original bug; ordinary original knockdown capture coverage remains absent.
- The old culling helper discarded the original low-depth fallback and used the wrong wrapped comparison. This was a port error, now corrected in the integration branch. Unusual source culling alone does not establish an original game defect.
- Camera-subject/possession confusion, the false stale-AE explanation of the upper20/21 draw branch, and use of F34F where original draw/contact callers use F02D are port translation errors. The draw branch is corrected in the pass-render checkpoint; contact-facing remains separate. They are not original behavior to preserve or add to the defect count.
- The reported ball-to-hands mismatch came from the port rebuilding a different torso/leg pose using head direction. Original D4/D6 body resources survive that selection. The render correction preserves them without moving the ball; this is a port defect, not a new original-game bug.
- The old period helper's missing positive-anchor Y negation was a **port error**. Original `$86:DDE7..DDED` negates Y; v2 correctly restores it.
- Duplicate host owners for actor `+56` (`target_x` / `special_contact_raw_56`), `+58` (`target_y` / `mode13_variant_raw_58`), and `+60` (`reaction_threshold` / `contact_action_timer_raw_60`) are port state-mapping gaps. Shared original storage alone is not a defect.
- A scratch pointer retaining its value is not automatically a “farthest-player bug.” The precise comparisons and no-winner behavior above are established; generalized player symptoms and intent are not.
- Verifier acceptance of malformed metadata, discarded SPC output latches in port code, and snapshot-based initialization would be port/tooling defects. They are not original-game behavior to preserve.
- This catalog does not claim freezes are complete game fixes, that controlled edge cases occur naturally, or that standalone preservation is already enabled in the current executable.
