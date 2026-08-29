# Match lifecycle

## Increment A: persistent initialization

The session now owns the current native period word `$0926`, both teams'
remaining timeout counts (`$4715/$4795`), and the five active roster slots for
each side.  The initial timeout count is seven, witnessed immediately after a
real timeout decremented one native side to six.  The initial lineup remains
the previously adopted ROM order `2,0,1,3,4`, but is no longer hardcoded in
the court initializer.

Clock initialization follows the ROM tables exactly:

| setup quarter | `$86:E38A` regulation | `$86:E392` overtime |
|---|---:|---:|
| 3 minutes | 10,800 | 7,200 |
| 5 minutes | 18,000 | 10,800 |
| 8 minutes | 28,800 | 14,400 |
| 12 minutes | 43,200 | 18,000 |

`$86:DBDC-$DBE5` selects the regulation value through `$17B1`; later-period
initialization at `$86:DD2D-$DD44` selects the overtime value when `$0926` is
at least four.  `nba_tipoff_init` consumes this session state and still uses
the already verified `$85:EDC6-$EE3D` clock writer during play.

This increment does **not** implement clock-expiry detection, period
advancement, halftime, timeout menus, substitutions, or postgame routing.
Those callers require their own native captures and differential gates.

`tools/match_lifecycle_probe.c`, run by `build.ps1 -Test`, reads all eight
values back from the canonical ROM, compares the C helpers, and protects
default timeout/lineup persistence plus production Tipoff binding for both
regulation and overtime.
