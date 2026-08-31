# Action and aligned-pass verifier v2 independent acceptance

Verdict: **PASS for both bounded composite checkpoints**: unchanged frozen action/aligned C and probes, paired with their separate v2 verification tools. No whole initializer, production enabling, normal human journey or native gameplay trajectory is accepted.

The original source contracts, coverage and protocol failures remain recorded in `completion-human-pass-action-independent-audit.md` and `completion-human-pass-aligned-independent-audit.md`; all original evidence remains intact.

| Component | New controller freeze SHA256 | New verifier SHA256 | Referenced identities rechecked |
| --- | --- | --- | --- |
| Action | `b7766b370862088a6ac21691e882d21838d04b0674550c04dd1798386b4138bf` | `b03c72c2e2059f5e0ada621ead4a8ca3cb4cb8578d0b72b7e22a99369d31539a` | 96 |
| Aligned | `7871708f317a2eebfc781c01ccc90f8859d7126d4e90b73811bb21f97953f938` | `0a556a322a63095bad767b5a0906bfdf7b892754faaa52360eeca1317fc4c733` | 108 |

Each freeze adds only a verifier, tests and documentation. Independent diffs show the verification behavior changes only at the subprocess result: exit status must be an actual integer zero, and stderr must equal the single original loader success line using the actual asset argument, its size and 263 asset count. The verifier already SHA-pins that pack. Missing text, another path/count/size, duplication, extra error text and noninteger zero codes cannot pass. Native manifest/source/command/environment/settings/raw checks and C result comparisons remain unchanged.

Fresh replays use the auditor's independently compiled unchanged v1 probes, not the implementer's recorded output. Action again passes 31,266 left plus 33,003 right values (64,269 total). Aligned again passes 94,815 left plus 35,120 right (129,935 total). Both stdout and stderr are byte-for-byte identical to the auditor's original successful native replay for both routes. Expected native values and source behavior are unchanged.

The unchanged independent ten-case protocol tools reject all ten cases for each component, including all three previously accepted diagnostic corruptions. Fresh expanded local suites reject all 55 action and 70 aligned cases, including boolean/float zero process codes and altered diagnostics. Original 45/60-case results and 7-of-10 independent rejection reports remain preserved.

Copies, diffs, fresh replay logs and all mutation outputs are under auditor `build/pass-action-audit-v2` and `build/pass-aligned-audit-v2`. Their `freeze-recheck.json` files enumerate the reviewed identities. The source/native scope remains: action through AC50 decisions/B00B/upper child, and aligned through AD0E/AE10/F473/AEDD/upper child with explicit unexecuted continuation routes. The aligned `options_07f6` naming caveat is documented as raw PRNG state; no numeric ABI change was made. The 38 action and 731 aligned independent controlled guards supplement, but do not expand, natural coverage.
