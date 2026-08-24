# Gameplay Lab and ROM comparison telemetry

F8 opens the live Gameplay Lab after the tip-off scene begins. It overlays all
ten actor slots on the court without replacing the ROM-derived player/court
assets. Up/Down selects an actor, Left/Right changes the detail page, A pauses
or resumes simulation, and X advances exactly one simulation frame while
paused. F9–F12 retain their existing Player Lab, HUD, audio, and asset roles.

The CLI exposes the same state as newline-delimited JSON:

```powershell
.\build\nba95_port.exe --headless --rom '<rom>' `
  --assets .\build\nba95_assets.pak --tipoff-only --frames 220 `
  --gameplay-trace gameplay-port.jsonl
```

`--gameplay-lab --gameplay-actor 0..9 --gameplay-page 1..3` renders a lab
proof frame. `--gameplay-paused --gameplay-step-count N` verifies deterministic
single-frame stepping in headless runs.

F8 page 3 and the normal CLI debug state expose
`PLAY:code/step T:count W:wait R:request`.
JSON adds raw `$0942/$0946/$0994/$0998/$099A/$099C/$099E/$09A2/$09A4/$09C4/$09D0/$09DA`
values plus the three
side-relative `$09AA/$09AC/$09AE` selectors. Positive countdown records advance
once per completed 30-Hz actor pass. Negative event records preserve signed
underflow while `$85:B24C` scans the active five actors; signed actor `+$16`
or `+$7E & $40` releases each actor, and all five clear `$099E` and advance.
Each actor's JSON `raw` object also exposes `controller_assignment_16`,
`movement_magnitude_4c`, `recovery_inhibit_7a`, and `behavior_flags`.
Mode-15 rows additionally expose actor `+$62` pass band, `+$66` direction,
`+$84` saved mode, signed `+$C0` family, the selected release threshold, and
whether `$86:A6B3` has detached the ball.

## Confirmed ROM map

The initial Ghidra/Mesen audit established these hooks. Raw values remain in
the JSONL alongside interpreted C fields so later labels can be corrected
without changing the comparison contract.

| ROM/WRAM location | Evidence-backed role |
|---|---|
| `$80:CB8F–$CD7D` | controller auto-read into held masks `$0576–$0580` |
| `$86:8000–$8212` | controller ownership, assignment, and repeat timers |
| `$87:9B38` | gameplay controller-mask accessor |
| `$093E/$0940` | current possession/catcher actor index and resolved actor pointer |
| `$0952/$0954` | inbound side and provisional actor; `$85:C37D` always derives actor 2/7 from side 0/5 |
| `$34EB + slot*$100` | ten player actor records |
| `$3EEB` | ball actor; position `$3EEF/$3EF3/$3EF7` |
| `$0946` | transient possession/ball-owner player index |
| `$87:9244/$9BD0` | actor `+$5E` behavior-mode dispatch |
| `$85:BC43–$BD7D` | assignment target, direction, distance, control mode |
| `$85:B95C–$B9D1` | AI reaction threshold using ball distance and RNG |
| `$87:B649/$B66A/$B832–$B995` | resource-driven attached-ball X/Y/Z composition |
| `$86:CED6–$D43C` | tip contact and possession resolution |
| `$85:B100–$B28B` | randomized initial possession/play decision |
| `$85:B128–$B24B` | `$0994` play-request consumption, strategy selection, and stream reset |
| `$85:C661/$85:C729` | 29-team coin strategy bytes and seven play base/count ranges |
| `$85:B402–$B4B8` | velocity-biased target arrival/direction with inclusive tolerance |
| `$85:AD6B–$AE1D` | play/step/role formation install and side/mirror transform; actor `+$7E bit $08` latches it |
| `$85:AE35–$AF5B` | crossing/edge target routes; normal arrival sets actor `+$7E bit $40` |
| `$85:B4B9–$B50D`, `$85:F5E4–$F715` | `$09A2` clear-lane cutter cadence and blocker rectangle |
| `$0948` | canonical detached-shot activity; ORed with `$097C` for formation edge routing |
| `$0928` | match clock: 43200 at live gameplay frame 220, then one decrement per outer frame |
| `$09C8/$096A` | detached-shot shooter and retained shot value used by descending contact/interference |
| `$86:9C6F–$9CDA`, `$86:A7A0–$A7A7` | pass launch records and animation release thresholds |
| `$85:F3C3–$F472` | fine 16-direction and weighted-distance helper used by pass setup |
| `$86:AB2D–$AF65`, `$86:A6B3–$A790` | mode-15 pass animation selection, attached phase gate, and release |
| `$85:9192–$93F4` | camera subject transform and adaptive approach |
| `$85:8EE6–$90C3` | circular court streamer sourced from `$A0:8006` |
| `$85:9D40–$A079` | final hoop/rim/made classification |
| `$85:F1C1` | weighted max/min hoop-distance helper |
| `$86:A561–$A5AF` / `$85:ABFB` | two/three-point arc classification/table |
| `$85:A1E9–$A26E` | score write and post-make inbound initialization |
| `$85:C37D–$C5C0`, `$86:F3D2–$F653` | inbound steering, receiver and pass path |
| `$85:C37D–$C600` | inbound layout `$0956` to actor 2/7 target `$0958/$095A/$095C` and play request |
| `$86:F34F–$F439` | ordinary mode-11 owner cadence: `B678`, optional `AD6B`, then `B50E` |
| `$86:F43A–$F669` | inbound `[-9,+8]` arrival, `$092E` 300/240/120/60 gates, selectors and `AB2D` |
| `$85:A3B7–$A656` | ownerless Z/gravity, ground restitution, damping, and settle |
| `$85:A656–$A755`, `$86:A613–$A628` | shared actor/ball rectangular and isometric court clamp plus boundary-state cancellation |
| `$85:B50E–$B677` | persistent A2/AA/AC/AE receiver selection and candidate validation |
| `$86:B625–$B978`, `$86:9D6E–$A45E` | pose-attached mode-12 shot jump, velocity release gate, and launch |
| `$86:BAA2–$BC99` | shared catch/rebound ownership install, `$0994` request, and final shot-context reset |
| `$86:C4FE–$C6AC`, `$86:D12D–$D1D0` | foul/contact classification feeding pending `$0964` and actor IDs `$492D/$492F` |
| `$86:CD97–$CE65`, `$86:D078–$D128` | descending-shot pose contact, code-6 interference award, and detached catch RNG |
| `$85:93F5–$945E`, `$87:92A5–$95E6` | pending-event consumer and dead-ball violation/foul dispatch |
| `$87:9B41–$9BC8`, `$86:F56E–$F577` | shared dead-ball initializer and exact inbound-arrival whistle release |
| `$87:BACB–$BAF4`, `$83:EBD8–$EE4F` | whistle presentation-object queue and visual-control state |
| `$87:9AA6–$9BCA` | expired inbound: reload `$092E`, layout 5, opposite `$093A ^ 5` side, clear `$093E`, and restart |

