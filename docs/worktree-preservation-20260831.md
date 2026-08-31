# Worktree preservation checkpoint

The user requested committing and pushing all project work on 2026-08-31.
The integration branch retains the reviewed combined source. The other nine
worktrees are preserved as separate archival branches, including unfinished,
rejected and superseded experiments. They are not merged into main, and their
snapshot commits do not establish combined build or gameplay acceptance.

| Worktree | Branch | Snapshot commit |
|---|---|---|
| Independent auditor | `work/completion-auditor-20260830` | `dfbadbdfb342696edcd895f7f383ad131b674d2a` |
| Controllers | `work/completion-controllers-20260830` | `b8bbbcd300cd8c84d20a69844cb8a99921e1bcc9` |
| Scheduler | `work/completion-scheduler-20260830` | `fa0d0dee5c01d7d13292e2cc309ae30f0916564a` |
| Audio events | `work/audio-events-20260830` | `05a6ca5b2b791d056db330cd7fa02f400f530c81` |
| Intro transitions | `work/intro-transitions-20260830` | `22f3db1716aa1e6174b8203aebff4ba084529141` |
| Rules reentry | `work/rules-reentry-20260830` | `49e22eb0d22cc6650453e196bb82bbb9038ec56b` |
| Setup configuration | `work/setup-config-20260830` | `a04dc1c2b4866ca2d2235318313a27ab4612348a` |
| Team context | `work/team-context-20260830` | `f1ce288d95070dd3cec3176f3a655d7e55a7e17a` |
| Previously detached team-context audit | `work/team-context-audit-preserved-20260831` | `3f81a632b9773d7d3ddfa98dc5c9c111e3f7440c` |

All 641 changed source, test, fixture and documentation files in those nine
worktrees were checked against their staged bytes using SHA256. Path-specific
Git attributes preserve their exact line endings on the archival branches.
Their original working files were not rewritten. Local preparation receipts
are in primary `.analysis/preservation-20260831.json`.

The integration branch also commits the previously pending Rules reentry test
migration to explicit Simulation/three-minute setup and the already documented
input-release phase mapping. The native fixture and expected graphics/state
comparisons are unchanged. A fresh run against the HUD integration executable
still fails with **158 RGB/state mismatches**, first `native1176/C893: brightness`.
The retained log is `build/hud-runtime-integration-v1/rules-reentry-preservation.log`.
This checkpoint records that failure rather than weakening its assertions.
The separate CPU pass-state failure and other acceptance limits remain in
[the HUD integration report](gameplay-hud-integration.md).

Main remains at `caa91344879d3bdb408da86dc34683c6ed01ca2c`, and the desktop
executable is not replaced. Ignored captures, screenshots, compiler output,
ROMs and asset packs stay local. Ten untracked compiler logs in the controller
and team-context worktrees are now explicitly ignored, without moving or
deleting them. No captured evidence is newly published by this checkpoint.
