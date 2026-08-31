# Cold-boot and EA transition audit, 2026-08-30

Follow-up: `intro-indexed-resources.md` and `intro-text-independent-audit.md`
record the accepted bounded replacement of screenshot/handwritten graphics
and the exact303-frame EA renderer repair. The findings below describe the
retained pre-repair baseline. Full-intro timing/input/audio/handoff remains
FAIL; the phase-aligned renderer PASS does not change that verdict.

**FAIL: complete intro timing and consecutive-frame equivalence.** This is
fresh investigation evidence, separate from the accepted bounded Rules repair.
No intro production change or new expected image hash is authorized by this
report alone.

Two fresh private portable Mesen captures are retained under
`.analysis/intro-exact-20260830/capture-v1` and `capture-v2`. The second contains
1,500 synchronous RGB frames beginning at native `$80:FD9E`, immutable Lua,
original-ROM/emulator/script hashes, observed script-data directory, explicit
zero-RAM/no-frame-skipping/video settings, empty initial SRAM and verified
persisted settings. Neither script supplies controller input, modifies machine
memory nor loads a savestate. The RGB format is fixed 256x239, with the same
seven leading rows removed for all 256x224 active-frame comparisons.

The executable replay and image pairs are under
`.analysis/intro-exact-20260830/c-port-v1` and its sibling files. These captures
are evidence, not production art. The existing production indexed E/A/SPORTS
assets remain unchanged in main. A later direct extractor audit found that
these indexed letters are only part of the production intro asset path.

**Production provenance FAIL:** `tools/extract_assets.py` builds asset1 from
handwritten `license_rows`, asset2
(legal notice) from `legal.png`, assets3–6 and72 from the captured EA stage
PNGs, and asset73 from eleven `ea_motion_*.png` images. Their union also
determines the shared canvas dimensions/origin used by indexed letter assets.
These are emulator-rendered production images, contrary to the required
ROM-asset policy and earlier documentation. Root is replacing these dependencies
with ROM-derived tile data, native object layout and palette sequencing in the
isolated `work/intro-transitions-20260830` worktree. Do not describe the inherited
pack as wholly compliant while those inputs remain.

Audio asset7 combines four decoded ROM BRR samples at hardcoded host times.
It is not a recorded song, but it also does not reproduce the original
command/sequence timing. The BRR-derived individual samples and native
sequencing requirements must be distinguished in the replacement.

## Observed defects and owners

| Area | Independent finding | Implementation/evidence boundary |
|---|---|---|
| License initialization | Native `$80:FD9E` begins a builder; the first seven captured frames remain forced blank. The license hold starts later at `$80:FE7B`, and its decrement/BPL loop includes the zero iteration. | C starts with an immediately visible license and a nominal 120-frame hold. Resource construction versus presentation timing must be translated explicitly. |
| Legal hold and input | `$80:FEE6-$FF01` is followed by a second `$80:FF03-$FF26` wait loop. The first is 181 waits; the second is 121 and tests **exactly** Start (`$1000`) during that latter interval. Fresh v2 reaches the second loop at global400 and fade-out at521. | `NBA_LEGAL_FRAMES=180` and the current scene dispatcher omit the second loop and permit an earlier input skip. The existing source comment describes only part of the routine. |
| Legal-to-EA handoff | Native legal fade-out starts at global521; EA motion entry `$82:F2EA` occurs at592. Forced blank/resource and audio initialization occupy part of this interval. | No complete C translation or independently verified portable timing contract exists for this handoff. Do not insert a guessed delay to fit the final picture. |
| EA moving frames | Applying the existing address-based motion mapping (`C=345+motion`) to fresh synchronous native motion1..302 gives 211 exact frames and 91 mismatches. The first difference is motion3; C remains black while native already shows E. | `nba_ea_intro.c` deliberately delays the matrix by one frame based on the old asynchronous screenshot assumption. More geometry/palette/ownership differences may remain after correcting that cause. |
| EA-to-title | Fresh v2 reaches native title entry `$80:E1B1` at global1146, EA motion554. | Entire hold/audio/fade/resource ordering remains to be compared; static logo endpoint equality cannot establish this handoff. |

The existing Ghidra dump `.analysis/ea_intro_bank80_helpers.txt` contains the
complete `$80:FD9E-$FF3E` flow, including both legal loops; its corresponding
source is `tools/ghidra/DumpBank80IntroHelpers.java`. The EA helper dump and
`DumpEaIntro.java` cover the Mode7 and palette routines. Fresh narrow recomp
and original-byte checks are required before implementing this follow-up;
these old dumps are useful navigation, not independent execution proof.

## Why the old gate does not prove parity

`tools/test_intro_sequence.py` locks selected C image hashes. Its optional
native PNG checks accept mean absolute errors up to 2.7 RGB levels and silently
skip the check when the local PNG is missing. The comment attributes this to
analog color conversion, but the inspected Mesen path produces digital RGB
from its PPU frame, and the fresh controlled brightness experiment reproduces
its five-bit conversion exactly. That explanation does not justify a timing,
geometry or palette tolerance.

The old capture uses `takeScreenshot`, which can return a previously presented
image. The fresh test must instead retain synchronous consecutive frames,
native routine/raster state and fixed input/phase boundaries. Preserve the C
regressions as such while adding an independent exact gate. Do not replace the
91 failed frames with a larger tolerance or regenerate their expectations
from the C implementation.
