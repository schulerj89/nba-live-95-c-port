# Gameplay completion goal

This goal closes three connected retail-gameplay areas without treating host
approximations or captured-address closure as whole-game equivalence.

## Evidence baseline

### Longer possessions, dribbling and ball attachment

The isolated attachment tables and render resources already have native-vector
coverage, but the production regression excludes dynamic dribble animation
bases 9 and 11.  A 20,000-frame audit found 1,936 excluded rows among 11,464
attached-ball rows (16.9%).  The first new differential must retain consecutive
native checkpoints around:

- `$86:F34F-$F439` owner decisions;
- `$86:E4A7-$E5AA` dribble-pose selection;
- `$87:AD5B-$AEC2` animation cadence;
- `$87:B649-$B67B` attached-ball position; and
- `$85:A4F2-$A597` attached vertical response.

The C harness will expose `owner.pose.end`, `behavior.end` and
`ball.attach.begin/end`, then remove the bases-9/11 exclusion only after an
exact transition-aware oracle passes.

### Fouls and free throws

Classification (`$86:C4FE-$C6AC`) and pending-event consumption
(`$85:93F5-$945E`) have strong native replay evidence.  Completion still
requires:

- full foul-out bookkeeping at `$86:C493-$C4FD`;
- complete free-throw state sequencing at `$87:9CBF-$A017`;
- shooter/lane readiness at `$87:A15C-$A360`;
- state-2 presentation startup at `$85:9530-$9597`; and
- one-shot, two-shot, and-one, final-make/inbound and final-miss/rebound gates.

Foul-out must publish a typed substitution request until the independently
captured substitution transaction is implemented.

### Match lifecycle

The existing clock writer `$85:EDC6-$EE3D` is translated, but its callers are
not a match lifecycle.  The current hardcoded 43,200-tick seed must be replaced
by the ROM tables selected by `$17B1`:

| Setting | Regulation ticks | Overtime ticks |
|---|---:|---:|
| 3 minutes | 10,800 | 7,200 |
| 5 minutes | 18,000 | 10,800 |
| 8 minutes | 28,800 | 14,400 |
| 12 minutes | 43,200 | 18,000 |

Period/end-game orchestration is rooted at `$87:95E9-$985C`; timeout/pause is
rooted at `$86:8300-$8605`, with confirmation at `$86:844E-$8495`.  Exact
substitution and postgame children require fresh native capture before labels
or behavior are claimed.

## Ordered increments and gates

1. Add the persistent lifecycle model and exact regulation/overtime clock
   initialization.  Test every length and the session-to-gameplay binding.
2. Add consecutive dynamic-dribble attachment capture/checkpoints, fix the
   first native/C ordering mismatch and remove the bases-9/11 test exclusion.
3. Add clock-expiry, quarter, halftime, overtime and final-state orchestration,
   initially separated from presentation.  Actor, ball, RNG and clocks must
   freeze at the native boundary.
4. Add the native free-throw differential fixture, complete foul-out request
   bookkeeping and replace the compressed CPU free-throw scene.
5. Capture and implement timeout/resume, period presentation and their exact
   stamina/audio/fade ordering.
6. Capture the substitution parent, implement atomic lineup/appearance/role/
   fatigue/stat rebinding, and consume foul-out requests.
7. Capture and implement final/postgame children.  Season persistence remains
   a separate feature.

Each increment requires focused native/recomp/Ghidra evidence, a permanent
probe or differential fixture, the full release suite, a refreshed desktop
build, and a main-branch checkpoint.  Unknown branches stay documented rather
than being filled with guessed behavior.
