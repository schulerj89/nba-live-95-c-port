# Literal player sprite composition checkpoint

This checkpoint implements `nba_player_compose_sprite_pose`, the body-layer
geometry and ordered `$80:B348` call arguments from `$80:AD92` and the shared
body portion of `$80:AF1E`. It does **not** migrate gameplay to these inputs.
The existing combined-direction renderer remains a compatibility entry, with
9,976 old/new output comparisons and an actual 390-frame C journey unchanged.

Base: `c172877f378a102a174f58e6eae936abc8e5c781`. Worktree:
`C:/Users/joshs/Projects/nba-live-95-c-port/.analysis/worktrees/sprite-pose-compositor-20260831`.
Only `include/nba_player_lab.h` and `src/nba_player_lab.c` change existing
production files. No tipoff/AI/session/source-manifest or physics changes.

## Source ownership

| Literal input | Original producer / consumer |
|---|---|
| D6 upper, D4 lower | Raw actor +2A/+2C stored at 87:A4E1/A517; not recomputed from head facing. |
| 47 status | 87:A61B/A61E copies actor+28 after A5FB–A609 updates **only bit2** for head facing. |
| Body mirror bit15 | 87:AB48/AB4B/AB4E clears actor+28 bit15; later AC13–AC22 sets it when resolved +52 is below3. This ordered producer is separate from rendering. |
| 51 head order | 87:A4F5 reads signed byte AC:B6B3[D6]; A504 stores the sign-extended word. |
| D8 jersey | A506 reads signed AC:C7E3[D6]. Negative is stored literally; otherwise A520/A525 indexes 87:A98E by actor+52. |
| DA head | A610 reads 84:C36E[selected head facing*2], then adds actor+2E at A615. |
| C0 versus C2 | A658/A65B copies actor+4E to C0. A653/A656 separately copies actor+52 to C2. Neither is selected head facing. |
| 4F attributes | A at AD92/AF1E entry, from caller A692–A697 or A69F–A6A2. Before-entry DP4F is stale and is not used as an input. |
| X/Y origins | Actual CPU X/Y at AD92/AF1E entry; no camera or world-coordinate reconstruction. |
| 0884 glyph work | Full word inherited at entry; INC before each jersey call and STZ afterward. FFFF wraps to zero, not a normalized boolean. |

AD9A–ADB4 / AF26–AF40 XOR masks1/2 when47 is negative, then independently
decode upper mask1, lower mask2 and head mask4. Signed attachment bytes are
extended before optional negation; the -128 case becomes +128. Coordinates
wrap as 16-bit words. Tables are A9:D86E/D03E for lower/upper attachments and
AC:D07B/AE1B for the jersey attachment.

AE50/AEC5 uses the **sign of51** for head first/last. AE69–AE71 performs
`DEC; CMP #5; BCC`, so C0=1..5 submits the jersey before the upper body; all
other words submit it afterward. Negative D8 suppresses the jersey. D8=0591
adds attribute4000 independently of body/head flips; every jersey adds0E00.
These are preserved original operations, including word wrap and unusual
controlled values. They are not declared original bugs without further proof.

The fixed output has four possible submissions, each with kind/resource,
X/Y, complete attribute word and observed parent0884 value. It also exposes
the nine native geometry/flip words and final parent0884. Invalid upper/lower
indices (outside0..082F), absent/invalid animation data and out-of-bounds
table reads reject without changing output. Source arithmetic is bounded to
native D/M/X=0. Resource allocation/validity beyond this parent boundary is
the external B348 consumer's responsibility.

## Evidence and limits

Canonical original ROM SHA256:
`2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`.
Original unmodified ball capture is referenced read-only at
`../ball-pass-alignment-20260831/build/ball-native-v1`, manifest
`4f17d6675caa4ea9ab6707389b9a7e1f39ecea56c2295efddc46982906305af9`.
It used normal controller input, private Mesen home/settings/saves and no
prestate injection. No new emulator process was launched for this checkpoint.

`build/sprite-pose-tests-final-v2/report.json` records:

- 43 complete native draws:24 AD92 and19 AF1E; all eight resolved directions.
  The C component reads only each actual draw entry. All387 native geometry/
  flip words match the actual actor return. Another301 before-state producer
  words bind51/D8/C0/C2/47/raw resources to the original tables/actor fields.
- Four natural47 values are0004: head flipped while upper/lower are not.
  Six natural51 values are negative: head last. Thus the old combined head/
  body flip and fixed head-first caller is still an explicit runtime defect.
- 149,632 controlled original-opcode AD92 cases compare5,386,752 C output
  words: all65,536 status words, all65,536 C0 words,1,792 facing/head-resource/
  order/number combinations and16,768 table-index/wrapped-coordinate cases.
  The1,792 facing combinations also exercise the AF1E body-order projection.
