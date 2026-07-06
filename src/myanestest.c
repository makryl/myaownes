#include "myanes.h"
#include "imp_sdl.h"

Rom test_rom;

bool mn_init()
{
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/other/nestest.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/240pee/240pee.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/ppu_vbl_nmi/ppu_vbl_nmi.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/nmi_sync/demo_ntsc.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/ppu_sprite_hit/rom_singles/09-timing.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/ppu_sprite_overflow/rom_singles/03-timing.nes");

  // test_rom = mn_rom_file("../../tmp/GoodNES/USA/Battletoads (U) [!].nes");
  // test_rom = mn_rom_file("../../tmp/GoodNES/USA/Mega Man (U) [!].nes");
  if (!test_rom) {
    return false;
  }

  mn_rom_load(test_rom);
  mn_power();

  return true;
}

void mn_quit() { mn_rom_release(test_rom); }
