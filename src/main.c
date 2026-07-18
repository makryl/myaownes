#include "myaownes.h"
#include "imp_sdl.h"
#include <unistd.h>

bool imp_init()
{
  const char* test = nullptr;

  // test = "nes-test-roms/other/nestest.nes";
  // test = "nes-test-roms/240pee/240pee.nes";
  // test = "nes-test-roms/ppu_vbl_nmi/ppu_vbl_nmi.nes";
  // test = "nes-test-roms/nmi_sync/demo_ntsc.nes";
  // test = "nes-test-roms/ppu_sprite_hit/rom_singles/09-timing.nes";
  // test = "nes-test-roms/ppu_sprite_overflow/rom_singles/03-timing.nes";
  // test = "nes-test-roms/tvpassfail/tv.nes";

  // test = "GoodNES/USA/Mega Man (U) [!].nes";
  // test = "GoodNES/USA/Contra (U) [!].nes";
  // test = "GoodNES/Japan/Fire Emblem Gaiden (J) [!].nes"; // mapper?

  // test = "GoodNES/USA/Battletoads (U) [!].nes";
  // test = "MegaPack/USA/Micro Machines (U).nes";
  // test = "GoodNES/USA/Marble Madness (U) [!].nes";
  // test = "GoodNES/USA/Crystalis (U) [!].nes";
  // test = "GoodNES/USA/Super Mario Bros. + Duck Hunt (U) [!].nes";
  // test = "GoodNES/USA/Super Mario Bros. 3 (U) (V1.1) [!].nes";
  // test = "GoodNES/USA/Rad Racer (U) [!].nes";
  // test = "GoodNES/USA/Legend of Zelda, The (U) (V1.1) [!].nes";
  // test = "GoodNES/USA/Kirby's Adventure (U) (V1.1) [!].nes";
  // test = "GoodNES/USA/Elite (U) (Proto) [!].nes"; // PAL?
  // test = "GoodNES/USA/Immortal, The (U) [!].nes";

  if (test) {
    chdir("../../tmp");
    imp_rom_load(test);
  }

  return true;
}

void imp_quit() {}
