# Closure digest attribution, 2026-08-30

**PASS for frozen v3, final-v3 and final-dispatch-root bounded change
attribution; not whole-game ROM parity.**
The integrator independently reran the audit, checked baseline files against
Git, inspected the native capture/provenance and viewed the resulting image
before authorizing `773c1df2a9820701` → `fdbdd69c21271f89`.

`tools/gameplay100_closure_probe.c` previously mixed sampled image hashes,
five gameplay fields and five final session words into one digest. A changed
digest did not identify whether pixels, simulation or configuration changed.
Its optional `--diagnostics <existing-directory>` now retains separate component
digests and full sampled output without changing the legacy mixing order.
The expected digest was left unchanged while producing this evidence.

## Independent reproduction

`tools/capture_closure_diagnostics.py` exported baseline commit
`2723af610aab0ec63263a6449fa6a161a155f974` through `git archive` into a private
source tree and froze current sources in another tree. It compiled the same
diagnostic adapter against each tree using the same MSVC invocation. Neither
build used shared repository objects. Baseline and current runs each executed
the original closure twice, retaining every output. No expected value was
supplied to either probe.

For reruns after the approved default-golden update, the private builder reads
the historical golden from Git and supplies `NBA_CLOSURE_EXPECTED_DIGEST` only
to that baseline compilation. This changes only the probe's final acceptance
comparison. No simulation/render input receives it; actual old output must
still equal the separately checked historical digest and counters.

Accepted evidence is `.analysis/closure-digest-audit-20260830/v3/`:

| Item | Before | After |
|---|---|---|
| Probe executable SHA-256 | `5c44e87f67251e95010f25d556de0bfd496eff1447941070caa20738edbee8b4` | `154f4fad7a46164ebb649f5b72f4cb7f59e92f414195a5d99dd54510e0bafb29` |
| Pack SHA-256 | `d6adfe3ab8a49805a2cd10921281c33541135a332b4f2b174dfe25c093c2ebfd` | `126b7c8178451dfc76bb0200e3df3f41e7a26e647ee595517893e60b42c8b0c9` |
| Legacy aggregate | `773c1df2a9820701` | `fdbdd69c21271f89` |
| Five-field gameplay component | `2a077c2ee0cc8f28` | identical |
| Final session-word component | `54b25da22b72ea34` | identical |
| Render component | `4a977a934ee4af41` | `0d2ad7230908d429` |

Both revisions retain the same eight handoffs, 65 image changes, 2,910 motion
frames, 13,122 upper-resource changes and 72 possession changes. Counters alone
are insufficient; the audit also checks exact recorded contents:

| Compared output, per run | Result |
|---|---|
| All6,000 owned `NbaTipoff` snapshots, 21,264,000 bytes | Byte-identical |
| All6,000 `NbaSession` snapshots, 912,000 bytes | Byte-identical |
| All6,000 semantic telemetry rows, 133,562,859 bytes | Byte-identical |
| All6,000 historical five-field projection rows | Byte-identical |
| 66 sampled images | 65 identical; Rules sample1 differs |
| Each revision's second complete run | Every retained artifact identical to its first run |

The raw `NbaTipoff` record begins at `offsetof(NbaTipoff,frame)`, omitting only
the four preceding host pointers/callbacks. All members thereafter are value
state in this audited layout; initialization clears its padding. Header/layout
identity and record sizes are checked. This byte protocol is suitable for the
same-compiler C comparison only; it is not a portable save format or native
WRAM comparison. The independent integrator checked69 baseline source/header/
manifest files against Git. The baseline checkout's CRLF versus archived LF
explains raw source-hash differences for unchanged text.

## The changed Rules image

Only sample1 changes: Rules immediately after the bounded45→44 Defensive Fouls
adjustment, at internal C Setup frame228. The retained post-update BG2 scroll
changes22→23. Exactly15,136 pixels differ within `[16,0]-[255,223]`; every other
sample, including all50 gameplay images at120-update intervals, is unchanged.

A fresh private portable Mesen run executes the unchanged original USA ROM,
SHA-256 `2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`.
It selects Simulation/three-minute settings through ordinary controller input,
then adds a prescribed Left pulse at evidence labels619..621. No CPU, ROM,
WRAM, SRAM or PPU state is written by the script. Full WRAM/OAM snapshots and
surrounding RGB/PPU/raster data are retained. The selected comparison frame620
was prescribed before capture, not selected by minimizing pixel differences.
The working threshold is45 at618 and44 at619..621; row0 is retained throughout.

