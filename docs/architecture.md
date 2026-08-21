# Runtime architecture

`NbaGame` owns process-lifetime systems (ROM, asset pack, renderer, audio), a
persistent `NbaSession`, and a tagged-by-`NbaGameState` union containing only
the active heavy scene. Title and Setup therefore share scene storage instead
of permanently adding separate 64 KiB PPU images to every game instance.

`nba_game_enter_state` is the sole scene-entry API. It stops outgoing scene
audio, resets scene-local timing, clears the inactive union, initializes the
new scene, and applies its audio policy. GUI shortcuts, headless starts, and
production transitions all use this path.

`NbaSession.config` owns Mode, Style, Level, Quarter, Rules, and Options.
`NbaSetupScreen` receives a pointer to that configuration and owns only
temporary presentation/input state. Reinitializing Setup therefore cannot
silently restore defaults. Its update result separates menu sound from
navigation action. Confirming a main value row emits the generic `CONFIRM_MODE`
event; the scene dispatcher then routes the persistent mode value to Exhibition
team selection, Season, Playoffs, or Load Series without conflating them.

Audio synthesis and host playback are separate outcomes. Headless runs disable
host playback before entering an audio scene, retain synthesized PCM for WAV
dumps/fingerprints, and report `READY`. GUI playback reports `PLAYING` or
`HOST_FAIL`; a missing output device no longer destroys valid synthesized PCM.
