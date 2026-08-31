# Original culling branch correction

Candidate based on facd818. Only the wired `nba_court_actor_visible` helper in
`src/nba_court_presentation.c` changes production behavior. No camera, draw
order, OAM, scratch ownership or controller code changes here. This is one
bounded D1 repair, not completion of D1 or native graphics parity.

The original87:A419..A42D routes both depth>=288 and depth<-20 through the
depth-minus-Z fallback. The old port discarded the lower branch and used host
signed comparisons. The corrected helper preserves wrapped16-bit CMP N tests,
including the original depth=-21,Z=0 survival. Its comment identifies that
source behavior without labeling every unusual branch an original game bug.
Controlled players keep their original inner-rectangle route; the helper still
does not implement their separate off-screen indicator.

## Source and C verification

`tools/test_court_culling_source.py` executes only the bounded original culling
instructions read from canonical ROM2115c39f... and compares the helper Boolean.
It starts after human/CPU selection and stops before indicator/sentinel effects.
Expected results are generated from ROM bytes before invoking the C probe.
The test does not establish reachability, registers, writes, OAM or cycle parity.
It enumerates all65536 X values for both controller routes; all depths for six
CPU heights and the human route; all heights for six boundary/wrapped depths.

-983,040 controlled cases match, with three malformed binary-input rejections.
-A fresh unchanged facd818-body probe fails196,508 of the same cases.
-Both probes compile all40translation units with MSVC /W4 /WX and bind source
  and header hashes. The baseline body is obtained with git show into its own
  build directory; no working source is overwritten.
-A fresh40-source CLI completes63,800frames using the current HUD pack
  f564c29612928984002ed3f0389d317de639fff122baf61a7bc9ecaef2a6be09.
  Trace SHA256 is
  e1e7932d12cf29afac79a33c77f087ec1f417b172815c1f1a7aaeb3519a305f5.
-A field-by-field comparison against the accepted C1 trace finds22changed
  rows, solely `/actors/6/visible`, first frame54152. No other logged state
  changes. This is C regression evidence, not full machine-state equivalence.

Evidence lives in ignored `build/culling-candidate-v1`,
`culling-baseline-v1`, their `*-source-v1` result directories,
`culling-cli-v1` and `culling-gameplay-v1`. Fresh CLI SHA256:
a956287ed64c273628ed059608100888462b8d1a593d71e4b08fde994af80bf4.

## Retained failure and open work

The existing `court_runtime_probe.c` exits20 after its16,000update checks and
812isolated panorama views: its requirement that every BG/backdrop category
was observed is not satisfied. A fresh unchanged facd818 runtime with the
same pack also exits20 at that assertion. Both logs/builds are retained under
`build/culling-court-*` and `culling-baseline-court-*`; the baseline build is in
the integration worktree. No assertion was removed or expected output updated.
This attribution establishes that the failure predates this repair, not that
the fixture or renderer is correct. Its subsequent four period scenarios did
not run. The broader image-golden failure is also still open at this candidate.

Independent review and integrated checkpoint screenshots remain required.
The shared graphics queue, ordered source pass, OAM construction, overlapping
DP writes, camera-subject direction, anchor-direction branch, exceptional
period calls and actual NMI visibility remain separate D1 work.
