#include "myanes.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct
{
  u8 idLength;
  u8 colorMapType;
  u8 dataTypeCode;
  u16 colorMapOrigin;
  u16 colorMapLength;
  u8 colorMapDepth;
  u16 xOrigin;
  u16 yOrigin;
  u16 width;
  u16 height;
  u8 bitsPerPixel;
  u8 imageDescriptor;
} TgaHeader;

typedef struct
{
  u32 hash;
  u32 frames;
  u8 joy1;
  bool reset;
} TestStep;

typedef struct
{
  const char* path;
  TestStep steps[32];
} TestParams;

static TestParams tests[] = {
  // { 0, 0, "apu_mixer/dmc.nes" }, // todo
  // { 0, 0, "apu_mixer/noise.nes" }, // todo
  // { 0, 0, "apu_mixer/square.nes" }, // todo
  // { 0, 0, "apu_mixer/triangle.nes" }, // todo
  { "apu_reset/4015_cleared.nes", { { 0x03085F30, 20, 0, 0 }, { 0x68020FC4, 20, 0, 1 } } }, //
  { "apu_reset/4017_timing.nes", { { 0x65F6087F, 20, 0, 0 }, { 0x499911CA, 20, 0, 1 } } }, //
  { "apu_reset/4017_written.nes", { { 0x03085F30, 20, 0, 0 }, { 0x8BECFD90, 20, 0, 1 }, { 0x2421E3A9, 20, 0, 1 } } },
  { "apu_reset/irq_flag_cleared.nes", { { 0x03085F30, 20, 0, 0 }, { 0x6157F5A6, 20, 0, 1 } } }, //
  { "apu_reset/len_ctrs_enabled.nes", { { 0x03085F30, 20, 0, 0 }, { 0xB62FEAE3, 20, 0, 1 } } }, //
  { "apu_reset/works_immediately.nes", { { 0x03085F30, 20, 0, 0 }, { 0x0000839C, 20, 0, 1 } } }, //
  { "apu_test/rom_singles/1-len_ctr.nes", { { 0x8E3C39E5, 19, 0x00, 0 } } }, //
  { "apu_test/rom_singles/2-len_table.nes", { { 0x0D477786, 14, 0x00, 0 } } }, //
  { "apu_test/rom_singles/3-irq_flag.nes", { { 0xBA4FC3FB, 18, 0x00, 0 } } }, //
  { "apu_test/rom_singles/4-jitter.nes", { { 0x3BA3894E, 17, 0x00, 0 } } }, //
  { "apu_test/rom_singles/5-len_timing.nes", { { 0x5B826EC5, 111, 0x00, 0 } } }, //
  { "apu_test/rom_singles/6-irq_flag_timing.nes", { { 0x31D8322C, 20, 0x00, 0 } } }, //
  { "apu_test/rom_singles/7-dmc_basics.nes", { { 0x96FCAE66, 22, 0x00, 0 } } }, //
  { "apu_test/rom_singles/8-dmc_rates.nes", { { 0xF97AC6BC, 27, 0x00, 0 } } }, //
  // { "apu_test/apu_test.nes", { { 0x71B41136, 247, 0, 0 } } }, //
  { "blargg_apu_2005.07.30/01.len_ctr.nes", { { 0x2886AFFC, 24, 0x00, 0 } } }, //
  { "blargg_apu_2005.07.30/02.len_table.nes", { { 0x2886AFFC, 11, 0x00, 0 } } }, //
  { "blargg_apu_2005.07.30/03.irq_flag.nes", { { 0x2886AFFC, 18, 0x00, 0 } } }, //
  { "blargg_apu_2005.07.30/04.clock_jitter.nes", { { 0x2886AFFC, 15, 0x00, 0 } } }, //
  { "blargg_apu_2005.07.30/05.len_timing_mode0.nes", { { 0x2886AFFC, 21, 0x00, 0 } } }, //
  { "blargg_apu_2005.07.30/06.len_timing_mode1.nes", { { 0x2886AFFC, 23, 0x00, 0 } } }, //
  { "blargg_apu_2005.07.30/07.irq_flag_timing.nes", { { 0x2886AFFC, 17, 0x00, 0 } } }, //
  { "blargg_apu_2005.07.30/08.irq_timing.nes", { { 0x2886AFFC, 15, 0x00, 0 } } }, //
  { "blargg_apu_2005.07.30/09.reset_timing.nes", { { 0x2886AFFC, 11, 0x00, 0 } } }, //
  { "blargg_apu_2005.07.30/10.len_halt_timing.nes", { { 0x2886AFFC, 15, 0x00, 0 } } }, //
  { "blargg_apu_2005.07.30/11.len_reload_timing.nes", { { 0x2886AFFC, 15, 0x00, 0 } } }, //
  { "blargg_nes_cpu_test5/cpu.nes", { { 0x80ED81E0, 981, 0, 0 } } }, //
  { "blargg_nes_cpu_test5/official.nes", { { 0x80ED81E0, 643, 0, 0 } } }, //
  { "blargg_ppu_tests_2005.09.15b/palette_ram.nes", { { 0x2886AFFC, 18, 0x00, 0 } } }, //
  { "blargg_ppu_tests_2005.09.15b/sprite_ram.nes", { { 0x2886AFFC, 18, 0x00, 0 } } }, //
  { "blargg_ppu_tests_2005.09.15b/vbl_clear_time.nes", { { 0x2886AFFC, 23, 0x00, 0 } } }, //
  { "blargg_ppu_tests_2005.09.15b/vram_access.nes", { { 0x2886AFFC, 18, 0x00, 0 } } }, //
  { "branch_timing_tests/1.Branch_Basics.nes", { { 0xB6A4872B, 13, 0x00, 0 } } }, //
  { "branch_timing_tests/2.Backward_Branch.nes", { { 0x36D590FE, 15, 0x00, 0 } } }, //
  { "branch_timing_tests/3.Forward_Branch.nes", { { 0x6D0B3B45, 15, 0x00, 0 } } }, //
  { "cpu_dummy_reads/cpu_dummy_reads.nes", { { 0xA942A3CF, 47, 0x00, 0 } } }, //
  { "cpu_dummy_writes/cpu_dummy_writes_oam.nes", { { 0x0DEC41EF, 331, 0x00, 0 } } }, //
  { "cpu_dummy_writes/cpu_dummy_writes_ppumem.nes", { { 0x0124227E, 236, 0x00, 0 } } }, //
  { "cpu_exec_space/test_cpu_exec_space_apu.nes", { { 0xC9890A61, 301, 0x00, 0 } } }, //
  { "cpu_exec_space/test_cpu_exec_space_ppuio.nes", { { 0x31CA6229, 45, 0x00, 0 } } }, //
  { "cpu_interrupts_v2/rom_singles/1-cli_latency.nes", { { 0x04E43C3C, 14, 0x00, 0 } } }, //
  { "cpu_interrupts_v2/rom_singles/2-nmi_and_brk.nes", { { 0xEA3D7CA3, 105, 0x00, 0 } } }, //
  { "cpu_interrupts_v2/rom_singles/3-nmi_and_irq.nes", { { 0xB47C1A9A, 125, 0x00, 0 } } }, //
  { "cpu_interrupts_v2/rom_singles/4-irq_and_dma.nes", { { 0x62782874, 68, 0x00, 0 } } }, //
  { "cpu_interrupts_v2/rom_singles/5-branch_delays_irq.nes", { { 0x709B36CC, 383, 0x00, 0 } } }, //
  { "cpu_interrupts_v2/cpu_interrupts.nes", { { 0x12619948, 725, 0, 0 } } }, //
  { "cpu_reset/ram_after_reset.nes",
    { { 0x5B69B3BD, 100, 0, 0 }, { 0x07673647, 50, 0, 0 }, { 0x68B7E9F1, 20, 0, 1 } } }, //
  { "cpu_reset/registers.nes", { { 0x3235473C, 100, 0, 0 }, { 0x07673647, 50, 0, 0 }, { 0x8233D68A, 20, 0, 1 } } },
  { "cpu_timing_test6/cpu_timing_test.nes", { { 0x3751F886, 613, 0, 0 } } }, //
  { "dmc_dma_during_read4/dma_2007_read.nes", { { 0x8D3CE006, 23, 0x00, 0 } } }, //
  { "dmc_dma_during_read4/dma_2007_write.nes", { { 0x94175082, 28, 0x00, 0 } } }, //
  { "dmc_dma_during_read4/dma_4016_read.nes", { { 0x441C2F34, 16, 0x00, 0 } } }, //
  { "dmc_dma_during_read4/double_2007_read.nes", { { 0x80B4A112, 16, 0x00, 0 } } }, //
  { "dmc_dma_during_read4/read_write_2007.nes", { { 0xEB956420, 18, 0x00, 0 } } }, //
  // dmc_tests/* // todo
  // exram/mmc5exram.nes // todo
  { "dpcmletterbox/dpcmletterbox.nes", { { 0x24EBFE89, 10, 0x00, 0 }, { 0xCAB5C165, 216, 0xA0, 0 } } }, //
  { "full_palette/flowing_palette.nes", { { 0x426913E7, 30, 0, 0 } } }, //
  { "full_palette/full_palette_smooth.nes", { { 0x0C972D06, 30, 0, 0 } } }, //
  { "full_palette/full_palette.nes", { { 0x29F8072A, 30, 0, 0 } } }, //
  { "instr_misc/rom_singles/01-abs_x_wrap.nes", { { 0x290CAB05, 11, 0x00, 0 } } }, //
  { "instr_misc/rom_singles/02-branch_wrap.nes", { { 0x54041E4A, 12, 0x00, 0 } } }, //
  { "instr_misc/rom_singles/03-dummy_reads.nes", { { 0x4D1E0C1E, 56, 0x00, 0 } } }, //
  { "instr_misc/rom_singles/04-dummy_reads_apu.nes", { { 0x8808910F, 140, 0x00, 0 } } }, //
  { "instr_misc/instr_misc.nes", { { 0x0AE0BB6B, 226, 0, 0 } } }, //
  { "instr_test-v3/rom_singles/01-implied.nes", { { 0x0F76F905, 97, 0x00, 0 } } }, //
  { "instr_test-v3/rom_singles/02-immediate.nes", { { 0x60452B1E, 87, 0x00, 0 } } }, //
  { "instr_test-v3/rom_singles/03-zero_page.nes", { { 0x75E5E809, 116, 0x00, 0 } } }, //
  { "instr_test-v3/rom_singles/04-zp_xy.nes", { { 0x2D7C7D05, 260, 0x00, 0 } } }, //
  { "instr_test-v3/rom_singles/05-absolute.nes", { { 0x0E0A1490, 110, 0x00, 0 } } }, //
  { "instr_test-v3/rom_singles/06-abs_xy.nes", { { 0x393F728D, 365, 0x00, 0 } } }, //
  { "instr_test-v3/rom_singles/07-ind_x.nes", { { 0x8A0D9C19, 146, 0x00, 0 } } }, //
  { "instr_test-v3/rom_singles/08-ind_y.nes", { { 0x28E6D4DE, 138, 0x00, 0 } } }, //
  { "instr_test-v3/rom_singles/09-branches.nes", { { 0xB23466FB, 43, 0x00, 0 } } }, //
  { "instr_test-v3/rom_singles/10-stack.nes", { { 0x17116DAB, 164, 0x00, 0 } } }, //
  { "instr_test-v3/rom_singles/11-jmp_jsr.nes", { { 0x21549A14, 17, 0x00, 0 } } }, //
  { "instr_test-v3/rom_singles/12-rts.nes", { { 0xF10F555D, 14, 0x00, 0 } } }, //
  { "instr_test-v3/rom_singles/13-rti.nes", { { 0x935AD5B5, 14, 0x00, 0 } } }, //
  { "instr_test-v3/rom_singles/14-brk.nes", { { 0xDE89D3A9, 25, 0x00, 0 } } }, //
  { "instr_test-v3/rom_singles/15-special.nes", { { 0x49A4C0BC, 11, 0x00, 0 } } }, //
  { "instr_test-v3/all_instrs.nes", { { 0x8EEA7B5E, 2330, 0x00, 0 } } }, //
  { "instr_test-v3/official_only.nes", { { 0x8EEA7B5E, 1812, 0x00, 0 } } }, //
  { "instr_test-v5/rom_singles/01-basics.nes", { { 0xE654945D, 17, 0x00, 0 } } }, //
  { "instr_test-v5/rom_singles/02-implied.nes", { { 0xB2AF2680, 97, 0x00, 0 } } }, //
  { "instr_test-v5/rom_singles/03-immediate.nes", { { 0x8CE0845F, 87, 0x00, 0 } } }, //
  { "instr_test-v5/rom_singles/04-zero_page.nes", { { 0xE4C3F180, 116, 0x00, 0 } } }, //
  { "instr_test-v5/rom_singles/05-zp_xy.nes", { { 0x9EDDA0AE, 260, 0x00, 0 } } }, //
  { "instr_test-v5/rom_singles/06-absolute.nes", { { 0x5DFCDDD1, 110, 0x00, 0 } } }, //
  { "instr_test-v5/rom_singles/07-abs_xy.nes", { { 0x0067CDD4, 365, 0x00, 0 } } }, //
  { "instr_test-v5/rom_singles/08-ind_x.nes", { { 0x17036DB5, 147, 0x00, 0 } } }, //
  { "instr_test-v5/rom_singles/09-ind_y.nes", { { 0x9313623A, 137, 0x00, 0 } } }, //
  { "instr_test-v5/rom_singles/10-branches.nes", { { 0x43645E68, 41, 0x00, 0 } } }, //
  { "instr_test-v5/rom_singles/11-stack.nes", { { 0xB408FAE0, 164, 0x00, 0 } } }, //
  { "instr_test-v5/rom_singles/12-jmp_jsr.nes", { { 0x9C8D4591, 18, 0x00, 0 } } }, //
  { "instr_test-v5/rom_singles/13-rts.nes", { { 0x1DAAFA1C, 14, 0x00, 0 } } }, //
  { "instr_test-v5/rom_singles/14-rti.nes", { { 0x027CCC3C, 14, 0x00, 0 } } }, //
  { "instr_test-v5/rom_singles/15-brk.nes", { { 0x6D280E02, 26, 0x00, 0 } } }, //
  { "instr_test-v5/rom_singles/16-special.nes", { { 0x1A5209FD, 13, 0x00, 0 } } }, //
  { "instr_test-v5/all_instrs.nes", { { 0xB619C5B3, 2398, 0, 0 } } }, //
  { "instr_test-v5/official_only.nes", { { 0xB619C5B3, 1871, 0x00, 0 } } }, //
  { "instr_timing/rom_singles/1-instr_timing.nes", { { 0xE82C7F11, 1015, 0x00, 0 } } }, //
  { "instr_timing/rom_singles/2-branch_timing.nes", { { 0xB8E3CDB8, 140, 0x00, 0 } } }, //
  { "instr_timing/instr_timing.nes", { { 0x6F1BAA40, 1302, 0, 0 } } }, //
  { "MMC1_A12/mmc1_a12.nes", { { 0x382587DA, 27, 0x00, 0 } } }, //
  { "mmc3_irq_tests/1.Clocking.nes", { { 0xB732D83F, 18, 0x00, 0 } } }, //
  { "mmc3_irq_tests/2.Details.nes", { { 0x3E64BDBE, 23, 0x00, 0 } } }, //
  { "mmc3_irq_tests/3.A12_clocking.nes", { { 0x9A45FD7C, 18, 0x00, 0 } } }, //
  { "mmc3_irq_tests/4.Scanline_timing.nes", { { 0xA6F1F0EA, 79, 0x00, 0 } } }, //
  { "mmc3_irq_tests/5.MMC3_rev_A_m12.nes", { { 0x022863D4, 18, 0x00, 0 } } }, //
  { "mmc3_irq_tests/6.MMC3_rev_B.nes", { { 0x6DBFFE7B, 18, 0x00, 0 } } }, //
  { "mmc3_test/1-clocking.nes", { { 0x3D4C2E04, 24, 0x00, 0 } } }, //
  { "mmc3_test/2-details.nes", { { 0x390FC588, 26, 0x00, 0 } } }, //
  { "mmc3_test/3-A12_clocking.nes", { { 0xD3189FEE, 24, 0x00, 0 } } }, //
  { "mmc3_test/4-scanline_timing.nes", { { 0xF6D14A7E, 315, 0x00, 0 } } }, //
  { "mmc3_test/5-MMC3.nes", { { 0xFAF35560, 23, 0x00, 0 } } }, //
  { "mmc3_test/6-MMC6_m12.nes", { { 0xAD40DA76, 23, 0x00, 0 } } }, //
  { "mmc3_test_2/rom_singles/1-clocking.nes", { { 0x3D4C2E04, 24, 0x00, 0 } } }, //
  { "mmc3_test_2/rom_singles/2-details.nes", { { 0x390FC588, 26, 0x00, 0 } } }, //
  { "mmc3_test_2/rom_singles/3-A12_clocking.nes", { { 0xD3189FEE, 24, 0x00, 0 } } }, //
  { "mmc3_test_2/rom_singles/4-scanline_timing.nes", { { 0xF6D14A7E, 319, 0x00, 0 } } }, //
  { "mmc3_test_2/rom_singles/5-MMC3.nes", { { 0xFAF35560, 25, 0x00, 0 } } }, //
  { "mmc3_test_2/rom_singles/6-MMC3_alt_m12.nes", { { 0x09093FB9, 24, 0x00, 0 } } }, //
  // { "mmc5test_v2/mmc5test.nes", {} }, // todo
  { "nes_instr_test/rom_singles/01-implied.nes", { { 0x0F76F905, 63, 0x00, 0 } } }, //
  { "nes_instr_test/rom_singles/02-immediate.nes", { { 0x60452B1E, 57, 0x00, 0 } } }, //
  { "nes_instr_test/rom_singles/03-zero_page.nes", { { 0x75E5E809, 75, 0x00, 0 } } }, //
  { "nes_instr_test/rom_singles/04-zp_xy.nes", { { 0x2D7C7D05, 174, 0x00, 0 } } }, //
  { "nes_instr_test/rom_singles/05-absolute.nes", { { 0x0E0A1490, 71, 0x00, 0 } } }, //
  { "nes_instr_test/rom_singles/06-abs_xy.nes", { { 0x393F728D, 244, 0x00, 0 } } }, //
  { "nes_instr_test/rom_singles/07-ind_x.nes", { { 0x8A0D9C19, 104, 0x00, 0 } } }, //
  { "nes_instr_test/rom_singles/08-ind_y.nes", { { 0x28E6D4DE, 98, 0x00, 0 } } }, //
  { "nes_instr_test/rom_singles/09-branches.nes", { { 0xB23466FB, 32, 0x00, 0 } } }, //
  { "nes_instr_test/rom_singles/10-stack.nes", { { 0x17116DAB, 128, 0x00, 0 } } }, //
  { "nes_instr_test/rom_singles/11-special.nes", { { 0x3A5F745A, 10, 0x00, 0 } } }, //
  { "nmi_sync/demo_ntsc.nes",
    { { 0xB9F1DFBA, 29, 0, 0 }, { 0x1D8EF9BE, 1, 0, 0 }, { 0xB9F1DFBA, 1, 0, 0 }, { 0x1D8EF9BE, 1, 0, 0 } } }, //
  // { 0, 0, "nmi_sync/demo_pal.nes" }, // todo
  { "oam_read/oam_read.nes", { { 0xA35A57E7, 32, 0x00, 0 } } }, //
  { "oam_stress/oam_stress.nes", { { 0x596023FD, 1703, 0x00, 0 } } }, //
  { "other/midscanline.nes", { { 0x42311477, 20, 0x00, 0 } } }, //
  { "other/nestest.nes",
    { { 0x9E8BA5E6, 10, 0, 0 },
      { 0xDF7DA5E2, 20, 0x08, 0 },
      { 0xD6C2CFA3, 10, 0x04, 0 },
      { 0xDEA9BC46, 20, 0x08, 0 } } }, //
  { "other/oam3.nes", { { 0x4419951C, 10, 0x00, 0 } } }, //
  { "other/read2004.nes", { { 0x61473648, 20, 0x00, 0 } } }, //
  // { 0, 0, "pal_apu_tests/*.nes" }, // todo
  { "ppu_open_bus/ppu_open_bus.nes", { { 0x3EB4ABE5, 250, 0x00, 0 } } }, //
  { "ppu_read_buffer/test_ppu_read_buffer.nes", { { 0xD22D42A2, 1307, 0x00, 0 } } }, //
  { "ppu_sprite_hit/rom_singles/01-basics.nes", { { 0xE654945D, 43, 0x00, 0 } } }, //
  { "ppu_sprite_hit/rom_singles/02-alignment.nes", { { 0x057D9670, 40, 0x00, 0 } } }, //
  { "ppu_sprite_hit/rom_singles/03-corners.nes", { { 0xEFCA28B7, 25, 0x00, 0 } } }, //
  { "ppu_sprite_hit/rom_singles/04-flip.nes", { { 0xB297CC81, 21, 0x00, 0 } } }, //
  { "ppu_sprite_hit/rom_singles/05-left_clip.nes", { { 0x9DF4E002, 38, 0x00, 0 } } }, //
  { "ppu_sprite_hit/rom_singles/06-right_edge.nes", { { 0x601652B5, 28, 0x00, 0 } } }, //
  { "ppu_sprite_hit/rom_singles/07-screen_bottom.nes", { { 0x50189868, 33, 0x00, 0 } } }, //
  { "ppu_sprite_hit/rom_singles/08-double_height.nes", { { 0x53286670, 24, 0x00, 0 } } }, //
  { "ppu_sprite_hit/rom_singles/09-timing.nes", { { 0x00CC842A, 177, 0x00, 0 } } }, //
  { "ppu_sprite_hit/rom_singles/10-timing_order.nes", { { 0x94A60512, 69, 0x00, 0 } } }, //
  { "ppu_sprite_hit/ppu_sprite_hit.nes", { { 0xDDE50B5E, 584, 0x00, 0 } } }, //
  { "ppu_sprite_overflow/rom_singles/01-basics.nes", { { 0xE654945D, 25, 0x00, 0 } } }, //
  { "ppu_sprite_overflow/rom_singles/02-details.nes", { { 0x7111D067, 30, 0x00, 0 } } }, //
  { "ppu_sprite_overflow/rom_singles/03-timing.nes", { { 0x6E1E7158, 316, 0x00, 0 } } }, //
  { "ppu_sprite_overflow/rom_singles/04-obscure.nes", { { 0xCE39FCC6, 26, 0x00, 0 } } }, //
  { "ppu_sprite_overflow/rom_singles/05-emulator.nes", { { 0x5E275C0F, 19, 0x00, 0 } } }, //
  { "ppu_sprite_overflow/ppu_sprite_overflow.nes", { { 0x12619948, 457, 0x00, 0 } } }, //
  { "ppu_vbl_nmi/rom_singles/01-vbl_basics.nes", { { 0x1AF3E9A3, 140, 0x00, 0 } } }, //
  { "ppu_vbl_nmi/rom_singles/02-vbl_set_time.nes", { { 0x59B9C83E, 178, 0x00, 0 } } }, //
  { "ppu_vbl_nmi/rom_singles/03-vbl_clear_time.nes", { { 0x7B0B8BE1, 169, 0x00, 0 } } }, //
  { "ppu_vbl_nmi/rom_singles/04-nmi_control.nes", { { 0xCD67F9C1, 31, 0x00, 0 } } }, //
  { "ppu_vbl_nmi/rom_singles/05-nmi_timing.nes", { { 0xE19A001A, 219, 0x00, 0 } } }, //
  { "ppu_vbl_nmi/rom_singles/06-suppression.nes", { { 0x4D1533CC, 222, 0x00, 0 } } }, //
  { "ppu_vbl_nmi/rom_singles/07-nmi_on_timing.nes", { { 0x1658D4F5, 197, 0x00, 0 } } }, //
  { "ppu_vbl_nmi/rom_singles/08-nmi_off_timing.nes", { { 0xA3F47CBF, 221, 0x00, 0 } } }, //
  { "ppu_vbl_nmi/rom_singles/09-even_odd_frames.nes", { { 0xA590A81C, 76, 0x00, 0 } } }, //
  { "ppu_vbl_nmi/rom_singles/10-even_odd_timing.nes", { { 0xB3F44119, 142, 0x00, 0 } } }, //
  { "ppu_vbl_nmi/ppu_vbl_nmi.nes", { { 0xDDE50B5E, 1616, 0, 0 } } }, //
  { "read_joy3/count_errors_fast.nes", { { 0xE8E7148C, 35, 0x00, 0 } } }, //
  { "read_joy3/count_errors.nes", { { 0x2B5D8852, 59, 0x00, 0 } } }, //
  { "read_joy3/test_buttons.nes",
    { { 0x3162E4DC, 10, 0, 0 },
      { 0x3162E4DC, 10, 0x01, 0 },
      { 0x8588E368, 10, 0, 0 },
      { 0x8588E368, 10, 0x02, 0 },
      { 0x73A2854D, 10, 0, 0 },
      { 0x73A2854D, 10, 0x04, 0 },
      { 0x47A1151A, 10, 0, 0 },
      { 0x47A1151A, 10, 0x08, 0 },
      { 0x731A42E0, 10, 0, 0 },
      { 0x731A42E0, 10, 0x10, 0 },
      { 0x3B40BF56, 10, 0, 0 },
      { 0x3B40BF56, 10, 0x20, 0 },
      { 0xE614B6BA, 10, 0, 0 },
      { 0xE614B6BA, 10, 0x40, 0 },
      { 0x3952B9C7, 10, 0, 0 },
      { 0x3952B9C7, 10, 0x80, 0 },
      { 0x9C94D0D4, 10, 0, 0 } } }, //
  { "read_joy3/thorough_test.nes", { { 0x6ECF1F4B, 123, 0x00, 0 } } }, //
  { "scanline/scanline.nes", { { 0xAF352CF3, 80, 0, 0 } } }, // area1 flickering pixels after line
  { "scanline-a1/scanline.nes", { { 0xAF352CF3, 80, 0, 0 } } }, // same as above
  { "scrolltest/scroll.nes", { { 0xCE84FBEF, 7, 0, 0 }, { 0x3DD88BC3, 0, 0xA0, 0 } } }, //
  { "sprdma_and_dmc_dma/sprdma_and_dmc_dma.nes", { { 0xF4998922, 139, 0x00, 0 } } }, //
  { "sprdma_and_dmc_dma/sprdma_and_dmc_dma_512.nes", { { 0x42149129, 143, 0x00, 0 } } }, //
  { "sprite_hit_tests_2005.10.05/01.basics.nes", { { 0xAC38F2AD, 33, 0x00, 0 } } }, //
  { "sprite_hit_tests_2005.10.05/02.alignment.nes", { { 0x3BF08842, 31, 0x00, 0 } } }, //
  { "sprite_hit_tests_2005.10.05/03.corners.nes", { { 0xC53BD4BF, 20, 0x00, 0 } } }, //
  { "sprite_hit_tests_2005.10.05/04.flip.nes", { { 0x0B05D9E8, 19, 0x00, 0 } } }, //
  { "sprite_hit_tests_2005.10.05/05.left_clip.nes", { { 0x95D74663, 30, 0x00, 0 } } }, //
  { "sprite_hit_tests_2005.10.05/06.right_edge.nes", { { 0x420C5953, 23, 0x00, 0 } } }, //
  { "sprite_hit_tests_2005.10.05/07.screen_bottom.nes", { { 0xCE4105CF, 24, 0x00, 0 } } }, //
  { "sprite_hit_tests_2005.10.05/08.double_height.nes", { { 0xCE51B630, 20, 0x00, 0 } } }, //
  { "sprite_hit_tests_2005.10.05/09.timing_basics.nes", { { 0xE68A1C8B, 72, 0x00, 0 } } }, //
  { "sprite_hit_tests_2005.10.05/10.timing_order.nes", { { 0xEBEBD82C, 72, 0x00, 0 } } }, //
  { "sprite_hit_tests_2005.10.05/11.edge_timing.nes", { { 0x055D6A29, 47, 0x00, 0 } } }, //
  { "sprite_overflow_tests/1.Basics.nes", { { 0x03CC909D, 15, 0x00, 0 } } }, //
  { "sprite_overflow_tests/2.Details.nes", { { 0xC833B4E1, 24, 0x00, 0 } } }, //
  { "sprite_overflow_tests/3.Timing.nes", { { 0x39154B5F, 128, 0x00, 0 } } }, //
  { "sprite_overflow_tests/4.Obscure.nes", { { 0xE2E52CC7, 22, 0x00, 0 } } }, //
  { "sprite_overflow_tests/5.Emulator.nes", { { 0xBAFE1B96, 13, 0x00, 0 } } }, //
  { "stomper/smwstomp.nes", { { 0xD5AE626D, 244, 0, 0 } } }, //
  // { 0, 0, "tvpassfail/tv.nes" }, // fail square
  { "vbl_nmi_timing/1.frame_basics.nes", { { 0x509B862D, 176, 0x00, 0 } } }, //
  { "vbl_nmi_timing/2.vbl_timing.nes", { { 0xACDD6906, 154, 0x00, 0 } } }, //
  { "vbl_nmi_timing/3.even_odd_frames.nes", { { 0xF2D9B6C9, 99, 0x00, 0 } } }, //
  { "vbl_nmi_timing/4.vbl_clear_timing.nes", { { 0x3B42F8FA, 117, 0x00, 0 } } }, //
  { "vbl_nmi_timing/5.nmi_suppression.nes", { { 0xB2955A03, 166, 0x00, 0 } } }, //
  { "vbl_nmi_timing/6.nmi_disable.nes", { { 0xFD58C6AA, 109, 0x00, 0 } } }, //
  { "vbl_nmi_timing/7.nmi_timing.nes", { { 0xE5553CE0, 109, 0x00, 0 } } }, //
};

