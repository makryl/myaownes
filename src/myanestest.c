#include "myanes.h"
#include "imp_sdl.h"

Rom test_rom;

bool mn_init()
{
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/other/nestest.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/240pee/240pee.nes");

  // test_rom = mn_rom_file("../../tmp/nes-test-roms/ppu_vbl_nmi/ppu_vbl_nmi.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/nmi_sync/demo_ntsc.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/full_palette/flowing_palette.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/full_palette/full_palette_smooth.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/full_palette/full_palette.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/cpu_interrupts_v2/cpu_interrupts.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/apu_reset/4017_written.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/apu_test/rom_singles/7-dmc_basics.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/blargg_ppu_tests_2005.09.15b/vram_access.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/mmc3_irq_tests/3.A12_clocking.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/ppu_read_buffer/test_ppu_read_buffer.nes");

  // test_rom = mn_rom_file("../../tmp/nes-test-roms/dmc_dma_during_read4/dma_2007_read.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/dmc_dma_during_read4/dma_2007_write.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/dmc_dma_during_read4/dma_4016_read.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/dmc_dma_during_read4/double_2007_read.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/dmc_dma_during_read4/read_write_2007.nes");

  // test_rom = mn_rom_file("../../tmp/nes-test-roms/ppu_sprite_hit/rom_singles/01-basics.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/ppu_sprite_hit/rom_singles/02-alignment.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/ppu_sprite_hit/rom_singles/03-corners.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/ppu_sprite_hit/rom_singles/04-flip.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/ppu_sprite_hit/rom_singles/05-left_clip.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/ppu_sprite_hit/rom_singles/06-right_edge.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/ppu_sprite_hit/rom_singles/07-screen_bottom.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/ppu_sprite_hit/rom_singles/08-double_height.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/ppu_sprite_hit/rom_singles/09-timing.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/ppu_sprite_hit/rom_singles/10-timing_order.nes");

  // test_rom = mn_rom_file("../../tmp/nes-test-roms/ppu_sprite_overflow/rom_singles/01-basics.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/ppu_sprite_overflow/rom_singles/02-details.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/ppu_sprite_overflow/rom_singles/03-timing.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/ppu_sprite_overflow/rom_singles/04-obscure.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/ppu_sprite_overflow/rom_singles/05-emulator.nes");

  // test_rom = mn_rom_file("../../tmp/nes-test-roms/sprite_hit_tests_2005.10.05/01.basics.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/sprite_hit_tests_2005.10.05/02.alignment.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/sprite_hit_tests_2005.10.05/03.corners.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/sprite_hit_tests_2005.10.05/04.flip.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/sprite_hit_tests_2005.10.05/05.left_clip.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/sprite_hit_tests_2005.10.05/06.right_edge.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/sprite_hit_tests_2005.10.05/07.screen_bottom.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/sprite_hit_tests_2005.10.05/08.double_height.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/sprite_hit_tests_2005.10.05/09.timing_basics.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/sprite_hit_tests_2005.10.05/10.timing_order.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/sprite_hit_tests_2005.10.05/11.edge_timing.nes");

  // test_rom = mn_rom_file("../../tmp/nes-test-roms/sprite_overflow_tests/1.Basics.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/sprite_overflow_tests/2.Details.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/sprite_overflow_tests/3.Timing.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/sprite_overflow_tests/4.Obscure.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/sprite_overflow_tests/5.Emulator.nes");

  test_rom = mn_rom_file("../../tmp/GoodNES/USA/Battletoads (U) [!].nes");
  // test_rom = mn_rom_file("../../tmp/GoodNES/USA/Mega Man (U) [!].nes");
  if (!test_rom) {
    return false;
  }

  mn_rom_load(test_rom);
  mn_power();

  return true;
}

void mn_quit() { mn_rom_release(test_rom); }
