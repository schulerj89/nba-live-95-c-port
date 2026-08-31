# Native boundary metadata binding

Revision3 is verifier-only. It preserves the original C/native sources and both
earlier freezes. Independent QA found that revision2 accepted three malformed
native boundary records: the CPU80BC observer PC, resident0380 observer PC and
post-F1 SPC tick count could each be changed without rejection. Those failed
results remain retained separately from passing source replay.

The new reader binds all five named boundaries to their fixed source hook PCs,
actual instruction rows and raw snapshot cycle/master metadata. It validates
the declared24-bit CPU and16-bit SPC PC widths. The capture's last-observed PC
is checked against the corresponding instruction stream; it is deliberately
not confused with a later internal PC during DMA or a split SPC instruction.
For example, CPU80BC observes the last SPC instruction FFF9 while the emulator's
internal SPC PC has advanced to FFFA. That valid distinction is preserved.

The independent three-corruption tool is reused unchanged, alongside the
earlier independent nine cases,21 original local cases and12 additional profile
cases. Each suite also runs an accepted baseline. Fresh C traces retain the
same SHA256245371d44b6bd3ed5954349b95278530b6d14c0286833232c88ba3a1b5a7191c.
No source or capture was altered to fit the reader. The new verifier file and
the unchanged revision2 profile guard are both pinned in the report.

Run the same commands documented in `bootstrap-verifier-v2.md`, substituting
the three `*_v3.py` verifier/test files and new output directories. The new
composite freeze contains all660 revision2 identities unchanged plus the
revision3 files, fresh reports and an unchanged copy of the independent boundary
tool. Independent acceptance remains pending. Source scope still stops at
CPU80BC; the separate DMA child under development is not part of this packet.
