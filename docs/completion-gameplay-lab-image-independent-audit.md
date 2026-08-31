# Independent Gameplay Lab image attribution

PASS for the single C-only image baseline refresh in owner freeze
`build/gameplay-lab-image-freeze-v1.json`, SHA256
`2c834b59d475948d2cb917d9e3d4e2ec5c113129ac1f2d4e2672010281438d1f`.
This is not a native image, whole-game trajectory, or human-control acceptance.

All 28 frozen identities and the four recorded executable identities were
independently rehashed. The old and new `test_gameplay_debugger.py` ASTs differ
only in the `EXPECTED_LAB_RGB` string; comments are not AST nodes. Every other
assertion remains unchanged. The original failure and old image remain retained.

The auditor reran frame170, Gameplay Lab page2/actor7, using the two privately
built 52c2899 CLI programs from the accepted tipoff-image audit. Their matched
37-source builds differ only in the original versus canonical tipoff source;
both use the same candidate asset pack, SHA256
`951f82331c4bb6ce8f381da519ee8bfdf517bf8c13f2cd6f20cfa9c34d5ed4df`.
The entire RGB payload of each new capture matches its corresponding frozen
owner capture, not merely a selected sample or region:

| Source control | RGB SHA256 |
| --- | --- |
| Old tipoff | `d6e5e07757e57bcaa961ac650283d829f4d157a8fae75e26a73380ac71072015` |
| Canonical tipoff | `d52fd6308d82e0a60ecfcdf86648dd331c40da5c1c2e20b37c67225c0cb1dc6f` |

The executable hashes are respectively
`e02d51d3b12320836e901b534c20dbb0394bed9571cc4135a210d920b0d74fd6`
and `9bc4dd2f328cd003d563fbed356fda87a1538a3d68d3ec1647496968d88e9c16`.
This controlled result attributes the changed C image to the previously reviewed
team/rank initialization. It does not authorize unrelated rendering changes.

The complete frozen debugger test also passed independently against the
hash-verified owner `nba95_regression_checkpoint_v15.exe`. This retains its
telemetry, CPU-only control, pause/step, comparator positive and negative cases,
split-pass coalescing, and source/input marker assertions. Evidence is under
auditor `build/gameplay-lab-image-audit-v1`: `report.json`, `full-test.log`, and
the two freshly generated BMP/PNG pairs. Earlier matched-source provenance is
in `build/tipoff-image-audit-v1` and `build/gameplay85-audit-v1`.

Both images were visually inspected. The selected actor/page markers remain;
player appearance/geometry and rank-related debugger text change. The stale
WEST/ORLANDO score/clock background is still an unresolved port HUD gap. It is
not accepted as native parity or labeled an original-game bug. No original ROM,
asset pack, raw native fixture, or production source was changed by this audit.