u32 calc_crc(const u8* data, u32 size)
{
  u32 crc = 0xFFFFFFFF;
  for (u32 i = 0; i < size; ++i) {
    crc ^= data[i];
    for (u32 j = 0; j < 8; j++) {
      crc = (crc >> 1) ^ ((crc & 1) ? 0xEDB88320 : 0);
    }
  }
  return crc ^ 0xFFFFFFFF;
}

void save_tga(const char* img_path, const void* pixels, u16 width, u16 height)
{
  FILE* f = fopen(img_path, "wb");
  if (!f) {
    return;
  }

  u8 header[18] = {};
  header[2] = 2;
  header[12] = width & 0xFF;
  header[13] = (width >> 8) & 0xFF;
  header[14] = height & 0xFF;
  header[15] = (height >> 8) & 0xFF;
  header[16] = 32;
  header[17] = 0x20;

  fwrite(header, 1, 18, f);
  fwrite(pixels, 4, (size_t)width * height, f);

  fclose(f);
}

void save_test_step_img(const char* rom_path, u8 step)
{
  char img_path[512] = {};
  sprintf(img_path, "screenshots/%s.%d.tga", rom_path, step);
  size_t img_path_len = strlen(img_path);
  for (size_t i = 0; i <= img_path_len; i++) {
    if (img_path[i] == '/') {
      img_path[i] = 0;
      mkdir(img_path, 0777);
      img_path[i] = '/';
    }
  }
  save_tga(img_path, mn_output(), 256, 240);
}

