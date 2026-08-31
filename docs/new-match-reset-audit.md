# Independent new-match reset audit — 2026-08-30

**PASS for the bounded reset repair. Whole-game acceptance remains FAIL.**
The integration owner reviewed the gameplay implementer's source and evidence
directly, independently of its completion summary.

The reviewed production call is Exhibition confirmation in `nba_game.c`, before
Team Select. `nba_session_begin_match` clears match-owned state while leaving
teams, side choice and every configuration array intact. It is not called on
Rules/Options return, timeout resume, or the later-period path. The old FINAL
state does reach the early return in `nba_tipoff.c`; the retained pre-fix caller
log shows the subsequent Exhibition preserving period5 and the prior score.
The fix therefore addresses a real production lifecycle failure.

The audit read the original ROM and the generated
`NewMatchClockAndPeriod_M0X0` recomp slice at `$86:DBD1-$DBEA`. Its branch on
`$07F8` can bypass the configured clock lookup, but both paths reach the period
clear at `$DBE8` (`9C 26 09`). The later-period `$DD2D` lookup preserves period.
This supports the reset distinction; it is not verification of the unresolved
continuation beyond `$DBEA` or of the complete initialization routine.

The verifier reread the original first-court WRAM and its capture manifest,
checked their identities and the ROM identity, and extracted all29 represented
values: period, two scores, two timeout counts, and two twelve-slot roster
orders. All matched the external fixture and the production reset output.
Two controlled completed-C-match seeds then followed real input publication
and scene dispatch through Postgame, Setup, Team Select, Player Setup,
introductions and Tipoff. Both reached live period0 with their configured
clock, retained altered configuration/team/side choices, and advanced60 ticks.
Seven verifier tests reject malformed/provenance data and all29 value changes.
Root replay: `.analysis/new-match-reset-20260830/root-independent-replay.log`.

Caveats retained:

- The native witness is first-match initialization with disclosed pregame
  team/controller selection injection, not a natural second-match journey.
- Native host-enum, availability, presentation, pixel and audio equivalence is
  not established by this projection. The C liveness ceiling is not a native
  timing tolerance.
- Human ownership, native factory defaults, all-team roster semantics,
  Season/Playoffs persistence, and full initialization/RNG/scheduler alignment
  remain open. The reset helper adds no instruction-coverage credit.

No unresolved defect was found in the bounded Exhibition reset contract.
