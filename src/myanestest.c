#include "myanes.h"
#include "imp_sdl.h"

Rom test_rom;

bool mn_init()
{
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/other/nestest.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/ppu_vbl_nmi/ppu_vbl_nmi.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/full_palette/full_palette.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/blargg_nes_cpu_test5/cpu.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/blargg_ppu_tests_2005.09.15b/palette_ram.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/blargg_ppu_tests_2005.09.15b/sprite_ram.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/blargg_ppu_tests_2005.09.15b/vbl_clear_time.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/blargg_ppu_tests_2005.09.15b/vram_access.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/cpu_interrupts_v2/cpu_interrupts.nes"); // apu fail
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/cpu_timing_test6/cpu_timing_test.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/cpu_dummy_reads/cpu_dummy_reads.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/cpu_dummy_writes/cpu_dummy_writes_oam.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/cpu_dummy_writes/cpu_dummy_writes_ppumem.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/instr_misc/instr_misc.nes"); // apu fail
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/instr_timing/instr_timing.nes"); // apu fail
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/ppu_open_bus/ppu_open_bus.nes"); // open bus 1 sec decay fail
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/ppu_read_buffer/test_ppu_read_buffer.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/scanline/scanline.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/scrolltest/scroll.nes");
  // test_rom = mn_rom_file("../../tmp/Battletoads (U).nes");
  test_rom = mn_rom_file("../../tmp/GoodNES/USA/Mega Man (U) [!].nes");
  if (!test_rom) {
    return false;
  }

  mn_rom_load(test_rom);
  mn_power();

  return true;
}

void mn_quit() { mn_rom_release(test_rom); }
