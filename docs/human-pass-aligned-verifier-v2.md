# Aligned checkpoint diagnostic protocol repair

This three-file repair changes verification only. The aligned11 and action11
source modules, probes, native captures, executables, objects and original
freezes are unchanged. There is no production source-manifest or human-routing
change.

The independent auditor found the same stderr gap as in the action verifier:
extra error output, a missing loader line, and a forged different-pack line
were accepted when C stdout matched. The original rejection report and the
independently authored adapted ten-case tool are preserved under
`build/human-pass-aligned-verifier-v2`. The generic tool's earlier failed attempt
remains in the auditor worktree; the adapted tool selects a row containing
`dp_words` rather than assuming the first lane-result row has that field.

`tools/verify_human_pass_aligned_v2.py` requires exactly the loader's one success
line containing the actual asset argument, actual file size and 263 assets from
the already SHA-pinned immutable pack. It rejects missing, extra or altered
stderr regardless of stdout values. The process code must be an integer zero.
Native raw/metadata/type/clock/source checks and complete typed C-result checks
retain their original scope.

Both original captures still pass: 20 AD0E gates, 17 choices, 17 installs,
17 upper children and three F473 calls compare 129,935 values. Both C stdout
and stderr remain byte-identical to their frozen original final outputs on
each route. No native capture or C result was refreshed to repair this gap.

Seventy expanded mutations reject. Existing stdout mutations now carry the
valid baseline stderr, so unrelated diagnostic failure cannot substitute for
testing the intended output guard. The unchanged independently authored
ten-case protocol tool also rejects all ten when run by the implementer. This
does not claim independent acceptance of v2.

Verifier SHA-256:
`0a556a322a63095bad767b5a0906bfdf7b892754faaa52360eeca1317fc4c733`.
Expanded test SHA-256:
`5e6987d4f2cdd863347cf0ee318102c77200a700714f21ea5155529b91982694`.
Unchanged probe SHA-256:
`d02f567ca7b9b75868d441f353e04d19ade7e311e5efab7071fd47e8c2b94db6`.

The original aligned scope limits remain: all three F473 native results are
AA=1; no clear/endpoint/extreme native claim; catch, pose/attachment, commit and
whole initializer continuation are not accepted by that checkpoint. Its field
named `options_07f6` contains the actual raw `$07F6` PRNG state, not Options UI
configuration. The operation and raw adapter are unchanged and faithful; a
future explicit API revision should rename it before production integration.

The patch and `freeze-v1.json` under `build/human-pass-aligned-verifier-v2`
attest these three new files, all test outputs, the original counterexamples,
and all nine earlier checkpoint identities, including action verifier v2.
