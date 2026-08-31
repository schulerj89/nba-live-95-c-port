# Live scoreboard and clock repair

The bounded HUD repair is enabled on the integration branch. The renderer now
uses the original indexed HUD working state instead of the historical captured
WEST2/ORLANDO0/11:49 panel. Scores and teams come from the current match; the
clock uses the original snapshot/formatter path. Temporary panels expire, the
shot-clock graphic updates, and the last minute retains its clock display.
Pause requests fresh scores and returning to play applies the reviewed resets.
Main's checkout, asset pack and desktop executable are unchanged.

The accepted controller freeze is SHA256
`60f9d3157369b0dfb19068424c0099ec6655ad1b0a34e27fd987c061755249c8`.
Root checked all 13,697 identities (2,430,971,039 bytes), then copied only the
five runtime C/header files and the new HUD probes, verifier and source notes.
The separately integrated asset pipeline was preserved. See the
[independent acceptance](completion-hud-lifecycle-independent-audit.md) and
[frozen source/evidence description](gameplay-hud-lifecycle-repair.md). Its
candidate wording records the handoff stage; this document records integration.

## Root verification

Fresh MSVC `/W4 /WX` builds compile the 40-source CLI, 40-source runtime probe,
40-source contract probe and nine-source native diagnostic independently.
Evidence remains under `build/hud-runtime-integration-v1`.

- Contract checks pass all 65,536 timer words and 655,360 shot/game-clock cases,
  42 malformed resources, legacy-resource refusal, reset preservation and
  three clear boundaries. The expected invalid-resource initialization error
  is part of the rejection test, not a failure to load the current pack.
- An ordinary C runtime journey, with no clock/score/actor seeds, passes
  120 paused updates, 182 formation holds, 7,630 dead-ball clock advances,
  1,942 dead-ball holds and 32,001 live clock advances. Its final-minute images
  change from59.8 to59.4. All eight BMPs match the accepted candidate byte for
  byte; pause entry/held images are identical.
- The known native capture supplies before-state inputs for746 comparisons.
  There are 2,747,926 matching bytes outside two differing16-bit observations
  of08F6. The strict run fails and is retained in `native-strict`; the explicit
  bounded run records those same observations in `native-bounded` and keeps
  `full_atomic_parent_pass=false`. Its input/output binaries are byte-identical
  to the reviewed candidate's. No native after-state is supplied to C.

The two observations describe the same interrupt crossing in a parent and its
child: C reads39945 at parent entry, while native execution reaches the inner
clock read after a decrement to39944. Native VRAM map differences from queued
upload timing are also retained. This is not a frame-perfect scanout or full
CPU/NMI scheduling pass. General verifier mutation certification remains open;
the acceptance is for the frozen capture and reviewed implemented behavior.

The retained ordinary 63,800-frame C trace completes. The existing long CPU
regression is being checked against it separately; this repair does not claim
whole-game regression acceptance or resolve the known period-restart gap.

## Behavior deliberately left incomplete

Original CE36 can choose statistics instead of a scoreboard. The reviewed
capture selects kinds1,6,1 across its first three requests. The repaired caller
stores the original shot category/assist inputs and consumes the shared RNG
in the original selector order, including rejected draws. It does not force
every basket to show a score panel. Untranslated statistics, advertisement
uploads and complex foul-clear children are reported explicitly in stderr;
they remain port limitations, not original-game bugs to preserve.

The C pause path holds its game clock, actors and visible image. Native paused
08DE/callback behavior is not reproduced or claimed: the C early return also
holds that timer, while original EDAC callback ownership still needs the
hardware scheduler. Return resets that state through B99A/BA54. Normal human
gameplay remains gated. The original shot-clock endpoint quirk is retained and
commented, with its source-only limits in the
[known-original-bugs catalog](known-original-game-bugs.md).

## Reproduction and preserved dependencies

Use the [HUD asset instructions](gameplay-hud-assets.md) to build a pack with
complete version2 resource286. The older pack without it is rejected when
entering gameplay. For fresh probes, choose a new directory for each kind:

```powershell
python tools/build_gameplay_hud_probe.py --kind cli --output build/hud-check-cli
python tools/build_gameplay_hud_probe.py --kind runtime --output build/hud-check-runtime
python tools/build_gameplay_hud_probe.py --kind contract --output build/hud-check-contract
python tools/build_gameplay_hud_probe.py --kind native --output build/hud-check-native
```

The runtime and contract commands and the immutable native capture path are
recorded in the copied `build/hud-runtime-integration-v1/handoff.json`. For a
native replay, use the new probe, its `manifest.json`, this source checkout,
the HUD pack and a fresh output directory. Omit `--bounded-shared-clock-read`
to reproduce the explicit strict failure rather than the limited projection.

Updating the asset header's obsolete WIP comment would otherwise invalidate
the separately frozen formation dependency. Its exact old header is archived
at `tools/period_formation_dependencies_v1/nba_assets.h`; the preparation tool
checks the unchanged pinned hash and uses it only in the private standalone
closure. The live game uses the current `include/nba_assets.h`. All30 formation
dependencies and the alias map still validate; no old evidence was rewritten.
