# Period restart: current runtime state mapping

This inventory is an implementation prerequisite, not acceptance of production
restart. It describes the integration worktree at `dc10166`. The audited entry
prefix and the independently reviewed/under-review formation components must
consume current owned data, never native checkpoint images. The old
`match_restart_period` is still present and remains incorrect for regulation
formation: it invokes the foul/dead-ball parent instead of DCA6/E207.

## Existing owners to reuse

| Original field | Current owner or mapping |
| --- | --- |
| 0926, 0932 | `period_raw_0926`, `tip_winner_group_raw_0932`; the caller increments the period before the prefix. |
| 0928 | `match_clock_raw_0928`; distinct from the carried selected period length at0A0C. |
| 092C, 0994, 0996 | `rim_raw_092c`, `play_request_raw`, `play_code`. DD47 does not write09C6. |
| 08F0 | `fouls.latched_event_raw_08f0`. |
| 0910, 0918, 091A, 0922 | `catch_actor_record_raw_0910`, `role_focal_x_raw_0918`, `role_focal_y_raw_091a`, `shot_previous_actor_x_raw_0922`. |
| 091C | `shot_bounce_timer_raw_091c`. |
| 0936, 093A | `live_state_raw`, `camera_side_group_raw`; source side words are0/5, not host team indices0/1. |
| 093E, 0940 | `possession_actor`, `camera.subject_pointer_0940`; logical `ball.owner_actor` is not a substitute for the original owner word. |
| 0942, 0944, 0946 | `pass_actor_raw`, `pass_aux_raw`, `pass_receiver_raw`. |
| 0948, 094A, 094C | `ball_activity_raw`, `rim_raw_094a`, `shot_value_raw`. |
| 0952..095C | `inbound_state_raw`, `inbound_actor_raw`, `inbound_layout_raw`, `inbound_target_x_raw`, `inbound_target_y_raw`, `inbound_direction_raw`. |
| 0962, 0964, 0966 | `rim_raw_0962`, `fouls.foul_event_raw_0964`, `dead_ball_raw_0966`. |
| 0968, 096A, 097C, 097E | `dead_ball_raw_0968`, `rim_raw_096a`, `rim_raw_097c`, `dead_ball_raw_097e`. |
| 09B0, 09B2, 09BA | `dead_ball_x_raw_09b0`, `dead_ball_y_raw_09b2`, `inbound_ready_raw`; preserve the original continuing-period carry. |
| 09B4, 09B6, 09B8, 09BC | `dead_ball_dispatch_busy_raw_09b4`, `fouls.whistle_active_raw_09b6`, `inbound_transfer_raw`, `fouls.shooting_foul_raw_09bc`. |
| 09C0, 09D0, 09D2, 09D6 | `assistance_team_raw_09c0`, `play_hold_raw`, `role_cadence_raw_09d2`, `role_rebuild_raw_09d6`. |
| 09F6, 0A02, 0A04 | `attached_ball_state_raw_09f6`, `deferred_shot_foul_phase_raw_0a02`, `dead_clock_enabled_raw_0a04`. |
| 4933, 4935 | `context_raw_4933`, `context_raw_4935`. |
| Context0A,26,41,49,56 | `team_context` owns anchor, score, controller actor, actor-order bytes and play selection. Context offsets are hexadecimal. |
| Actor16 / context3B,3D | `controllers.actor_assignment`, `controllers.count`, `controllers.cursor`; synchronize the old actor assignment mirror after actual controller work. |
| RNG07F6 | `rng.state`; appearance and role work must share this state in source order. |
| Camera0860 | `camera.y`; avoid a newer camera value when a source boundary requires retained presentation data. |
| Basket3FEF | `court_presentation.basket_x_3fef`, updated by `nba_court_presentation_update` at85:8E28. Keep the value through the period-two anchor flip. |

Prefix4713/4793 clear context28. The existing `strategy_raw_2e` is a different field and must
not be cleared in its place. The current team-context structure therefore
needs a separate owner for28 before representing that prefix write.

## Actor mappings and aliases requiring care

Original4E/50/52 map to the current actor's `movement_direction`,
`requested_direction`, and `direction`, respectively. The standalone parent's
field names differ; adapters must follow offsets rather than matching names.
Source34/36/3E/40 are published pose fields, distinct from channel30/32/3A/3C.
The current actor does not retain all four published fields.

`actor_animation_channels` copies both complete three-entry queues into the
typed channel record. Its current store helper copies cursors and state but
does not copy queue contents back. Period adoption must preserve and commit
the full arrays. The legacy animation advance also applies its own cadence
and must not replace the accepted AAB2 child.

There are duplicate host representations of original actor words:

| Original word | Current host fields |
| --- | --- |
| +56 | `target_x`, `special_contact_raw_56` |
| +58 | `target_y`, `mode13_variant_raw_58` |
| +60 | `reaction_threshold`, `contact_action_timer_raw_60` |

These are port ownership gaps, not original-game bugs. A production adapter
must reconcile the aliases; writing only a convenient mirror leaves a later
mode reading a different value from the same original word. Unifying storage
needs normal-play regression checks, not just a copied checkpoint test.

Original position words have sixteen fraction bits; host positions currently
have eight. `fp_integer_word` is the proper integer projection, not `fp_round`.
The ordinary captures examined so far have zero low fraction bytes. This
observation does not authorize dropping arbitrary carried sub-byte fractions
or claiming the existing host representation covers all native states.

## State that needs explicit ownership or lifecycle work

- Selected period length0A0C: initialize from the original new-game table and
  carry it through regulation. Overtime updates it from86:E392. Recomputing
  every regulation period from current settings is not the prefix behavior.
- The12-record draw permutation7E44: initialize through the80:FBE9 sequence,
  then preserve ordinary80:FC80 reverse adjacent passes and exceptional full
  FBFF sorts. Static actor iteration cannot supply the carried permutation.
- Basket Y3FF3 has an explicit zero writer at86:DBC2. This is initialization
  provenance, not permission to fabricate arbitrary runtime inputs. Basket X
  already has an owner and must not be duplicated or derived after the flip.
- Source09DA..09EC is one ten-word scratch/result buffer. Existing
  `pass_distance_raw` and `role_nearest_offense_raw_09de` represent aliases at
  slots0 and2;09E2 is slot4. The assignment child writes the shared buffer
  before role work. Independent copies cannot remain authoritative.
- Context28, original093C last-side state,4015/401B basket presentation state,
1864, source frame counters084A/084C, collision-list links and A6 require
  precise ownership or a documented before-write/output-only classification.
- The sourceC6 dispatch delta is an explicit formation input. All four captured
  period cases have2, but the adapter must establish its scheduler owner or
  declare that bounded domain rather than infer a general elapsed-time model.
- The carried24-entry roster address table is initialized by the original
  roster path. Do not replace it silently at each restart with fresh lookups
  merely to satisfy a child guard. Active roster/statistic pointers are then
  produced by the actual D85E assignment child.

Do not reuse `prepare_substitution_actor_bindings` as the period assignment
child: it also performs legacy appearance/geometry work that can overwrite
the preceding AAB2 outputs. Do not reuse `ball_attach_to_actor` to publish a
different logical mode or clear original latches during the source sequence.
The old restart also clears13F7 and writes09C6 without corresponding entry
prefix writes. Those port changes are not original quirks to preserve.

The next acceptance step is a real runtime adapter plus ordinary CPU replay,
including the currently failing frame49412. Standalone native data proofs
alone do not establish scheduler timing, visible period transitions, or
whole-game parity.
