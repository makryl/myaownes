#include "myanes.h"
#include "imp_sdl.h"

Rom test_rom;

void mn_init()
{
  // test_rom = mn_nes_file("../../data/nestest.nes");
  // test_rom = mn_nes_file("../../data/ppu_vbl_nmi.nes");
  // test_rom = mn_nes_file("../../data/power_up_palette.nes");
  // test_rom = mn_nes_file("../../data/palette_ram.nes");
  test_rom = mn_nes_file("../../data/full_palette.nes");
  // test_rom = mn_nes_file("../../data/full_palette_smooth.nes");
  // test_rom = mn_nes_file("../../data/flowing_palette.nes");
  mn_load(test_rom);
  mn_power();
}

void mn_quit() { mn_nes_release(test_rom); }
