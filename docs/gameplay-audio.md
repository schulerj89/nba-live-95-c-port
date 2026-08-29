# Gameplay audio evidence and implementation map

## Current port boundary

As of commit `fa6fd63`, gameplay calls only the proven whistle renderer.
Crowd ambience/reactions, ball bounce/catch, rim/make, shoe squeak, player
contact, clock warning and buzzer commands are not yet dispatched by the C
gameplay state. F11 exposes 52 ROM-derived BRR/WAV resources, but those items
are an inspection catalog rather than a reconstructed gameplay sound bank.

The gameplay sound system is not one large WAV. The 65816 raises event bits,
`$82:FD65-$FF84` converts them to indexed commands, and `$80:9DF3` submits the
commands to the resident SPC700 driver. That driver selects BRR sources and
mixes/sequences as many as eight DSP voices. Native CPU-vs-CPU evidence shows
voices 6/7 continuously sequencing crowd beds while the short gameplay
effects use the other voices.

## Verified event/command map

| Native producer | Event | `$82` command selection | Observed DSP result |
| --- | --- | --- | --- |
| `$85:A43A-$A44B`, `$85:A582-$A588` | ball/floor bounce | `$13E7.0` -> `$23/$2B/$33/$23` | SRCN `$0A`, pitch variants `$0800/$07CE/$0832` |
| `$85:9EBE-$9EC4`, `$85:9FC5-$9FCB` | inner rim response | `$13E7.1` -> 16-entry table at `$82:F82A` | resident gameplay effect family; exact variant depends on RNG |
| `$85:A33C-$A342` | made basket | `$13E7.2` -> `$0C/$14/$1C/$0C` | captured `$0C`: SRCN `$04`, pitch `$0800` |
| `$85:9B8C-$9B9E` | outer rim contact | `$13E7.3` -> `$08/$10/$18/$08` | resident rim effect family |
| `$86:D34A-$D350` | ball acquisition/catch | `$13E7.4` -> `$24/$2C/$34/$24` | SRCN `$0B`, pitch `$0659/$0627/$068B` |
| `$86:986D-$994B` | close ball/player presentation contact | `$13E7.5` -> `$0D/$15/$1D/$0D` | captured `$0D`: SRCN `$05`, pitch `$0556` |
| `$85:98EC-$9903` | player direction-change cue | `$13E7.6` -> `$0E/$1E/$1E/$0E` | SRCN `$06`, pitch `$0659/$068B`; shoe/floor cue family |
| `$86:C0B3/$C110/$C9C3/$CA4A` | player collision/knockdown | `$13E7.7` -> eight-entry table at `$82:F872` | player-contact effect family |
| `$86:C6CC-$C6D2` | knockdown landing/continuation | `$13E7.8` -> `$20/$28/$30/$20` | player/body-floor effect family |
| `$85:EE56-$EE90` | final-five-second shot-clock ticks | `$13E7.10` -> `$40` | clock warning cue |
| `$87:9A03-$9A72` | expired shot clock while ball resolves | `$13E7.11` -> `$41` | shot-clock horn/buzzer cue |
| `$85:A2C3/$A302` | three-point scoring presentation | `$13E7.12` -> `$43` | three-point presentation cue |
| `$85:9413-$941F` | foul whistle | `$13E7.13` -> `$44` | SRCN `$12`, pitch `$0556`, VOL `$14/$14`, ADSR `$8E/$A0` |
| score/tip/foul side events | crowd reaction/stinger | `$13E9` -> `$38/$39/$3A-$3C/$2F` via `$80:9F0F` | captured `$39`: SRCN `$14`, pitch `$0556`; voices 6/7 retain the crowd bed |

The inner-rim and collision rows deliberately retain family labels until a
targeted native capture exercises every RNG variant. The table does not assign
plausible names to unheard variants.

## Gameplay bank and ROM assets

A fresh gameplay-frame-zero Mesen snapshot uses DSP directory page `$02` and
master volume `$7F/$7F`. It differs from asset 265 (Player Introduction SPC
RAM) in 58,623 of 65,536 bytes. Therefore the Player Introduction snapshot
must not be reused as the gameplay bank.

The active BRR data is nevertheless ROM-owned. Byte-identical searches from
the live gameplay SPC directory found these sources in the ROM:

| SRCN | Gameplay role observed | ROM BRR start |
| --- | --- | --- |
| `$04` | made-basket effect | `$A9:9BCA` (file `$149BCA`) |
| `$05` | close ball/player contact | `$A6:9904` (file `$131904`) |
| `$06` | direction-change/shoe cue | `$A6:D10C` (file `$13510C`) |
| `$0A` | ball bounce family | `$A6:8040` (file `$130040`) |
| `$0B` | catch/acquisition family | `$AA:8FF0` (file `$150FF0`) |
| `$0C/$0D` | continuously sequenced crowd beds | `$A1:D20B/$A1:8040` |
| `$12` | whistle | `$A8:BA04` (file `$143A04`) |
| `$14/$15` | crowd reaction/presentation family | `$A1:9C06/$A1:9F15` |
| `$1C` | additional gameplay effect | `$AC:8F08` (file `$160F08`) |

These ranges overlap the existing F11 ROM sample inventory. Runtime gameplay
still needs a ROM-derived gameplay bank/sequence asset and an event mixer;
Mesen RAM/DSP dumps remain comparison oracles and must not become shipped art
or audio assets.

## Reproduction

```powershell
.\tools\capture_gameplay_audio.ps1 `
  -OutputDir .analysis\gameplay-audio `
  -Frames 1500
```

The passive observer records:

- `gameplay_sound_commands.txt`: `$80:9DF3` command IDs plus raw gameplay state;
- `gameplay_apu_ports.txt`: cycle-order CPU writes to `$2140-$2143` with PC;
- `gameplay_dsp_voices.txt`: changed SRCN/pitch/envelope/volume per voice;
- `gameplay_spc_ram.bin` and `gameplay_spc_dsp.bin`: frame-zero comparison
  oracles, never runtime assets.

The recomp remains useful for executing the authentic SPC/APU core and for
CPU/APU differential state, while Ghidra supplies the event producers and
dispatcher control flow. Mesen is authoritative for the original ROM's
command timing and DSP results.
