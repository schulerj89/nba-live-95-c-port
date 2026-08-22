# Runtime architecture

`NbaGame` owns process-lifetime systems (ROM, asset pack, renderer, audio), a
persistent `NbaSession`, and a tagged-by-`NbaGameState` union containing only
the active heavy scene. Title and Setup therefore share scene storage instead
of permanently adding separate 64 KiB PPU images to every game instance.

`nba_game_enter_state` is the sole scene-entry API. It stops outgoing scene
audio, resets scene-local timing, clears the inactive union, initializes the
new scene, and applies its audio policy. GUI shortcuts, headless starts, and
production transitions all use this path.

`NbaSession.config` owns Mode, Style, Level, Quarter, Rules, and Options;
`NbaSession.left_team/right_team` own the Exhibition matchup across scenes.
`NbaSetupScreen` receives a pointer to that configuration and owns only
temporary presentation/input state. Reinitializing Setup therefore cannot
silently restore defaults. Its update result separates menu sound from
navigation action. Confirming a main value row emits the generic `CONFIRM_MODE`
event; the scene dispatcher then routes Exhibition to the Team Select scene.
That handoff intentionally preserves the resident Setup SPC track. Team Select
uses its own scene-local active side, seven-position name/ranking selector, PPU presentation, and
transition state while writing team IDs back to the session.

Debug screens are process-lifetime overlays outside the scene union. F9 owns
`NbaPlayerLab`, freezes scene progression, and reads fixed roster, ROM-tile,
palette-table, and pose-layout assets from the loaded pack. F10 remains compact
state telemetry, F11 owns audio/sample inspection, and F12 owns the general
asset browser. Keeping Player Lab separate from gameplay lets the same roster
and appearance diagnostics survive future gameplay scene extraction.

Audio synthesis and host playback are separate outcomes. Headless runs disable
host playback before entering an audio scene, retain synthesized PCM for WAV
dumps/fingerprints, and report `READY`. GUI playback reports `PLAYING` or
`HOST_FAIL`; a missing output device no longer destroys valid synthesized PCM.
