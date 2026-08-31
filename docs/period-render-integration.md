# Standalone period collision and draw sort

The unchanged reviewed `period_render_tail` component implements the data
effects of $86:E1F7-$E207: collision X ordering, $80:FBFF draw-depth ordering
and the $084A/$084C counter increment. FBFF is not a DMA routine. The code
preserves wrapped signed comparisons, arithmetic shifts, the original shell
gaps, equal-key behavior and counter carry. It requires canonical object
permutations, consistent typed X values and a zero leading collision sentinel.

The final independently accepted freeze is
`build/period-render-tail-freeze-v2.json`, SHA256
`b9362eab95705e0b0c4fcab5a8f6ce85b846de9f32a28a1dfc90dcdc79e90e96`,
with 2100 identities including all 2082 original identities unchanged.
The acceptance report is `completion-period-render-tail-v2-acceptance.md`,
SHA256 `c23abd34dae76d6c839d75746f142c94e08fa10096c4c1f36bd4b1c24f8117f2`.
The original verifier rejection remains documented separately.

Independent source review passes four native cases / 12,496 bytes plus 384
controlled original-ROM cases / 1,199,616 bytes. It also checks equal-depth
ordering, unrelated-memory preservation, invalid typed states and full counter
wrap. The verifier repair requires the complete native hook sequence and its
unique consecutive tail pair. All nine independent corrupted routes reject
before C runs; old C reports never supply the replay commands.

Fresh root integration in `build/period-render-integration-v2` compiles all
dependencies and the new sources with `/W4 /WX`, without old objects. The
four native comparisons, eight output-protocol checks and nine raw-domain
refusals pass again. Native cases0/1 cross one video frame during this tail;
cases2/3 do not. These observations remain recorded. The component has no
CPU, DP, stack, interrupt, video/DMA or timing-parity claim.

```powershell
./tools/build_period_render_probe.ps1 -OutputDirectory build/period-render-new
python tools/verify_period_render_tail.py `
  --probes build/period-render-new `
  --captures build/period-restart-attribution-v1 `
  --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' `
  --output build/period-render-new-verification
```

This standalone component is absent from the game source manifest. NbaTipoff
wiring, the complete period restart and whole-game acceptance remain separate.
Original ROM and diagnostic binaries stay local.
