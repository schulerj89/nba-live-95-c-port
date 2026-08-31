# CPU image and inbound target oracle attribution

This checkpoint updates five C image anchors and three conditions in the
Python inbound-target oracle. It does not claim a full CPU test pass. With
these changes the test's focused inbound section reaches frame49412 and still
rejects the port's incomplete period restart. That failure is retained.

The former Python helper grouped layout1 with layout4. Original `$85:C39C`
instead sends layout1 through the edge constructor at `$C50B`; layout4 alone
takes `$C450`. The production correction already has independent native and
source acceptance. Replaying its nine frozen native cases against the Python
helper reproduces five former layout1 failures and nine corrected matches.
The layout4 `(404,-224,0)` result remains unchanged. One case is naturally
entered; the other eight are declared controlled native input cases. These
checks do not establish an entire original-game trajectory.

The image controls use five source/executable cohorts at frames600,1300,3480,
6932 and6954. The primary executable and the matched old-tipoff source control
reproduce all five previous RGB arrays exactly. The matched canonical-tipoff
control and the later pre-layout source control reproduce the same intermediate
arrays. The accepted layout1 source correction produces the five current
arrays. Existing tipoff and closure freezes pin the source/object differences
for those controls. All25 renders were rechecked; five fresh renders from the
actor-execution diagnostic revision reproduce the current arrays byte-for-byte.

The five current frames were visually inspected. Court and player rendering
remain intact. The static WEST/ORLANDO scoreboard remains an unaccepted port
presentation gap; it is not labeled an original-game bug or accepted live HUD.
These images lock inspected C output, not pixel parity with original gameplay.

An AST comparison requires that the only executable Python changes since the
accepted actor-trace checkpoint are five RGB values and the three layout branch
conditions. No gameplay assertions, frame counts or tolerances are removed.
The exact shot section still passes77 starts and76 releases; the inbound
section's new49412 failure remains visible in `sections-v2/results.json`.

Evidence is under `build/cpu-image-attribution-v1` and
`build/cpu-oracle-attribution-v1`. Independent review is pending. Period
formation, native scheduler timing, human enablement and whole-game completion
remain separate work.
