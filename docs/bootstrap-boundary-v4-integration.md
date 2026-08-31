# Base bootstrap boundary verifier repair

The accepted reset/upload/F1 source still stops at CPU80BC. Its comparison entry
point is now `tools/verify_bootstrap_v4.py`, with shared helper
`tools/bootstrap_boundary_protocol_v2.py`. This adds validation without changing
C, clocks, captures, runtime behavior or the production source manifest.

Independent review found eleven reproducible false acceptances in the earlier
v3 reader: contradictory terminal CPU registers/types and extra, missing,
duplicated or altered stdout boundary diagnostics could pass. The actual C and
native values still matched; the reader did not establish the claimed integrity
of every supplied field. Old readers, failed examples and frozen evidence remain
unchanged as historical artifacts. They are not the current acceptance route.

The new reader binds every CPU-hook row, including the terminal unexecuted row,
to its raw snapshot with exact register types. DP=False cannot stand in for zero.
It also requires exactly the canonical loader line, two boundary lines and final
summary. Boundary text is reconstructed from checked C events/state and upload
stores, not copied from native afterstate or fixed timestamps. Existing source,
bus, timing, capture isolation and output-state checks remain in place.

## Accepted delivery and root checks

Root copied only four byte-identical files from the independently accepted
additive packet: the shared helper, base v4 reader, base boundary test and
regression driver. The source packet includes separate fill/table readers, but
those additions and their C implementations are not installed by this checkpoint.

Freeze SHA256 `ac46c46f35d1a4e6573151292a006642a680b47a96c08c7b868569de6090d789`
has5,772 direct identities, all rehashed before root checks. Independent audit
SHA256 `97563a94a22af5a23ee0bda7806012e193bc266647c02941d4a43c56105d409c`
accepts the additive guards and confirms unchanged execution artifacts; its
tables guard acceptance does not establish table-source acceptance.

Fresh root eight-source MSVC /W4 /WX probe SHA256:
`425ce05a1408f87ed83adb6d554301ca0605ea21260b209ebf6e9a2d15c60d69`.
The root v4 replay passes28,405 CPU instruction states,16,259 CPU data accesses,
9,616 SPC instruction states,6,594 SPC write/I/O accesses,84 scalars, full WRAM
and the checked ARAM boundaries. Trace SHA256 remains
`245371d44b6bd3ed5954349b95278530b6d14c0286833232c88ba3a1b5a7191c`.

Eleven new boundary corruptions,21 existing v3 protocol corruptions and12 v3
profile corruptions reject, and each suite's baseline passes. These are related
guard tests, not44 independent source-correctness proofs. All seven execution
files are byte-identical to the prior accepted root replay: events, final WRAM,
final ARAM, resident/F1 ARAM, and resident/F1 scalar state files.

Evidence remains in ignored `build/bootstrap-boundary-root-v4`. The first runner
used an older protocol suite and then an incorrect nonexistent profile filename;
that setup failure is retained. Corrected explicit v3 suite paths were rerun
against the copied root reader. No C, fixture, expected result or test assertion
changed to obtain a pass.

Run the existing `build_bootstrap_probe.ps1` into a fresh directory, then the v4
reader with explicit `--native`, `--rom`, `--exe`, `--decoder-root` and `--output`.
The four new files preserve the exact audited bytes. Source and profile limits
remain unchanged: no production bootstrap wiring, full03DB/DSP/NMI service,
Rules timing, or equivalence between lazy native SPC callback master times and
source oscillator deadlines. The existing pass-render screenshot gallery remains
current because this checkpoint changes no rendering source or behavior.