The after-image matches **all57,344 active RGB pixels** of original frame620.
The fixed Mesen256×239 buffer-to-visible conversion takes rows7..230, without
per-frame fitting. Active RGB SHA-256 is
`1f3c956bacce89b957edd8d91369ff6818f2f401b6bd6c48581cb0a4bb133733`.
Raw256×239 RGB SHA-256 is
`aecd95c32d117be41ad13c0ae369e9abaeef61351af4fbe7f48522a250680b1d`.

Native evidence is
`.analysis/closure-digest-audit-20260830/native-rules44-v1/`, with frozen private
capture sources under the adjacent `native-source/`. The runner records exact
ROM/executable/Lua/settings/source identities, an initially empty private save
directory and exit0. The script separately records its actual Mesen home; the
auditor checks it belongs to that private executable's Lua directory. Frame
skipping is disabled; RGB comes from synchronous `emu.getScreenBuffer()`.
`tests/fixtures/closure-rules44-native.json` retains the manifest and compact
observation, including the full13 working values and committed Main values
around the edit. It contains no captured production asset.

Native code `$81:D446-$D47A` adjusts the bar and `$81:D491` marks Custom.
That second effect was absent from the initial frozen v3 C comparison: native
committed Style was2 after the edit while that C snapshot retained1. The later
final-v3 comparison below covers the repaired side effect explicitly. The
image comparison alone establishes the configured render state, not full
configuration or machine equivalence.
`docs/transition-ownership-audit.md` and the independently audited consecutive
Rules reveal/steady fixtures establish the owning PPU/raster contract; this
extra witness covers the exact bar value rendered by the closure test.

## Final return and Custom caller replay

The Rules-return implementation was subsequently frozen and replayed in
`.analysis/closure-digest-audit-20260830/final-v3/`. Its pack SHA-256 is
`5d364ce926bbb8d7c12a51990e3a7409a17a5a45350b0cc6838db5ed16b1193f`.
The baseline pack is preserved as
`.analysis/closure-digest-audit-20260830/baseline-assets.pak`, SHA-256
`d6adfe3ab8a49805a2cd10921281c33541135a332b4f2b174dfe25c093c2ebfd`.
Do not substitute the later contents of `build/nba95_assets.pak` for it.

The final closure still produces `fdbdd69c21271f89`; no additional golden
change was needed. All6,000 owned gameplay records, semantic telemetry rows
and five-field projection rows remain byte-identical to baseline. All65 other
sampled images remain identical, including the Options image and all gameplay
samples. The changed Rules image remains the exact native620 image above.

Session records now have one deliberately bounded difference: **every one of
the6,000 records has old Style1 and new Style2 at the named
`config.main_values[1]` offset; every other byte is identical**. The auditor's
default still requires total session equality. Its explicit
`--allow-native-custom-style` mode requires this exact transition, checks the
named structure offset/layout, and corroborates it with native WRAM618/620 and
the C pre/post-edit render snapshots. It does not ignore the field or permit
arbitrary values. Protocol mutation tests reject wrong old/new Style values,
changes elsewhere in the initial/middle/final records, incorrect layouts and
truncated populations.

The repaired return selects Main row0, matching native execution. The old
closure test assumed row4 remained selected and pressed Down once to reach
Options. That assumption made the first final candidate stop at code16; the
failed evidence remains `final-v1`. The production test now asserts row0 and
sends five real Down presses. Only the private Git-baseline build defines
`NBA_CLOSURE_HISTORICAL_NAVIGATION=1` to reproduce its historical one-Down
journey. Final-v3 records actual `ui-inputs.jsonl` dispatches:12 before and16
after, including the different known return rows. The auditor verifies both
explicit sequences and their arrival at Options. This is a corrected caller
expectation, not a retry loop or relaxed navigation assertion.

`tools/rules_custom_caller_probe.c` separately checks the Custom effect through
`nba_game_input_update` and `nba_game_tick`. It enters C Setup directly from
an **explicitly supplied native preconfiguration**, then opens Rules/Options
through ordinary C menu input. Expected poststates never reach the C process.
`tools/verify_rules_custom_caller.py` reads native observations from
`tests/fixtures/setup-config-native-witnesses.json` and checks every committed
Main/Rules/Options word, every active working word, scene, page and row:

| Native actions | Exact caller behavior |
|---|---|
| `presets-v2`13..16 | Right at45 marks Custom despite the clamp; B does nothing; Left edits working45→44; Start commits and returns Main row0 |
| `rules-v2`7..8 | Left at0 marks Custom despite the clamp; Right then edits0→1 |
| `options-v2`7..9 | Three Left volume edits retain Arcade Style0 |

