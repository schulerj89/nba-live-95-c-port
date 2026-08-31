# Continuing-period entry integration

The independently accepted DCA6-to-DD97 prefix is copied unchanged from the
controller freeze `build/period-entry-prefix/freeze-v1.json`, SHA256
`a96b329445557aaa0e4d6976e97a14b9f518d0cbd55ccaaed4ff36893b3e3ea8`.
All source, dependency, external, object and capture identities were checked
before the copy. The independent acceptance report is retained alongside this
note as `completion-period-entry-prefix-independent-audit.md`.

A fresh root `/W4 /WX` build and replay in
`build/period-entry-prefix-integration-v1` pass all four captured cases at
three boundaries, comparing 786,432 complete WRAM words. Ninety controlled
source cases compare 14,310 typed words, and all 56 integrity mutations are
rejected. No borrowed objects or native after-state inputs are used.

The source preserves the original continuing-period carry, including ready
and dead-coordinate words and animation queue contents. Source comments
identify that behavior; it is not replaced with a generic inbound reset.
Regulation carries the selected clock, while overtime reads the original
four-entry clock table. Only period two flips the team anchors.

```powershell
./tools/build_period_entry_prefix_probe.ps1
python tools/verify_period_entry_prefix.py --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --probe build/period-entry-prefix/period_entry_prefix_probe.exe --output build/period-entry-prefix-replay-new
python tools/test_period_entry_prefix.py --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --probe build/period-entry-prefix/period_entry_prefix_probe.exe --output build/period-entry-prefix-tests-new
```

This is a standalone data component, absent from the game source manifest.
The caller must provide the already incremented period and owned carried
state. Production restart wiring, the preceding scheduler, CPU timing and
whole-game parity are not accepted by this integration.
