# Pass body pose and draw direction integration

This checkpoint fixes the reported visual separation between the passer's hands
and the ball. The port was rebuilding torso/leg resources from the independently
selected head direction. Original `87:A4E1/A517` preserves actor +2A/+2C in D6/D4
before that direction selection; ordinary AD92 and subject AF1E consume those
body resources unchanged. The draw and telemetry callers now use the published
body pose. Physics, ball attachments, player coordinates and animation producers
are unchanged. The source contract and the former port error are commented in C.

The separately accepted direction correction is included: the mode14 ball-facing
branch uses the camera subject at $0940, upper20/21 uses anchor >>1 rather than
the old incorrect stale-AE explanation, and the draw quantizer uses original
F02D rather than F34F's different equality behavior. Contact-facing correction
remains a separate candidate; no global direction helper is replaced here.

## Review and exact composition

Direction freeze SHA256
`000af9b1681d0f8a6bdd28af4858b9e93380cd71ea20cea21776ad08dceb03e4`
contains 343 identities. Independent final audit SHA256
`56cc5ec6ce910b95b0bc9b75b4d6e1d8e3c0da0e68e6fcaf776b9f83c6debc98`
accepts its bounded direction source/caller and visual checks. Ball freeze
`aef5ac1cfa04ac91dfc7921e6c51f4aac59493b22bc96c9fb7b88290f85a61b4`
contains 1,319 identities; independent review reports matching source/native
words, fresh actual C journeys and inspected pass images.
The ball audit SHA256 is
`4af0813247dba2e5d195c58c8d03c785fdc951bf22f2b763783f7ff4fad09e13`.

Both freezes were rehashed before composition. The combined tipoff source is
exactly the frozen ball candidate with only `actor_draw_direction` replaced by
the accepted direction version. AI and every new tool/document match their
respective candidate bytes. Ignored `build/graphics-integration-v1/composition.json`
records the composition. Frozen candidate documents retain their historical
pending-review descriptions; this integration receipt supersedes that status.
Independent combined audit SHA256
`7d7e149c26564d12116523517c97842f9f84f01646db4aa5baae1d147e31a6ad`
also verifies the exact composition, all 40 compiled sources/headers, complete
63,800-row comparison, and nine attested combined pass-stage images.

An initial pre-copy assertion rejected working CRLF bytes versus LF Git blobs;
no mutation occurred. A normalized comparison proved those baselines differed
only in line endings, and the original working bytes were retained. `.gitattributes`
preserves accepted bytes; the large raw AI diff is predominantly that encoding
preservation. The logical AI change is 33 lines (27 added, six removed).

## Fresh combined checks

All evidence below is retained in ignored `build/graphics-integration-v1`.

| Check | Result |
|---|---|
| Fresh production CLI | 40 sources, MSVC /W4 /WX; SHA256 `a3cd0a69649e17f939228d6a25cf15acddf7a12efd47afccc60db5596a053d3c` |
| Actual combined draw caller against original ROM instructions | 240,192 cases, zero mismatches; 11 malformed inputs refused, two positive controls |
| Actual public actor loop and body rendering | 63,800 frames, 628,226 body-pair checks, zero mismatches, all eight pass directions |
| Rendering purity | 116 real render calls across frames275..390 leave NbaTipoff and NbaSession unchanged |
| Full JSON trace versus accepted culling baseline | 63,800 complete rows; 9,530 changed rows, only 130 enumerated draw/appearance paths; zero unexpected changes |
| Existing C1 interruption/recovery guard | One interruption and one recovery; zero unfinished episodes or failures |
| Original draw-resource/projection evidence | 43 complete draw groups, 19 ball submissions, 592 owned words match; all eight pass directions |

The full JSON comparison checks every serialized field; ball/player positions,
fractions, velocities, possession, modes, RNG, scores and clocks are unchanged.
Only actor head/draw-direction/body-resource and derived appearance fields may
differ. Trace SHA256 is
`3e283f7509507e35c44e90465a0dfdc6118e4185430532b608725ddd46e1097f`;
baseline SHA256 is
`e1e7932d12cf29afac79a33c77f087ec1f417b172815c1f1a7aaeb3519a305f5`.
This is stronger than the candidate's per-frame diagnostic FNV checksums but
still excludes unlogged machine state and does not establish native game parity.

