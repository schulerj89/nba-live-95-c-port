# Standalone period appearance and support components

The reviewed C/header/probe bytes are copied unchanged into `src`, `include`
and `tools`. These components are intentionally absent from
`nba95_sources.txt`: this commit does not change the running game, fix the
whole restart, enable human control or claim native scheduling parity.

`period_appearance` implements the CPU period-parent projection of $87:AAB2.
It owns animation channels, resources, status/direction, RNG and the owner
pointer. Human palette handling and unrepresented CPU/DP/register state are
outside its domain. Original source choices remain commented, including the
canonical lower phase-count lookup with an alternate lower animation.

`period_support` implements $86:D85E assignment setup, $86:D5DB collision
object sorting and $86:E183-$E1A4 controller/pose/ball attachment. It preserves
the paired-actor help destination, second-call key-table carry, wrapped-sign
sort comparisons, equal-key behavior and carried ball fractions/velocities.

The semantic assignment/sort APIs require these raw adapter preconditions:

- Actor record IDs are canonical 0..9. The period parent establishes them.
- All 24 carried roster pointers agree with the selected teams' original
  $84:E640 tables. The typed API derives the corresponding asset addresses.
- The leading $34D1 collision-list sentinel is zero. Do not silently clear
  nonzero carried memory to make an unrelated caller fit this domain.

`tools/period_support_source_domain.py` checks these conditions against the
original ROM for diagnostic captures. Unsupported raw states are rejected,
not labeled original bugs or normalized into valid inputs.

## Evidence and independent acceptance

- Appearance final freeze: `build/period-appearance-freeze-v3.json`,
  SHA256 `d61ea18b12a15d90ecfbba1358bbb90ecdb57b6e8fda2f4762b198eebd9f3ce1`.
  40 native calls / 5,200 words; 14 output-protocol, 9 source-domain,
  58 local metadata and 12 independent metadata refusals. All 80 original
  native stdout/stderr files are unchanged.
- Support final freeze: `build/period-support-freeze-v2.json`,
  SHA256 `7a34dbcab0846133893dbd6d4c0a3b299f6bab94e17f408e455d4353d142a133`.
  11 native calls / 34,126 bytes; 17 protocol/metadata, 9 C-domain and
  39 raw source-domain refusals. All 33 original output files are unchanged.
- The auditor independently checked source behavior, controlled ROM cases
  and actual verifier command routes. Both derived routes reject seven
  malformed parsed-input cases before C executes, with old C reports
  forbidden as replay authorities. Every recorded field is bound to its
  own raw memory; source hook counts/order and CPU domain remain checked.
- Final composite acceptance:
  `completion-period-appearance-support-verifier-acceptance.md`, SHA256
  `2308fc97aa3fb9c40f80ef5fc772765418cd307d08fbdb91a683a87e4824483c`.
  Earlier rejected-verifier reports are retained separately.

The integration build `build/period-components-integration-v1/fresh-build-v2`
compiled thirteen sources with `/W4 /WX` and no borrowed object files.
`fresh-verify/report.json` records all 51 original native comparisons passing.
The first build's missing link dependencies remain recorded in `fresh-build`;
the corrected builder includes those dependencies explicitly.

## Reproduce with local evidence

```powershell
./tools/build_period_components.ps1 -OutputDirectory build/period-components-new
python tools/verify_period_components.py `
  --probes build/period-components-new `
  --captures build/period-restart-attribution-v1 `
  --pack build/full-extraction-v1/nba95_assets.pak `
  --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' `
  --output build/period-components-new-verification
```

The original ROM, pack, captures and executables remain local and are not
published. A separate before-only diagnostic composition now matches 125
DD97-to-E207 boundaries / 504,500 bytes, but that composition and its remaining
role/render gates require their own independent acceptance. DCA6-to-DD97,
the live NbaTipoff adapter, scheduling and the existing CPU parity failure
are not covered by this standalone acceptance.
