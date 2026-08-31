# Retail mode and saved-state inventory

Implementation baseline: `7002fb1`, 2026-08-31. This is root's M0 source inventory
while C1/S1 implementations and checkpoint QA run independently. It is not a
claim that Season, Playoffs or Load Series are playable in the port.

## Original dispatcher and current missing callers

Raw original ROM bytes confirm the mode comparisons at80:DBA8,DBAD,DBB2 and
the concrete JSL targets below. The mode word is17AB. The fuller dispatcher also
uses07F8,0A5C,1753,492B,8E40 and8F48; the simplified rows below do not erase those
cancel/resume/demo/result branches.

| Mode | Observed original ordinary route | Current C route / required work |
|---|---|---|
| 0 Exhibition | DBA8→DBF3, clears15C3, then DBF6→82:809A; later DC41→81:A489 | Current mode0 enters Team Select then Player Setup; full initialization and return parity remain separate |
| 1 Season | DC24→81:C54F, then DC30→84:8103 unless1753 requests return; later Player Setup | Current code logs SEASON but enters no mode scene; inventory configuration, season hub/schedule/result/terminal children of these actual callers |
| 2 Playoffs | DBC6→84:A2F3; conditional DBD6→84:A328, DBDC→84:AB5F and DBE8→84:A9DD; later Player Setup | Current code logs PLAYOFFS only; close bracket/setup/selection and cancellation state before exposing a working route |
| 3 Load Series | DBB2's non2 path reaches DBF6→82:809A, sharing that family with Exhibition | Current code logs LOAD_SERIES only; trace mode3-specific behavior inside the shared family and original SRAM selectors before inventing a separate Load screen |

Postgame is part of each mode. Original80:DC88..DD33 branches on492B,1886,
17AB and16C5, returns Season throughDC30, and sends Playoffs through84:A7A9,
84:AB5F and84:A9DD until its terminal conditions. The current C postgame panel
returns to Setup after a host countdown/button press. It does not provide retail
progression, record updates or full terminal-screen behavior.

No native menu journey for these non-Exhibition branches has been added by M0
yet. The next capture must use normal menu input and record first entry/cancel,
one match-result transition, save/reload and terminal/progression behavior.
Formats, season lengths, tiebreaks, record screens, roster-management features
and exact Load Series scope remain facts to establish, not values to invent.

## Configuration SRAM transactions are separate

The original save routines divide responsibility. A host serializer must not
write every session field at every menu commit.

| Original source boundary | Original writes / meaning | Implementation consequence |
|---|---|---|
| 81:C3D5..C41D | Saved Custom thresholds53/54 and Boolean bitmap55/56 | Rules Custom persistence is separate from active settings and the global validity transaction |
| 81:C0BA..C199 | 17A7→SRAM52; active values→48..4F; marker04=DA atC193 | Derive17A7's owner; do not substitute a physical pad or UI side without source proof |
| 82:8553..8563 | Team Select confirmation writes16B1→50 and16B3→51 as bytes | Team-selection save timing is a separate real caller, not performed byC0BA |
| 81:C19A..C231 | Validity check or factory initialization; factory Custom becomes45/45/07FC | A missing/invalid save resets the appropriate original fields, not the whole modern session structure |
| 81:C24B..C395 | Reload/validation and shared completion atC232 | Preserve original validation, conditional stores and downstream caller state |

Source validation accepts active thresholds and music/SFX volume below47 even
though the UI clamps to45. Style/level must be below3; Music Mode below3;
saved team selectors below34; saved17A7 below3. Team selector33 is remapped to24
for16B1 and21 for16B3. These source branches need explicit tests before a host
loader is accepted. Do not silently clamp unusual but accepted saves to UI limits.
Saved Custom is not independently clamped by LoadCustomRules.

Factory initialization writes raw16B1=9,16B3=17 and17A7=2. The current C session
chooses Chicago/Orlando by host team IDs and a legacy player-one side. Those
representations require a proven mapping and a matched initialization journey;
neither a snapshot default nor coincident numeric values prove ownership.

## Next vertical implementation slices

1. Map the native selectors,17A7, record pointers, validity markers and all SRAM
   read/write owners to current C fields. Derive the exact Season/Playoffs/Load
   graph with ordinary input; coordinate the shared native capture slot.
2. Implement source-correct serialization/validation and isolated host storage,
   wired at their real configuration and Team Select transactions. Verify a
   fresh-process reload with the existing original save chain, and add edge
   cases independently of candidate C. Never touch the user's emulator saves.
3. Deliver one whole retail-mode vertical slice: normal setup→selection→match
   result→progress→save→exit process→reload→continue. Then finish every original
   supported format and terminal screen. Source-state helpers alone cannot close
   this ticket; completed match service remains a dependency.

References: current `nba_game.c` mode/postgame callers; original
`.analysis/full-rom-census/listings/bank_80_instructions.tsv` and bank81/82
listings checked against canonical ROM bytes; regenerated
`.analysis/setup-config-native-20260830/reference/bank81.c`; existing
[configuration native contract](setup-config-native-contract.md) and
[independent completion matrix](completion-plan-audit-20260831.md).