Owned-ball defensive contact and descending-shot interference now originate
from their proven `$86:CCFC-$D205` predicates. The reusable `$85:93F5`
consumer and code-6 restart are represented and tested. `$87:BACB` is not an
audio call: `$80:8CD0` queues a visual object whose `$87:EC5D` payload is a
ROM descriptor stream, while `$80:8AD2` in `$83:EBDB` is VRAM DMA. Gameplay
whistle audio comes from the independent `$85:9413 -> $82:FEF4 -> $80:9DF3`
command-$44 path. It uses asset-pack SRCN `$12` on voice 4 at pitch `$0556`,
VOL `$14/$14`, ADSR `$8E/$A0`; the C port logs those exact parameters whenever
the `$09B6` whistle edge queues playback.

The extended CPU-only capture covers 1,801 frames. It confirms the `$35`
post-tip play, CPU reaction staggering, live matchup reassignment, the
eight-direction movement map, ballhandler and offense changes, camera
projection, and the actual coordinate writers. The C state machine therefore
continues across possessions and uses separate attached/pass/shot/bounce ball
modes rather than a timed first-pass script. Complete rules, scoring, and
collision policy remain later gameplay work.

`tools/mesen_tipoff_capture.lua` writes the ROM-side equivalent to
`gameplay_rom.jsonl`. Compare it with the port trace using:

```powershell
python tools/compare_gameplay_traces.py `
  --rom-trace .analysis/<capture>/gameplay_rom.jsonl `
  --port-trace gameplay-port.jsonl --report comparison.json
```

For scheduler/physics comparisons, use `--logical-passes`. An SNES NMI can
interrupt `$87:8EFB-$8F92` after any actor, so one logical 0..9 pass may appear
as prefix/suffix slices in adjacent ROM JSON rows. The option coalesces those
slices and compares the completed state with one atomic C simulation tick:

```powershell
python tools/compare_gameplay_traces.py `
  --rom-trace .analysis/<capture>/gameplay_rom.jsonl `
  --port-trace gameplay-port.jsonl --logical-passes --mode all
```

Core mode locks phase plus all ten actors' identity, visibility, world
coordinates and visible sprite origins/directions/animation. `--mode all`
reports every common non-unknown field, making unfinished controller, camera,
ball and AI behavior visible without incorrectly passing it as implemented.

For a movement-oriented summary rather than an exact-field diff, run:

```powershell
python tools/analyze_cpu_gameplay_trace.py gameplay-port.jsonl --require-sustained
python tools/analyze_cpu_gameplay_trace.py gameplay_rom.jsonl
```

It prints every play/offense and ball-mode transition, per-actor and per-team
movement counts for each time window, and the maximum ball-to-owner attachment
distance. `--require-sustained` fails on stationary teams, fewer than four
recurring play codes, missing pass/attach/shot/inbound physics, or a detached
owned ball.

## Regression contract

`tools/test_gameplay_debugger.py` locks the F8 mapping, ten actor records,
eight-player settled-camera visibility, CPU-only mapping, controller,
ball, camera, collision and AI raw fields, overlay pixels, JSON parsing, and
paused single-frame stepping, and both comparator pass and intentional-failure
paths. `tools/test_cpu_gameplay.py` separately protects 60,000 frames of live
CPU behavior, score monotonicity, made-ball Z, `$0952/$0954`, the 300-tick
inbound gates, and resumed possessions for both teams.
It also verifies that pose collision may replace provisional slot 2/7, that
the installed dead-ball `$093E` actor coexists with logical ball owner `-1`,
and that the current inbound actor owns arrival and transfer rather than the
initial provisional slot.
Actor JSON includes raw contact inhibit `+$5A` and the asset-composed body
height `+$AA`. The sustained regression also locks `$87:9C3A->$86:A5B0`:
once `$0946` becomes negative, mode 10 must normalize through `$86:9846`
within one 30-Hz actor pass instead of drifting indefinitely.
