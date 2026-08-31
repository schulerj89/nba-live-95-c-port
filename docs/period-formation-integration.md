# Typed period formation integration

The unchanged DD97-to-E207 component is copied from scheduler freeze
`.analysis/period-formation-freeze-v1.json`, SHA256
`8265eb8e8e71e6c59186b2a0526d9ea75c02fe96a80ccf18bed5507dde41e244`.
Root rechecked all1,594 frozen identities. Independent final acceptance is
retained in `completion-period-formation-independent-audit.md`, SHA256
`943e09ee450abd99ff57e1e711d1c0d107dd6e25763b1574481ab9aa54f99d01`.

The public component uses named C fields and real child functions. It has no
raw-WRAM input, after-state callback, child-return file or duration model.
Channel/pose/controller/assignment aliases share canonical owners. Original
fractions, queues, pair indices and carried ready/dead-ball words remain.
Unsupported appearance domains, missing carried nearest-pointer state and
unresolved role/assignment children stop explicitly.

Fresh root verification in `build/period-formation-integration-v1` compiles
19sources with `/W4 /WX`, using no old objects. All125native checkpoints /
128,500gameplay values pass. All12binary-input/trace/diagnostic artifacts are
byte-identical to the accepted baseline. The47controlled composition cases
pass1,600original-ROM coordinate comparisons and8,651role field comparisons;
all8entry refusals and32protocol corruptions pass. The first source-test
attempt refused because its local alias-map dependency had not been copied;
that output directory is retained, and the completed run is `source-v2`.

Independent checks additionally verify83,562C ownership assertions,
1029nonoverlapping fields,211role aliases and30malformed protocol cases.
123original-ROM cases with poisoned omitted CPU temporaries preserve25,953
owned outputs/stops. These checks support the explicit scratch exclusions;
they do not claim CPU register or interrupt parity. DP9A alone is excluded
from the native gameplay-value comparison and remains documented as residue.

`prepare_period_formation_dependencies.py` recreates the original30source/
header snapshots from this checkout using pinned hashes. Historical absolute
paths in the retained manifest are provenance only and are never opened by
the preparer. It also restores the unchanged211-field alias map for the source
tests. Changed or extraneous snapshots are refused, never overwritten. The
accepted builder/verifier themselves remain byte-identical.

```powershell
python tools/prepare_period_formation_dependencies.py
./tools/build_period_formation_probe.ps1 -OutputDirectory build/period-formation-new
```

Use the verifier and test commands in `period-formation-typed-composition.md`
with the fresh executable and fresh report directories. Only the accepted
v2parent is in the root source tree; historical v1 files included transitively
in the review packet remain in ignored preparation evidence and are not
integrated.

The game source manifest and NbaTipoff are unchanged. The existing basket X
owner is `court_presentation.basket_x_3fef`; carry it across the earlier anchor
flip rather than recomputing it. Runtime draw-order ownership, the entry-prefix
adapter, ordinary period scheduling and full-game acceptance remain separate.
See `period-tipoff-state-mapping.md` for the remaining ownership work.