The independent native fixture contains raw source hashes and complete
observations, not expectations generated from C. The frozen caller report is
`final-v2/custom-caller-report.json`: three cases,12 snapshots, all exact.
The strengthened verifier, including an explicit Setup-scene assertion, was
rerun against that frozen executable in `final-v3/custom-caller-report.json`
and again passed. Six protocol tests mutate all411 committed/working words,
scene/page/row, malformed or incomplete outputs, and process failures. These
protocol doubles are verifier tests, not additional C/native parity cases.
Its executable SHA-256 is
`706f26db8e50ebbb19bfe0f6c22382acc837aca44db1905ed4fe96522b280ed4`.
This is controlled-prestate caller proof; factory defaults, Style presets,
saved Custom retention, disk persistence, repeat timing, rendered frames and
in-game rule effects remain outside this gate. The broader configuration
baseline in `docs/setup-config-native-contract.md` remains FAIL.

The final-v3 capture-manifest SHA-256 is
`b891d15f9a35c06ce4913a55d76ec1b0cb04cb56f60ade3a6c67d0187d97671e`.
The strict attribution report is `final-v3/auditor-report.json`. Final-v2 had
the same result before the actual UI-input trace was added. The integrator
authorized retaining `fdbdd69c21271f89` for that replay and requested a separate
independent review of the Custom caller and exact Style exception.

After the Start-dispatch rendering repair, the integrator froze the final
semantic source again as `final-dispatch-root`. The independent auditor reran
the strict attribution into `final-dispatch-root/options-auditor-report.json`:
**PASS with the same digest, same single native Rules44 image difference,
same exact Style1-to2 exception and all other recorded bytes unchanged**.
Its capture-manifest SHA-256 is
`be2ecf800ef4b6a82c52893fd013500d4ce91731fe821ed2e9c657946ad482eb`.
The separate native Rules44 witness and its field/image evidence did not
change. This final replay supersedes final-v3 for checkpoint source identity.
A historical rules-return audit in Git history independently covers the actual Start
frame and following171-frame return; the closure's sparse samples alone do
not cover that repair.

## Reuse and limits

```powershell
python tools/test_closure_diagnostic_integrity.py
python tools/test_rules_custom_caller_verifier.py
python tools/capture_closure_diagnostics.py --baseline 2723af6 --output .analysis/closure-new --before-pack .analysis/closure-digest-audit-20260830/baseline-assets.pak --pack .analysis/transition-ownership-20260830/nba95_assets_rules_return_candidate.pak
python tools/audit_closure_diagnostics.py --capture .analysis/closure-new --native .analysis/closure-digest-audit-20260830/native-rules44-v1 --allow-native-custom-style --report .analysis/closure-new/auditor-report.json
powershell -NoProfile -ExecutionPolicy Bypass -File tools/build_vector_probe.ps1 -Name rules_custom_caller_probe
python tools/verify_rules_custom_caller.py --fixture tests/fixtures/setup-config-native-witnesses.json --probe build/rules_custom_caller_probe.exe --rom "F:/Games/SNES/NBA Live 95 (USA).sfc" --pack .analysis/transition-ownership-20260830/nba95_assets_rules_return_candidate.pak --report build/rules-custom-caller-report.json
```

The auditor checks source/output hashes, complete sample/frame populations,
both repeated runs, exact state equality and the independently observed native
image. It does not update a golden value. The accepted report is
`.analysis/closure-digest-audit-20260830/v3/auditor-report.json`.
Frozen capture-manifest SHA-256:
`584fe2df2e77328ba89f2928271fbc7e64d4c64525851c286b70b0cc7bd4cb5b`.
Native manifest SHA-256:
`d80b2a1fa0a5da3dda930808658fb609ec721e9c4fb6f8e328d25f5249482daa`.

The historical probe calls scene APIs directly, assigns the return row, seeds
gameplay RNG`$5A17` and clock43200, and compares two C runs. It does not exercise
complete `nba_game` dispatch, natural native initialization, human ownership,
full-match lifecycle, every option, every rendered frame or audio. None of its
counts or digests proves those features complete. Subsequent transition changes
must be rebuilt and attributed again before accepting another digest. These
approvals apply only to the frozen comparisons above. A state-aligned Rules
image does not by itself establish the owning BG2 scheduler or the entire
return transition; that separate native routine and consecutive-frame review
belongs to the transition audit.
