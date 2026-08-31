# Accepted bootstrap prefix through the pre-NMI boundary

This integration imports the independently accepted, continuous normal
power-on prefix through CPU `80:8145`, immediately before `STA $4200` enables
NMI and automatic controller reads. It combines the first-fill component, the
reset/table continuation, and the additive boundary-verifier repairs. It does
not add any of these sources to `nba95_sources.txt`, place them in `NbaGame`, or
claim a playable boot route.

The imported files are byte-identical to the accepted source packet. The
first-fill freeze is `bootstrap-first-fill-freeze-v1.json`, SHA-256
`1b7d3d7776a03e0fe65ebe6a78b97cbc8cba826d2ec44134c597efa5fec83cd2`.
The reset/table freeze is `bootstrap-reset-tables-freeze-v1.json`, SHA-256
`b0d23488b4baed2303abd35b4880834c5cd9a01c62937c36c3603916266b90b2`.
The additive verifier freeze is `bootstrap-boundary-verifier-freeze-v2.json`,
SHA-256
`ac46c46f35d1a4e6573151292a006642a680b47a96c08c7b868569de6090d789`.
The independent source/table audit has SHA-256
`b2351cc343c7404a3db1c6e7bb9f3963b333d644473c45c3ec7a599330d49615`;
the boundary addendum audit has SHA-256
`97563a94a22af5a23ee0bda7806012e193bc266647c02941d4a43c56105d409c`.

Fresh root-side verification used the original ROM, the accepted native
capture, and a newly built `/W4 /WX` probe. The v2 verifier passes with 58,765
CPU instruction states, 27,348 CPU data accesses, 131,072 DMA accesses, 20,650
SPC instruction states, 10,394 SPC/IO accesses, seven complete WRAM boundaries,
and 22 final typed fields. The independent boundary suite accepts its baseline
and rejects terminal-register and stdout mutations. The contract executable
passes 2,965 assertions and 10,802 explicitly checked bytes. Build and run
receipts are retained under `build/bootstrap-prefix-integration-v1`.

All 38 imported source files match the integration copy receipt. The production
manifest has the same Git blob as base `d0aa808`:
`f1b60adb7e89173abfda44b6bce3b8564fe66ce3`. The initial missing Python
dependency, the first receipt-folder collision, and the stale historical
manifest-hash assertion are retained in the ignored completion receipt. They
occurred before or after the substantive checks and are not recorded as passes.

The accepted boundary has no NMI service, controller sampling, OAM/DMA
publication, complete DSP state, `03DB` resolution, scene transition, or
gameplay integration. The next production boundary must carry the same WRAM,
CPU, SPC and clock owners across the real `4200=81` write and interrupt path.
No seeded controller state, canned DSP read, elapsed-time lookup or reset of
already consumed clock subcycles is permitted.