u32 run_test_step(TestStep step)
{
  if (step.reset) {
    mn_reset();
  }
  for (u32 i = 0; i < step.frames; ++i) {
    mn_frame(step.joy1, 0);
  }
  return calc_crc((u8*)mn_output(), 256 * 240 * 4);
}

bool run_test_steps(TestParams params, u32* out_hash)
{
  Rom rom = mn_rom_file(params.path);
  mn_rom_set(rom);

  bool result = true;
  for (u8 i = 0; i < 32; ++i) {
    if (i > 0 && params.steps[i].frames == 0) {
      break;
    }
    u32 hash = run_test_step(params.steps[i]);
    if (out_hash) {
      *out_hash = hash;
    }
    save_test_step_img(params.path, i);
    if (hash != params.steps[i].hash) {
      result = false;
      break;
    }
  }

  mn_rom_release(rom);
  return result;
}

bool run_test(TestParams params)
{
  bool result = false;
  if (!params.steps[0].hash || !params.steps[0].frames) {
    if (!params.steps[0].frames) {
      params.steps[0].frames = 2400;
    }
    run_test_steps(params, &params.steps[0].hash);

    u32 frames_pass = params.steps[0].frames;
    u32 frames_fail = 0;
    do {
      params.steps[0].frames = (frames_pass + frames_fail) / 2;
      if (run_test_steps(params, 0)) {
        frames_pass = params.steps[0].frames;
      } else {
        frames_fail = params.steps[0].frames;
      }
    } while (frames_pass - frames_fail > 1);
    params.steps[0].frames = frames_pass;
    run_test_steps(params, 0);

    printf("  { \"%s\", { { 0x%08X, %d, 0x%02X, %d } } }, //\n", params.path, params.steps[0].hash,
           params.steps[0].frames, params.steps[0].joy1, params.steps[0].reset);
  } else {
    u32 hash = 0;
    result = run_test_steps(params, &hash);
    printf("%4s | %08X | %s\n", result ? "OK" : "FAIL", hash, params.path);
  }
  return result;
}

int main(int, char**)
{
  chdir("../../tmp/nes-test-roms");

  u32 count = sizeof(tests) / sizeof(tests[0]);
  u32 errors = 0;
  for (u32 i = 0; i < count; ++i) {
    if (!run_test(tests[i])) {
      ++errors;
    }
  }
  printf("%4s | %-3d/%4d | TOTAL ERRORS\n", errors ? "FAIL" : "OK", errors, count);

  return 0;
}
