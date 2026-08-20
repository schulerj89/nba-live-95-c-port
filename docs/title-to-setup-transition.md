# Title-to-Game Setup transition — ROM facts


`$80:E5C7` is the routine that runs when the title is dismissed. It branches on
bit 7 of `$0A4C`, the "build already finished" flag:

```
80:E5C7  LDA !$0A4C
80:E5CA  BIT #$0080
80:E5CD  BNE $E5D9        ; already complete -> skip the snap
80:E5CF  JSL $80:F07E     ; snap the title to its finished state
80:E5D3  LDA #$0078       ; ...and hold 120 frames
80:E5D6  PHA
80:E5D7  BRA $E5DD
80:E5D9  LDA #$0028       ; already complete -> hold 40 frames
80:E5DC  PHA
80:E5DD  ...              ; load palette $80:E7D1 through $80:8A02
80:E5F9  JSL $80:86B0     ; wait one frame
80:E5FD  DEC A
80:E5FE  BPL $E5F9        ; runs count+1 times
80:E600  JSL $80:CF1B     ; fade out, then hand off to the next scene
```

`$80:F07E` does the snap by DMAing the finished title tilemap — 0x680 bytes
from `$7F:4006` — into VRAM in a single transfer, so the remaining pieces
appear at once instead of continuing to animate in.

`$80:CF1B` is the fade: `DEC $0562` once per frame until the brightness level
reaches zero.

These addresses came from a differential exec trace — the title screen traced
once with Start pressed and once without (`tools/mesen_title_trace.lua`); the
listed ranges are the ones that only execute on the pressed run.

### Measured against the ROM

| | ROM | port |
|---|---|---|
| press mid-build -> fade begins | 124 frames | 124 frames |
| press after build -> fade begins | 44 frames | 44 frames |
| snap latency | ~4 frames | 2 frames |
| fade length | 15 INIDISP steps | 15 steps |

The 124 and 44 figures are `#$0078`/`#$0028` plus one, because `DEC A / BPL`
runs the wait loop count+1 times, plus the ROM's input-detection latency.

**A second press during the hold does nothing.** Verified directly: pressing
Start at frame 1450 and again at 1500 produces a fade at exactly the same
frame as the single press. The hold is a fixed count, so the transition is
snap -> fixed hold -> fade, not snap -> wait for a second press.

The port needs one derived constant the ROM does not have. Its title is driven
by a reference frame stream rather than a tilemap, so the snap seeks that
stream to the point where the build has finished:
`NBA_TITLE_BUILD_COMPLETE_FRAMES = 965`, measured from the port's own stream
(the title scene starts at frame 649 and the build completes at 1614).
