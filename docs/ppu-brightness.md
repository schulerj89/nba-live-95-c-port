# Native brightness quantization

The 2026-08-30 consecutive Rules-transition comparison exposed a shared bug in
the C compositor and its separate Python snapshot renderer. Both expanded
five-bit CGRAM channels to eight bits and then multiplied by brightness/15.
Their agreement could not independently prove dimmed scanout.

The installed Mesen reports version
`2.1.1+137ae7ce3bf3f539d007e2c4ef3cb3b6c97672a1`. Its
[`SnesPpu::ApplyBrightness`](https://github.com/SourMesen/Mesen2/blob/137ae7ce3bf3f539d007e2c4ef3cb3b6c97672a1/Core/SNES/SnesPpu.cpp)
scales/quantizes each five-bit channel after color math. The
[`SnesDefaultVideoFilter`](https://github.com/SourMesen/Mesen2/blob/137ae7ce3bf3f539d007e2c4ef3cb3b6c97672a1/Core/SNES/SnesDefaultVideoFilter.cpp)
expands that RGB555 result for display. The source was fetched at the exact
installed commit, retained locally as
`.analysis/ownership-20260830/Mesen-installed-SnesPpu.cpp`, SHA-256
`5f61ed7c8da25938b494a4d42b270109dd9500ea5d155130b6b69ff774458180`.

Independent retail-ROM frame evidence is
`.analysis/transition-ownership-20260830/setup_rules_exact/open_step_552.rgb`
(synchronous 256x239 output, normal 224-line viewport at rows 7..230) versus
`c-rules-candidate-pack/frame_0249.bmp`. At brightness 7 the native red18
channel is 66, while the old C result is 69; red16 is native57, old C61.
The viewport conversion removes the emulator's normal output border; it is
not an artifact-hiding crop. The transition comparison must still examine all
224 active lines.

`nba_snes_cgram_color` now quantizes five-bit channels before expansion.
`tools/snes_ppu_oracle.py` has the independently sourced correction too; its
agreement with C remains subordinate to the recorded Mesen output. Four
literal startup witnesses distinguish mid-fade quantization, black, full
brightness and subtraction-before-brightness. No scene fade, extra wait,
blanking, asset replacement or display crop was introduced.

This is a shared color-conversion repair, not closure of all transitions.
Full-brightness outputs are unchanged. Dimmed C-only goldens must be reviewed
against native evidence before updating; mismatches are not automatically
accepted. The separate transition report records first-frame and remaining
upload/OAM/value-canvas differences.
