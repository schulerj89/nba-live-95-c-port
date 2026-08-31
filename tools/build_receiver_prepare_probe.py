"""Fresh private build of the C2 receiver child and before-only adapter."""
import build_gameplay_hud_probe as builder
builder.NATIVE=['tools/receiver_prepare_probe.c','src/nba_receiver_prepare.c',
 'src/nba_assets.c','src/nba_ea_intro.c','src/nba_intro_text.c',
 'src/nba_rom_font.c','src/nba_renderer.c','src/nba_snes_ppu.c']
if __name__=='__main__':builder.main()
