#include "myanes.h"
#include "imp_sdl.h"

Rom test_rom;

bool mn_init()
{
  // test_rom = mn_rom_file("../../data/nestest.nes");
  // test_rom = mn_rom_file("../../data/ppu_vbl_nmi.nes");
  // test_rom = mn_rom_file("../../data/full_palette.nes");
  // test_rom = mn_rom_file("../../data/full_palette_smooth.nes");
  // test_rom = mn_rom_file("../../data/flowing_palette.nes");
  // test_rom = mn_rom_file("../../data/blargg_ppu_tests_2005.09.15b/sprite_ram.nes");
  // test_rom = mn_rom_file("../../data/blargg_ppu_tests_2005.09.15b/vbl_clear_time.nes");
  // test_rom = mn_rom_file("../../data/blargg_ppu_tests_2005.09.15b/vram_access.nes");
  // test_rom = mn_rom_file("../../data/blargg_ppu_tests_2005.09.15b/palette_ram.nes");
  // test_rom = mn_rom_file("../../data/blargg_ppu_tests_2005.09.15b/power_up_palette.nes");
  // test_rom = mn_rom_file("../../data/scroll.nes");
  test_rom = mn_rom_file("../../tmp/Battletoads (U).nes");
  if (!test_rom) {
    return false;
  }

  mn_rom_load(test_rom);
  mn_power();

  return true;
}

void mn_quit() { mn_rom_release(test_rom); }
