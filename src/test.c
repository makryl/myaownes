#include "myanes.h"
#include "imp_sdl.h"
#include <unistd.h>

Rom test_rom;

bool mn_init()
{
  chdir("../../tmp");

  // test_rom = mn_rom_file("nes-test-roms/other/nestest.nes");
  // test_rom = mn_rom_file("nes-test-roms/240pee/240pee.nes");
  // test_rom = mn_rom_file("nes-test-roms/ppu_vbl_nmi/ppu_vbl_nmi.nes");
  // test_rom = mn_rom_file("nes-test-roms/nmi_sync/demo_ntsc.nes");
  // test_rom = mn_rom_file("nes-test-roms/ppu_sprite_hit/rom_singles/09-timing.nes");
  // test_rom = mn_rom_file("nes-test-roms/ppu_sprite_overflow/rom_singles/03-timing.nes");

  // test_rom = mn_rom_file("GoodNES/USA/Mega Man (U) [!].nes");
  // test_rom = mn_rom_file("GoodNES/Japan/Fire Emblem Gaiden (J) [!].nes"); // mapper?

  // test_rom = mn_rom_file("GoodNES/USA/Battletoads (U) [!].nes");
  // test_rom = mn_rom_file("MegaPack/USA/Micro Machines (U).nes");
  // test_rom = mn_rom_file("GoodNES/USA/Marble Madness (U) [!].nes");
  // test_rom = mn_rom_file("GoodNES/USA/Crystalis (U) [!].nes");
  // test_rom = mn_rom_file("GoodNES/USA/Super Mario Bros. + Duck Hunt (U) [!].nes");
  // test_rom = mn_rom_file("GoodNES/USA/Super Mario Bros. 3 (U) (V1.1) [!].nes");
  // test_rom = mn_rom_file("GoodNES/USA/Rad Racer (U) [!].nes");
  // test_rom = mn_rom_file("GoodNES/USA/Legend of Zelda, The (U) (V1.1) [!].nes");
  // test_rom = mn_rom_file("GoodNES/USA/Kirby's Adventure (U) (V1.1) [!].nes");
  // test_rom = mn_rom_file("GoodNES/USA/Elite (U) (Proto) [!].nes"); // PAL?
  // test_rom = mn_rom_file("GoodNES/USA/Immortal, The (U) [!].nes");

  if (!test_rom) {
    return false;
  }

  if (!mn_rom_load(test_rom)) {
    return false;
  }

  mn_power();

  return true;
}

void mn_quit() { mn_rom_release(test_rom); }
