# CPU role-rebuild reaction reload

The production role planner now executes the original `$85:B95C-$B9D1`
reaction reload at its three `$85:BD0D-$BE03` call sites. Eligible offensive
actors are visited first with their alternate assignments, followed by all
eligible defenders and then the eligible offensive actors again with their
base assignments. The passes stay separate because every call advances the
shared `$07F6` RNG in actor order.

Each call clears actor `+$7E` behavior flags and writes the randomized
ball-distance delay to actor `+$60`. The live-state `$82` provisional inbounder
still has its flags cleared, but preserves both its timer and the RNG. Actors in
mode 10 remain outside all three passes. The distance calculation uses the
65816's wrapped 16-bit subtraction, absolute value, comparison-N swap, wrapped
addition, and comparison-N clamp rather than widened host arithmetic.

## Source evidence

- The verified USA ROM SHA-256 is
  `2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`.
- A fresh headless Ghidra listing covers `$85:B95C-$B9D1` and
  `$85:BD0D-$BE03`. Its SHA-256 is
  `258bfb457cd155872b45d786cabb10c676337ec705263ce8a37663c7e4001727`.
- A fresh snesrecomp emission of the same ranges has SHA-256
  `eb5cbd532bcba408db72ac3511144121cb2bb537b5c2619f9eac2abecb90013a`.
  Both references identify calls at `$85:BD39`, `$85:BD96`, and `$85:BDF3`.
- A natural Mesen run captured 14 consecutive `$85:B95C` calls with no ROM
  patch or state injection. The vector JSONL SHA-256 is
  `232c7c7ef85ea99be35a334cf1db904666106b4af7f5a4d9fc83294e0e9d38c6`.
  All 14 reaction values and RNG exits replay through compiled C with zero
  differences, and every native exit cleared the actor flags. The first
  rebuild made 13 calls because actor 8 was in mode 10; it ended with RNG
  `$C408` and timers
  `97,120,143,128,118,7,85,92,unchanged,110`.
- A separate natural Mesen capture at the full `$85:BC07` planner entry has
  SHA-256
  `c79e1e47e377352d22afecfd0021672c640be03ba0c8112e545cffa358f990bd`.
  The production planner replay matches its exit RNG and every represented
  field, including all ten `+$60` timers and `+$7E` flags.

The generated source references and native captures are retained under
`build/cpu-reaction-20260903`. They are evidence from the named ROM and do not
claim parity for unrelated CPU decisions.

## Verification

```powershell
python tools/regenerate_cpu_reaction_reference.py --rom 'F:\Games\SNES\NBA Live 95 (USA).sfc' --output build/cpu-reaction-reference-fresh --recompiler 'C:\Users\joshs\Projects\tools\snesrecomp-source-v0.2.0-alpha\recompiler' --ghidra 'C:\Users\joshs\Downloads\ghidra_11.3_PUBLIC_20250205\ghidra_11.3_PUBLIC\support\analyzeHeadless.bat' --jdk 'C:\Program Files\Microsoft\jdk-21.0.12.8-hotspot'
.\tools\build_vector_probe.ps1 -Name reaction_core_vector_probe
python tools/verify_reaction_core_vectors.py --vectors build/cpu-reaction-20260903/native-mesen/cpu_reaction.vectors.jsonl --probe build/reaction_core_vector_probe.exe
.\tools\build_vector_probe.ps1 -Name defense_refresh_vector_probe
python tools/verify_defense_refresh_vectors.py --vectors build/cpu-reaction-20260903/native-planner/cpu_role_rebuild.vectors.jsonl --probe build/defense_refresh_vector_probe.exe
.\build.ps1
python tools/test_cpu_reaction_smoke.py --rom 'F:\Games\SNES\NBA Live 95 (USA).sfc' --output build/cpu-reaction-smoke-fresh
```

The smoke test presses nine buttons through the real setup route, reaches the
tipoff without state injection, detects the first live role rebuild, and then
replays it while capturing every frame and winning-layer header. The retained
run passed at frame 758: actors 0-8 received new nonzero timers and clear flags,
actor 9's mode-10 timer remained unchanged, and the shared RNG advanced from
`$018D` to `$EE19`. All 37 consecutive frames passed image, ordering, camera,
and layer-header checks. The report and contact sheets are in
`build/cpu-reaction-20260903/smoke-verified`.
