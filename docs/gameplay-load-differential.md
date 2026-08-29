# Gameplay appearance/load differential

Fresh natural Mesen captures execute `$86:E0B0-$E207` and the adjacent
`$86:E208-$E24B` table initializer during the real pre-game load. The compact
fixture compares 19 represented persistent words against `nba_tipoff_init`,
including live/dead-ball state, selectors, ball position/velocity, inbound
state and cached focal coordinates. The replay found and corrected one real
difference: dormant pre-tip `$0954` is zero, not `FFFF`.

The resource outcome is additionally guarded by the existing active-
appearance runtime sweep across all 29 teams (300 actor records), exhaustive
asset-pack validation, exact jersey/body/head composition, and atomic missing-
resource rejection. Native E208 configures the default human controller;
the CPU-vs-CPU verification mode intentionally normalizes controller
assignments after load, so controller ownership is not included in the 19-word
comparison.
