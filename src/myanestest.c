#include "myanes.h"
#include "imp_sdl.h"

Rom test_rom;

void mn_init()
{
  // test_rom = mn_nes_file("../../data/nestest.nes");
  test_rom = mn_nes_file("../../data/ppu_vbl_nmi.nes");
  mn_load(test_rom);
  mn_power();
}

void mn_quit() { mn_nes_release(test_rom); }
