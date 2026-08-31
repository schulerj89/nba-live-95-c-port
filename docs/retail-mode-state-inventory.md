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
| 3 Load Series | Still inside Setup,81:BF81 tests mode3 and07F8; BF93 clears15C1 and BF99 calls82:D085. Only after a possible Setup return can the later dispatcher reach the shared DBF6 family | Current code logs LOAD_SERIES only; implement the original Load prelude, its cancellation1753 path back to Setup, and the loaded-series state before the later dispatcher |

Postgame is part of each mode. Original80:DC88..DD33 branches on492B,1886,
17AB and16C5, returns Season throughDC30, and sends Playoffs through84:A7A9,
84:AB5F and84:A9DD until its terminal conditions. The current C postgame panel
returns to Setup after a host countdown/button press. It does not provide retail
progression, record updates or full terminal-screen behavior.

Four independently reviewed normal-input captures now establish first entry
plus300frames, as recorded below. They do not establish cancellation, a match
result, save/reload or terminal/progression behavior. Those journeys remain next.
Formats, season lengths, tiebreaks, record screens, roster-management features
and exact Load Series scope remain facts to establish, not values to invent.

## Reviewed first-entry checkpoint

The four `build/retail-mode-entry-v2` runs start from isolated emulator power-on
with explicitly nonrandom zero-initialized memory and initially empty private
save folders. Scripts submit controller input only; they do not seed game state
or jump scenes. The actual title Start is one poll at1996 before Setup takes
over. Setup Right/Start pulses are three polls each. Each run exits successfully
300frames after the first requested native entry.

| Capture directory | First entry | Entry → final frame | Manifest SHA256 |
|---|---|---|---|
| exhibition | 82:809A | 2447→2747 | `76b830299b7ce1f682dc18c1a86cc8db847459cf08cb68f96ef8dd82bb40db76` |
| season | 81:C54F | 2507→2807 | `d40e9581db06b7250c0b9cf459bd0091e41e46e079e6a9675d827535f494f937` |
| playoffs | 84:A2F3, then84:A328 | 2567→2867 | `8f81c198869503eea38caf2340147d73a45221608653d3a36798a8b241c116c1` |
| load-series | 81:BF99→82:D085 | 2628→2928 | `8aa9c2d7bdd5079b97cba8b4412d5d954b6e2ec2ce76245e239e0fbc188e6b27` |

Independent QA checked all69artifacts,11,345input rows,29events and1,218raw
field words, exact ROM/emulator/tool identities and private settings/save
isolation. Its receipt is `checkpoint-qa-20260831/build/m0-mode-entry-audit/independent-audit.md`,
SHA256 `726fdbbb10c2d0c379cc96f9289d26bf701ba9b4f869948062cbbe6128cc38aa`.
The v1 Load attempt missed the actual prelude hook and did not complete; its
failed evidence and the subsequent exact-private-process stop are retained.

The current tools are `capture_retail_mode_entry.py` SHA256
`f2d326cb9c58cbf1d836b1e7d120993b0955ab0ce399631db7809f13542e3f14`
and `mesen_retail_mode_entry.lua` SHA256
`8462620978da3d4664d3ab460c0323ca55eb382a6a53657dc4d20cb3db9e9337`.
Mesen's580Exhibition/579other uninitialized-read warnings remain in stdout.
This is bounded route evidence, not a clean-memory, full-mode, timing, rendering,
save/reload or general verifier-certification claim. No native screenshots were
produced here; the separate gallery shows the current C port.

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
