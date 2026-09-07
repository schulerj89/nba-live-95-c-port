# Graphics publication workflow

`NbaGame` owns one zero-allocated 128 KiB WRAM buffer for the lifetime of a
game instance. `NbaGraphicsBus` is only a borrowed view. The game, renderer,
and active Tipoff scene point at the same bytes; none of those views owns,
copies, or drains the publication ring. Binding a view never initializes the
allocator.

`nba_game_init` supplies the host zero-power-on profile and binds the game and
renderer views. Ordinary `nba_game_enter_state` calls clear only the scene
union, so WRAM, queue cursors, records, `$012C`, and upper WRAM persist. A
successful Tipoff initialization receives a fresh borrowed view. Immediately
after that bind, `nba_game_enter_state` invokes the native-equivalent graphics
allocator once with `$05EB=$6000`, X=`$0000`, and Y=`$01E0`. Ordinary Tipoff
ticks do not repeat it; entering a later Tipoff initializes the persistent
allocator again. Shutdown and initialization failure clear all live views
before freeing the allocation.

The source context is the native low-WRAM clearing and cursor setup at
`$80:80C5-$8136`; that range does not establish a whole-WRAM clear. The host
allocation is lifecycle support and carries no native routine-completion
credit. The production manifest compiles the borrowed-view and allocator
modules.
`nba_graphics_bus.c` still contains the dormant bounded consumer and remains
unlinked until its real NMI ordering and sinks are represented.

`nba_graphics_allocator_initialize` represents `$80:AB7E-$AC0C`, including
the exact `$80:AC0D-$AC1A` 1,049-word cache clear and
`$80:AC89-$ACC1` sparse fill/buffer rotation. Its signed low-byte loop,
16-bit rounded-Y wrap, preserved record holes, and byte-only `$0566` ready
write replay repeated native write streams. The integration is a bounded
equivalent of caller `$85:8B6C-$8B75`; the caller's later `$8B79-$8B93`
writes and native court initialization history remain outside this claim. The
host still constructs Tipoff actors before binding canonical WRAM. It then
runs the allocator before the explicit active-player graphics publication.

After Tipoff initialization, canonical WRAM binding, and allocator setup,
`nba_tipoff_initialize_player_graphics` invokes `$87:AFA2-$B058`. The parent
publishes the overlapping `$180B-$180D` upload seed, retains each actor's
native palette/height/variant/head outputs, writes `FFFF` to the ten cache
words `$8E10,$8E12,...,$8E22`, and calls the existing `$87:B059-$B354`
compositor. Actor `n` owns `$C0` bytes beginning at `$8690+n*$C0`; its six
32-byte slots at offsets `$00,$20,$40,$60,$80,$A0` represent display
directions `3,7,4,0,2,6`. Actors 0-4 use context0 home/right and uniform side
zero; actors 5-9 use context1 visitor/left and uniform side one. The roster
slot comes from the actor identity, not the team ID or display direction.

The digit source at `$A6:AFD6`, BCD table at `$80:859C`, and player records
come from `NBA_ASSET_PLAYER_ANIMATIONS` and `NBA_ASSET_PLAYER_ROSTERS`.
`NBA_ASSET_PLAYER_DRAW_INPUTS` version two retains the literal eight-word
`$87:A99E` source table after its existing head and number tables. Directions
one and five contain `FFFF` and produce no number work.

During the literal player render path, `$87:A64D-$A656` publishes twice actor
identity as BE and the actor's actual +$52 display direction as C2. A composed
number submission carries nonzero `$0884` work into `$80:ACC2`, which calls
`nba_graphics_jersey_append` (`$80:AD2B-$AD88`). The appender always writes
the unshifted `actor+$05EF` destination index to DP `$04`. A cache miss selects
the pack-backed `$87:A99E` base, adds the actor's `$C0` source stride, and
appends the overlapping eight-byte record at `$0100+$0037`: type one, source,
bank `$7E`, length `$20`, and shifted VRAM destination. The fourth destination
shift carries into the subsequent `$05EB` addition when index bit 12 is set.
The tail advances by eight modulo `$0200`; a cache hit leaves it and the record
unchanged while still writing DP `$04`. Source buffers and cache words come
from the successful Tipoff publication above. The appender does not drain the
ring. A descriptor whose tail is `$0028` writes its length word across
`$012C`; complete producer/consumer ordering and the value present when the
human pass path reaches `$87:B7D8` remain unverified.

The remaining ordered producers and consumer must establish the canonical
`$012C` history before `$87:B7D8` can support the human pass catch path.

Run the focused lifetime check with:

~~~powershell
./build.ps1 -AssetPack build/nba95_assets.pak
./tools/build_vector_probe.ps1 -Name graphics_allocator_vector_probe,game_wram_lifetime_probe
python tools/verify_graphics_allocator_vectors.py --vectors tests/fixtures/graphics-allocator-witnesses.json --probe build/graphics_allocator_vector_probe.exe
python tools/test_game_wram_lifetime.py --probe build/game_wram_lifetime_probe.exe --pack build/nba95_assets.pak
~~~

Run the active-player publication replay with:

~~~powershell
./tools/build_vector_probe.ps1 -Name player_appearance_publication_probe
python tools/verify_player_appearance_publication.py `
  --appearance-vectors tests/fixtures/action-pose-witnesses.json `
  --jersey-vectors tests/fixtures/jersey-number-witnesses.json `
  --probe build/player_appearance_publication_probe.exe `
  --pack build/nba95_assets.pak
~~~

Run the jersey appender replay and real renderer caller with:

~~~powershell
./tools/build_vector_probe.ps1 -Name graphics_jersey_vector_probe,graphics_jersey_caller_probe
python tools/verify_graphics_jersey_vectors.py `
  --vectors tests/fixtures/graphics-jersey-witnesses.json `
  --probe build/graphics_jersey_vector_probe.exe `
  --pack build/nba95_assets.pak
python tools/test_graphics_jersey_caller.py `
  --probe build/graphics_jersey_caller_probe.exe `
  --pack build/nba95_assets.pak
~~~

The allocator verifier projects ordered native functional WRAM writes onto
asset-free nonzero memory and compares the complete production result and
exact changed-byte footprint. CPU return registers remain capture context,
since the C API exposes memory state and success only. The lifecycle probe
calls the real game initializer, front-end scene entries,
`nba_session_begin_match`, Tipoff initialization, shutdown, reinitialization,
and an initialization failure. It also proves allocator persistence through a
Tipoff tick and scene clear, then reinitialization on the next Tipoff. Its own
writes are alias/lifetime sentinels only; they are not native behavior fixtures.
The appender verifier projects exact ordered writes from two identical natural
captures plus repeated controlled wrap/carry witnesses onto nonzero WRAM. It
checks all 49 instruction starts, all six valid sources, cache hits and misses,
tail rollover, 16-bit arithmetic, the shift carry, and atomic host rejection.
The caller probe enters Tipoff through `NbaGame`, uses the real published
jersey buffers, and proves miss, hit, direction change, directions one/five,
and rollover behavior through `nba_game_render`.
