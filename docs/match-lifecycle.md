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

## Increment B: expiry and gameplay handoff

The session now carries typed live, horn-flight, period-presentation,
period-restart, postgame-presentation, and final phases. The `$09B4` latch and
the exact owner/Z/`$0946` horn gate keep an unresolved high ball live. Once the
gate resolves, the runtime applies the common and period-specific stamina
grants, updates raw period `$0926`, selects regulation/OT clock tables, resets
the shot clocks, reverses anchors on raw-period 2 entry, and either resumes a
quarter/OT or marks a non-tied period-four result final.

Controlled Mesen runs now inventory the presentation children and their host
wait lengths: 1187 ticks for Q1, 1547 for halftime, 1367 for regulation-tie
overtime, and 1756 through the Exhibition postgame boundary. These replace the
arbitrary 120-tick wait. `$87:C2F3`, `$87:CC36`, `$87:D2AE`, and `$83:FA91`
are witnessed scene entries, but their decompressed tile composition is not
yet fixture-backed; the host final panel is therefore structural, not claimed
pixel parity. Season/playoff persistence below `$87:985C` remains separate.

`tools/match_lifecycle_probe.c`, run by `build.ps1 -Test`, reads all eight
values back from the canonical ROM, compares the C helpers, and protects
default timeout/lineup persistence plus production Tipoff binding for both
regulation and overtime.

`tools/match_lifecycle_expiry_probe.c` replays the four checked-in Mesen
witnesses, shot-in-flight horn branch, and two-tick Q1/final handoffs. A short-clock headless smoke
also proves that gameplay advances from raw period 0 to 1 and reseeds the
clock instead of remaining at zero.
