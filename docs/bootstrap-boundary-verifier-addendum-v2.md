# Additive CPU-hook and stdout verifier repairs

Independent review found verifier false acceptances in the first-fill packet,
not a mismatch in the actual C/native execution. The terminal native CPU
instruction row was excluded from the executed-instruction comparison and
its registers were not bound to the raw hook snapshot. In addition, the reader
validated only the last stdout JSON line. Thus forged terminal A/X/Y/PS/DB,
emulation or boolean DP, and inserted/missing/altered diagnostic lines, could
pass. The original rejected first-fill verifier and all its artifacts remain
unchanged.

The new tables reader inherited the same implementation. Read-only inspection
also identified the class in the earlier accepted80BC reader. A retained
negative-control run proves all11 adapted mutations pass that original v3
reader. That finding narrows confidence in the old reader's protocol guard;
it does not invalidate the actual unchanged source/native matching values or
extend their bounded scope.

Three additive readers now use `tools/bootstrap_boundary_protocol_v2.py`:

| Source checkpoint | Preserved reader | New reader |
|---|---|---|
|80BC reset/upload/F1 |verify_bootstrap_v3.py |verify_bootstrap_v4.py |
|80C0 first DMA fill |verify_bootstrap_fill.py |verify_bootstrap_fill_v2.py |
|8145 before NMI enable |verify_bootstrap_tables.py |verify_bootstrap_tables_v2.py |

Every CPU hook must bind exactly one matching instruction row to the raw
snapshot. The existing PC/master/cycle association remains; the new guard
additionally binds A/X/Y/SP/PS/DB/DP as exact integers and emulation as an exact
boolean. The native reader requires DP's type to be `int` before accepting0.
No callback-tracked SPC PC is confused with its internal instruction PC.

Stdout must contain exactly four lines: the canonical ROM-loader diagnostic,
resident and F1 boundary diagnostics, and the final summary JSON already
validated by the existing schema. The boundary lines are reconstructed from
the actual checked C event markers, public boundary state and preceding upload
stores. No recorded timestamp is hardcoded as an expected stdout value. Extra
errors, duplicate/missing boundaries and altered fields reject. CPU/SPC clock
reconstruction, source semantics, source stop and native artifacts are unchanged.

## Evidence and reproducibility

The auditor's exact first-fill corruption tool is retained at
`build/bootstrap-audit-boundary-v2/test_s1_fill_boundary_protocol.py`, SHA256
`ea1fb85a005ca10c2742f176b6d6f41f48020129a92761839ba9e52a6a20c8f0`.
Its baseline passes and all11 corruptions reject with the new first-fill reader.
The table and80BC adapters change only which native source-hook row is selected;
they do not claim to be independently authored tests. Their baselines also pass
and all11 corruptions reject. The original80BC negative control remains at
`build/bootstrap-boundary-v3-negative-control`; its expected nonzero test exit
records11 old false acceptances.

All247 checks pass:18 accepted baselines and229 rejected corruptions across
the three readers (including the unchanged earlier suites). This totals tests
with intentionally repeated coverage, not247 independent correctness claims.
`build/bootstrap-boundary-verifier-results-v2.json` lists every suite.

New baseline outputs are byte-identical to the previous source replays:
7 original-checkpoint files,10 first-fill files and15 table files, including all
event streams and written WRAM/ARAM/VRAM/public-state outputs. The receipts are
in `build/bootstrap-boundary-source-equality-v2.json`. There is no C-source,
executable, native-capture, source-generator, profile-clock or hardware change.

The unchanged frozen protocol suites can be reused with an explicit reader:

```powershell
python tools/test_bootstrap_guard_regressions_v2.py --suite tools/test_bootstrap_protocol_fill.py --verifier tools/verify_bootstrap_fill_v2.py --native build/native-bootstrap-fill-v1 --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --exe build/bootstrap-fill-probe-v2/bootstrap_fill_probe.exe --decoder-root C:/Users/joshs/Projects/tools/snesrecomp-source-v0.2.0-alpha/recompiler --output build/fill-protocol-v2-fresh
python build/bootstrap-audit-boundary-v2/test_s1_fill_boundary_protocol.py --verifier tools/verify_bootstrap_fill_v2.py --native build/native-bootstrap-fill-v1 --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --exe build/bootstrap-fill-probe-v2/bootstrap_fill_probe.exe --decoder-root C:/Users/joshs/Projects/tools/snesrecomp-source-v0.2.0-alpha/recompiler --output build/fill-boundary-v2-fresh
```

The regression driver prints the old suite and selected reader identities and
rebinds only the suite's verifier import. Each original mutation and assertion
is unchanged; this is an implementer rerun, not independent acceptance. The
old9-case and3-case independent tools also accept `--verifier` directly.
Fresh output directories are required. Use the source-specific probe builds
and native directories from each checkpoint document, substituting only the
reader version above.

The repaired composites still require independent acceptance. All earlier
source/profile limitations remain: no full03DB/DSP/NMI service, no source-clock
versus lazy native SPC callback-master equivalence, no production wiring, no
Rules reentry or whole-journey timing acceptance.
