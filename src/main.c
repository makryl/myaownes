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
  // test = "nes-test-roms/nmi_sync/demo_pal.nes";
  // test = "nes-test-roms/ppu_sprite_hit/rom_singles/09-timing.nes";
  // test = "nes-test-roms/ppu_sprite_overflow/rom_singles/03-timing.nes";
  // test = "nes-test-roms/tvpassfail/tv.nes";
  // test = "nes-test-roms/apu_mixer/square.nes";
  // test = "nes-test-roms/apu_mixer/triangle.nes";
  // test = "nes-test-roms/apu_mixer/noise.nes";
  // test = "nes-test-roms/apu_mixer/dmc.nes";
  // test = "nes-test-roms/dmc_tests/buffer_retained.nes";
  // test = "nes-test-roms/dmc_tests/latency.nes";
  // test = "nes-test-roms/dmc_tests/status_irq.nes";
  // test = "nes-test-roms/dmc_tests/status.nes";
  // test = "nes-test-roms/volume_tests/volumes.nes";
  // test = "nes-test-roms/other/8bitpeoples_-_deadline_console_invitro.nes";
  // test = "nes-test-roms/pal_apu_tests/01.len_ctr.nes";
  // test = "nes-test-roms/pal_apu_tests/02.len_table.nes";
  // test = "nes-test-roms/pal_apu_tests/03.irq_flag.nes";
  // test = "nes-test-roms/pal_apu_tests/04.clock_jitter.nes";
  // test = "nes-test-roms/pal_apu_tests/05.len_timing_mode0.nes";
  // test = "nes-test-roms/pal_apu_tests/06.len_timing_mode1.nes";
  // test = "nes-test-roms/pal_apu_tests/07.irq_flag_timing.nes";
  // test = "nes-test-roms/pal_apu_tests/08.irq_timing.nes";
  // test = "nes-test-roms/pal_apu_tests/10.len_halt_timing.nes";
  // test = "nes-test-roms/pal_apu_tests/11.len_reload_timing.nes";

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
  // test = "GoodNES/USA/Immortal, The (U) [!].nes";

  test = "GoodNES/Europe/Super Mario Bros. + Duck Hunt (E) [!].nes";
  // test = "GoodNES/USA/Elite (U) (Proto) [!].nes";
  // test = "GoodNES/Europe/Elite (E) [!].nes";
  // test = "GoodNES/Europe/Asterix (E) [!].nes";
  // test = "GoodNES/Europe/Smurfs, The (E) [!].nes";

  if (test) {
    chdir("../../tmp");
    imp_rom_load(test);
  }

  return true;
}

void imp_quit() {}