- 9,976 current-versus-base compatibility calls are byte-identical.
  Ten invalid resource-domain records reject. The separate fresh guard probe
  passes28 atomic refusals, including nulls, malformed headers, truncated
  data, overflowing offsets and unchanged output sentinels.
- 16 in-memory malformed build/protocol views reject, including missing
  sources/objects, wrong executable identity and extra/missing/wrong-pack
  diagnostics. This is bounded integrity testing, not general certification.

The original-opcode diagnostic is **test-only**. It intercepts B348 and B0FF
as explicit external calls preserving this parent's projected words; it does
not implement the child queue, allocation, OAM, palette/glyph uploads or ball
interleave. Native individual B348 entries were not captured here. Therefore
the ordered part stream is source/controlled evidence, not native submission
stream equivalence. Nine native words matching across the three recorded NMI
crossings do not establish CPU/interrupt/scanout parity. No whole D1 claim.

`build/sprite-pose-runtime-final-v1/report.json` compares fresh40-source CLI
builds: actual ordinary390-frame traces and the final BMP are byte-identical.
This protects existing gameplay behavior; it does not claim that the new
literal input API is already the gameplay renderer. Full human play remains
gated. The ball/direction/C2 and all prior checkpoints were not edited.

First attempt `build/sprite-pose-tests-v1` failed before C execution because
the verifier dictionary named direction.commit87:A609 / ball.submit80:B11A
instead of the immutable capture's actual hooks87:A61E /80:B11B. The failed
script and explanation remain. Corrected hooks and complete native artifact
identity checks pass; no native files or C body changed to accommodate it.
Early builds and source snapshots are retained; final header changes only
clarify the compatibility/domain wording.

## Minimal next production adapter

The two missing literal tables total2,112 bytes. A dedicated versioned draw-
input asset can preserve existing NBPANIM1v6 and all264 pack payloads unchanged:

- AC:B6B3,2,096 bytes, SHA256
  `c541203f3fedecf112bc992f79382f2a0c9b8e70baae0280e78cb2324fc32b97`.
- 87:A98E,16 bytes, SHA256
  `38a9c6ace59fd82da80e6d339813878bfb213aa6efa9242349bbdbb4443ca42e`.
  Words0593,FFFF,0591,0592,0593,FFFF,0591,0592.

84:C36E is already in packed bank84; AC:C7E3 and attachment arrays already
exist in NBPANIM1. `build/sprite-pose-source-final-v2/inventory.json` binds the
ROM/listings/table bytes. These files are evidence, not a changed asset pack.
Root owns the final resource ID/format decision and pack integration.

Before wiring the new renderer, own the complete AB48→AC22 mirror publication
at its animation cadence and the separate A5FB–A609 head-bit update. Current
`cpu_advance_actor_animation` publishes channel/resources without the mirror
word, and `cpu_store_shot_action` commits resolved mirror only for mode17;
feeding today's actor_status_raw_28 blindly is unsafe. Then resolve actual51,
D8 and DA, retain `movement_direction` as +4E/C0 versus `direction` as+52/C2,
and supply the actual caller attributes/origins. Do not set51 to zero, derive
body flags from head direction, or change ball attachments to follow a wrong
body pose. A renderer accepting the typed submissions must retain source order
and the existing reversal from queue priority to host painting. Full AF1E
ball interleave and canonical graphics queue remain independent D1 work.

## Repeatable checks

From this worktree, use fresh output directory names:

```powershell
python tools/build_sprite_pose_probe.py --output build/review-pose
python tools/build_sprite_pose_probe.py --baseline --output build/review-pose-old
python tools/test_sprite_pose.py --exe build/review-pose/sprite_pose_probe.exe --baseline build/review-pose-old/sprite_pose_probe.exe --pack ../completion-owner/build/hud-integration-v1/nba95_assets.pak --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --capture ../ball-pass-alignment-20260831/build/ball-native-v1 --output build/review-pose-tests
python tools/build_sprite_pose_guards.py --output build/review-pose-guards
build/review-pose-guards/sprite_pose_guard_probe.exe ../completion-owner/build/hud-integration-v1/nba95_assets.pak
python tools/test_sprite_pose_protocol.py --exe build/review-pose/sprite_pose_probe.exe --pack ../completion-owner/build/hud-integration-v1/nba95_assets.pak --positive build/review-pose-tests --output build/review-pose-protocol
python tools/build_gameplay_hud_probe.py --kind cli --output build/review-pose-cli
python tools/verify_sprite_pose_runtime.py --exe build/review-pose-cli/hud_cli.exe --pack ../completion-owner/build/hud-integration-v1/nba95_assets.pak --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --output build/review-pose-runtime
```