The unchanged frozen ball checker runs against the combined source and retains
its strict output/domain checks. Its old candidate-versus-baseline checksum
equality is not reused for the combined change, since corrected head direction
intentionally changes that checksum. The separate full JSON comparison above
explicitly checks the allowed combined differences instead.

The native verifier is closed to its exact immutable capture, with 271 raw hook
records. Eleven entry-only records are source-culled, and three NMI crossings
remain explicit. Matching 592 words is not full CPU/DP/OAM/interrupt replay or
general verifier mutation certification. No native memory or capture fixture
was injected or changed.

## Visual scope and remaining gates

Fresh combined pass images cover every frame275..390 in `pass-sequence`, including
before, windup, raise, last attachment, release, flight, receiver and catch. At
frame306 the passer now draws published body332/1168; the ball remains at world
(-119,99,47), screen(201,105). Root inspected the actual combined windup image.
The independent ball-only review inspected all 18 before/after named views;
304 and350 are pixel-identical continuation frames, not claimed improvements.
Changed body pixels during flight do not imply that ball flight changed.

The existing full CPU regression completed with exit1 after its gameplay/state/
pass/receiver/motion checks and the accepted600/1300 image anchors. It stops at
the same pre-existing3480 RGB mismatch,
`805bc5705a4faa6946bf484fe670614767a0f34a2465e490343c5cbff79e6662`.
The later image and final source-marker checks therefore do not run. No expected
hash or behavioral assertion was changed. This is not a whole-suite pass.

Additional actual before/after captures at all five existing image anchors are
retained in `image-anchors`:600/1300/3480/6932 are pixel-identical;6954 changes957
pixels. Root inspected the6954 pair as a changed body-pose view. This separate
capture is not an acceptance/migration of the old6954 expected image or a pass of
the later blocked regression checks. The exact-commit screenshot publication
receipt follows separately; no ROM, pack, executable or image enters Git.

Independent head/body mirror flags, jersey/head ordering, ball interleave,
persistent draw ordering and the canonical graphics queue remain D1 work. A
separate compositor ticket now maps literal D4/D6/D8/DA/47/51/C0 inputs to source
piece order; it must not invent missing input owners. The queue dependency is
documented in `completion-graphics-queue-consultation-20260831.md`. Full timing,
receiver/human integration, complete modes and save handling remain unfinished.
The main branch and desktop executable are not promoted by this checkpoint.

## Exact committed screenshot receipt

The screenshot agent rebuilt clean commit `744809a9d2ad548f83dedd9dffabce09e3cbda11`
from all40 production sources and112 bound identities, then directly inspected
33 images:13 standard views and20 before/after supplemental views. All195 local
links resolve. Primary ignored `.analysis/progress-screenshots/latest/index.html`
now points to that checkpoint; every previous dated run remains intact.

| Artifact | Identity |
|---|---|
| Standard run | `20260831T202823.851515Z` |
| Standard manifest SHA256 | `e5dcdb90f6aa2a0b1ad234e304952c28e033e813d16a225804b0e7241cd3943b` |
| Pass supplement | `20260831T203004.002173Z-pass-supplement` |
| Supplement manifest SHA256 | `702f2d1ec57f91f25e279d7381449245b705936ed8b0fceb82adbb26822f397d` |
| Fresh screenshot executable SHA256 | `c22556c30347bd1b69eafd17323c926ab4ce8bd857cdaff873d28095260beacf` |
| Independent receipt SHA256 | `8c2d462e4d4fb5190736ae78bb058e6047e7a65c155470a7083faa9515021d36` |

All13 standard views are pixel-identical to the prior culling checkpoint. The
combined supplemental differences at306/308/312/318/320/332/346 are respectively
506/630/641/688/699/225/594 pixels;304/350 are unchanged and the later6954 pair
changes957 pixels. These combined counts differ from the earlier ball-only
candidate because the reviewed head-direction correction is also present.
All nine pass ball projections remain identical, and committed images match the
reviewed combined candidate. PNG previews are lossless copies of the attested
BMP pixels. Captures use documented fresh scene-entry processes, not a
continuous boot-to-match or measured display-FPS test. No image, executable,
asset pack or ROM is committed.
