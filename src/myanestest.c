#include "myanes.h"
#include "imp_sdl.h"

Rom test_rom;

bool mn_init()
{
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/other/nestest.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/branch_timing_tests/1.Branch_Basics.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/branch_timing_tests/2.Backward_Branch.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/branch_timing_tests/3.Forward_Branch.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/cpu_dummy_reads/cpu_dummy_reads.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/cpu_dummy_writes/cpu_dummy_writes_oam.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/cpu_dummy_writes/cpu_dummy_writes_ppumem.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/cpu_exec_space/test_cpu_exec_space_apu.nes"); // fail
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/cpu_exec_space/test_cpu_exec_space_ppuio.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/cpu_interrupts_v2/rom_singles/1-cli_latency.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/cpu_interrupts_v2/rom_singles/2-nmi_and_brk.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/cpu_interrupts_v2/rom_singles/3-nmi_and_irq.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/cpu_interrupts_v2/rom_singles/4-irq_and_dma.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/cpu_interrupts_v2/rom_singles/5-branch_delays_irq.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/cpu_interrupts_v2/cpu_interrupts.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/cpu_reset/ram_after_reset.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/cpu_reset/registers.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/cpu_timing_test6/cpu_timing_test.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/full_palette/flowing_palette.nes"); // iNES 2.0 bug in rom.c
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/full_palette/full_palette_smooth.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/full_palette/full_palette.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/instr_misc/instr_misc.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/instr_test-v5/all_instrs.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/instr_timing/instr_timing.nes"); // was apu fail, now freeze
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/MMC1_A12/mmc1_a12.nes"); // fail
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/mmc3_test_2/rom_singles/1-clocking.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/mmc3_test_2/rom_singles/2-details.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/mmc3_test_2/rom_singles/3-A12_clocking.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/mmc3_test_2/rom_singles/4-scanline_timing.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/mmc3_test_2/rom_singles/5-MMC3.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/nmi_sync/demo_ntsc.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/nmi_sync/demo_pal.nes"); // not implemented
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/oam_read/oam_read.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/oam_stress/oam_stress.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/ppu_open_bus/ppu_open_bus.nes"); // open bus 1 sec decay fail
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/ppu_read_buffer/test_ppu_read_buffer.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/ppu_vbl_nmi/rom_singles/01-vbl_basics.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/ppu_vbl_nmi/rom_singles/02-vbl_set_time.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/ppu_vbl_nmi/rom_singles/03-vbl_clear_time.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/ppu_vbl_nmi/rom_singles/04-nmi_control.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/ppu_vbl_nmi/rom_singles/05-nmi_timing.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/ppu_vbl_nmi/rom_singles/06-suppression.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/ppu_vbl_nmi/rom_singles/07-nmi_on_timing.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/ppu_vbl_nmi/rom_singles/08-nmi_off_timing.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/ppu_vbl_nmi/rom_singles/09-even_odd_frames.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/ppu_vbl_nmi/rom_singles/10-even_odd_timing.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/ppu_vbl_nmi/ppu_vbl_nmi.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/read_joy3/count_errors_fast.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/read_joy3/count_errors.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/read_joy3/test_buttons.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/read_joy3/thorough_test.nes"); // black screen
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/scanline/scanline.nes"); // area1 flickering pixels after line,
  // can fix by removing alignment cycle in oam dma, but is that correct?
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/scanline-a1/scanline.nes"); // same as above
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/scrolltest/scroll.nes"); // fixed bottom panel flickers,
  // same as nestopia and mesen - is that correct?
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/sprdma_and_dmc_dma/sprdma_and_dmc_dma.nes"); // infinite
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/sprdma_and_dmc_dma/sprdma_and_dmc_dma_512.nes"); // infinite
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
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/stomper/smwstomp.nes");
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/stress/NEStress.NES"); // cpu ok, other has errors
  // test_rom = mn_rom_file("../../tmp/nes-test-roms/tvpassfail/tv.nes"); // fail square

  // test_rom = mn_rom_file("../../tmp/Battletoads (U).nes");
  // test_rom = mn_rom_file("../../tmp/GoodNES/USA/Mega Man (U) [!].nes");
  if (!test_rom) {
    return false;
  }

  mn_rom_load(test_rom);
  mn_power();

  return true;
}

void mn_quit() { mn_rom_release(test_rom); }
