# Visible smoke checkpoint harness

`tools/run_visible_smoke_checkpoints.py` is the single-command quick-test seed
for the visible defects raised during the August 31 recovery checkpoint. It
uses an unmodified production `nba95_port` executable and requires explicit
executable, ROM, asset-pack and output paths. It does not build or patch the
game.

The run covers:

- Team Select entry and the Team Select-to-Player Setup layer transition;
- neutral CPU-vs-CPU selection and Start skips for Matchup, Ratings and the
  complete Starting Lineup;
- all ten lineup cards and proportional text;
- team plates, variable logos and the Orlando court/Ratings composition;
- the ordinary pass, first inbound through release and at least five naturally
  observed pass draw directions;
- actor-centered pass/inbound crops for judging hand attachment at native pixel
  scale, plus Player Lab body, head, jersey and number samples.

The harness invokes the existing frontend, Player Setup, Player Introduction,
court/logo, sprite-pose source and consecutive-inbound regressions. Complete
temporary BMP sequences are rendered so transition frames cannot be invented
by separate starts, then only the selected PNG checkpoints are retained. The
gameplay telemetry is kept as compressed JSONL. Every command and SHA-256
identity is recorded in `manifest.json`.

On the August 31 post-transparency candidate, the full capture and focused
regression run finishes in about twenty seconds on the development host. The
capture-only mode is intended for intermediate monitoring, while the default
run is the checkpoint gate. Asset acceptance is based on pack-v31 structure and
the required resource-287 `NBPDRAW1` identity; it does not pin one historical
whole-pack digest.

Run it from the repository root in PowerShell:

```powershell
python tools/run_visible_smoke_checkpoints.py `
  --exe 'C:\path\to\nba95_port.exe' `
  --pack 'C:\path\to\nba95_assets_pose.pak' `
  --rom 'F:\Games\SNES\NBA Live 95 (USA).sfc' `
  --output '.analysis\progress-screenshots\20260831T235900Z-visible-smoke'
```

The output must be a new directory inside an ignored
`.analysis/progress-screenshots` tree. Open `index.html`, then follow the manual
checks in `review.md`. A passing run reports
`PASS_READY_FOR_INDEPENDENT_VISUAL_REVIEW`; that means the automated route and
identity gates passed, while a reviewer still needs to inspect the contacts.
The harness never changes `.analysis/progress-screenshots/latest`.

`--skip-regressions` is available for a faster capture-only monitoring run.
That mode still uses the same production routes and internal trace assertions,
but its manifest records that the focused regression suite was skipped and it
must not be published as an accepted checkpoint.

This is a bounded test entry, not a full-game completion gate. `--team-only`,
`--player-setup-only`, `--tipoff-only`, the explicit long gameplay clock, and
the F9 Player Lab route are all called out in the manifest. They provide quick,
repeatable access to the reported scenes without pretending to be a natural
startup-to-final retail journey or original-native evidence.
