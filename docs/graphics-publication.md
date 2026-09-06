# Graphics publication workflow

`NbaGame` owns one zero-allocated 128 KiB WRAM buffer for the lifetime of a
game instance. `NbaGraphicsBus` is only a borrowed view. The game, renderer,
and active Tipoff scene point at the same bytes; none of those views owns,
copies, seeds, or drains the publication ring.

`nba_game_init` supplies the host zero-power-on profile and binds the game and
renderer views. Ordinary `nba_game_enter_state` calls clear only the scene
union, so WRAM, queue cursors, records, `$012C`, and upper WRAM persist. A
successful Tipoff initialization receives a fresh borrowed view. Shutdown and
initialization failure clear all live views before freeing the allocation.

The source context is the native low-WRAM clearing and cursor setup at
`$80:80C5-$8136`; that range does not establish a whole-WRAM clear. The host
allocation is lifecycle support and carries no native routine-completion
credit. The production manifest compiles `nba_graphics_bus_view.c` alone.
`nba_graphics_bus.c` still contains the dormant bounded consumer and remains
unlinked until its real NMI ordering and sinks are represented.

The next native graphics step is `$85:8B6C-$8B75` calling
`$80:AB7E-$AC0C`, including its necessary `$80:AC0D` and `$80:AC89`
children, to establish the real `$05EB/$05EF` allocator inputs. Next,
`$87:AFA2` invalidates the cache and `$87:B05B-$B354` publishes the six jersey
views per actor. Only then can `$80:AD2B-$AD88` append queue records from real
sources. The remaining ordered producers and consumer must establish canonical
`$012C` provenance before `$87:B7D8` can support the human pass catch path.

Run the focused lifetime check with:

~~~powershell
./build.ps1 -AssetPack build/nba95_assets.pak
./tools/build_vector_probe.ps1 -Name game_wram_lifetime_probe
python tools/test_game_wram_lifetime.py --probe build/game_wram_lifetime_probe.exe --pack build/nba95_assets.pak
~~~

The probe calls the real game initializer, front-end scene entries,
`nba_session_begin_match`, Tipoff initialization, shutdown, reinitialization,
and an initialization failure. Its writes are alias/lifetime sentinels only;
they are not native behavior fixtures.
