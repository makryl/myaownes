#include "myaownes.h"

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
  uint hash;
  uint frames;
  u8 joy1;
  bool reset;
} TestStep;

typedef struct
{
  const char* path;
  uint tv;
  TestStep steps[32];
} TestParams;

static TestParams tests[] = {
  { "apu_reset/4015_cleared.nes", MN_TV_NTSC, { { 0x03085F30, 20, 0, 0 }, { 0x68020FC4, 20, 0, 1 } } }, //
  { "apu_reset/4017_timing.nes", MN_TV_NTSC, { { 0x65F6087F, 20, 0, 0 }, { 0x499911CA, 20, 0, 1 } } }, //
  { "apu_reset/4017_written.nes",
    MN_TV_NTSC,
    { { 0x03085F30, 20, 0, 0 }, { 0x8BECFD90, 20, 0, 1 }, { 0x2421E3A9, 20, 0, 1 } } },
  { "apu_reset/irq_flag_cleared.nes", MN_TV_NTSC, { { 0x03085F30, 20, 0, 0 }, { 0x6157F5A6, 20, 0, 1 } } }, //
  { "apu_reset/len_ctrs_enabled.nes", MN_TV_NTSC, { { 0x03085F30, 20, 0, 0 }, { 0xB62FEAE3, 20, 0, 1 } } }, //
  { "apu_reset/works_immediately.nes", MN_TV_NTSC, { { 0x03085F30, 20, 0, 0 }, { 0x0000839C, 20, 0, 1 } } }, //
  { "apu_test/rom_singles/1-len_ctr.nes", MN_TV_NTSC, { { 0x8E3C39E5, 19, 0x00, 0 } } }, //
  { "apu_test/rom_singles/2-len_table.nes", MN_TV_NTSC, { { 0x0D477786, 14, 0x00, 0 } } }, //
  { "apu_test/rom_singles/3-irq_flag.nes", MN_TV_NTSC, { { 0xBA4FC3FB, 18, 0x00, 0 } } }, //
  { "apu_test/rom_singles/4-jitter.nes", MN_TV_NTSC, { { 0x3BA3894E, 17, 0x00, 0 } } }, //
  { "apu_test/rom_singles/5-len_timing.nes", MN_TV_NTSC, { { 0x5B826EC5, 111, 0x00, 0 } } }, //
  { "apu_test/rom_singles/6-irq_flag_timing.nes", MN_TV_NTSC, { { 0x31D8322C, 20, 0x00, 0 } } }, //
  { "apu_test/rom_singles/7-dmc_basics.nes", MN_TV_NTSC, { { 0x96FCAE66, 22, 0x00, 0 } } }, //
  { "apu_test/rom_singles/8-dmc_rates.nes", MN_TV_NTSC, { { 0xF97AC6BC, 27, 0x00, 0 } } }, //
  // { "apu_test/apu_test.nes", MN_TV_NTSC, { { 0x71B41136, 247, 0, 0 } } }, //
  { "blargg_apu_2005.07.30/01.len_ctr.nes", MN_TV_NTSC, { { 0x2886AFFC, 24, 0x00, 0 } } }, //
  { "blargg_apu_2005.07.30/02.len_table.nes", MN_TV_NTSC, { { 0x2886AFFC, 11, 0x00, 0 } } }, //
  { "blargg_apu_2005.07.30/03.irq_flag.nes", MN_TV_NTSC, { { 0x2886AFFC, 18, 0x00, 0 } } }, //
  { "blargg_apu_2005.07.30/04.clock_jitter.nes", MN_TV_NTSC, { { 0x2886AFFC, 15, 0x00, 0 } } }, //
  { "blargg_apu_2005.07.30/05.len_timing_mode0.nes", MN_TV_NTSC, { { 0x2886AFFC, 21, 0x00, 0 } } }, //
  { "blargg_apu_2005.07.30/06.len_timing_mode1.nes", MN_TV_NTSC, { { 0x2886AFFC, 23, 0x00, 0 } } }, //
  { "blargg_apu_2005.07.30/07.irq_flag_timing.nes", MN_TV_NTSC, { { 0x2886AFFC, 17, 0x00, 0 } } }, //
  { "blargg_apu_2005.07.30/08.irq_timing.nes", MN_TV_NTSC, { { 0x2886AFFC, 15, 0x00, 0 } } }, //
  { "blargg_apu_2005.07.30/09.reset_timing.nes", MN_TV_NTSC, { { 0x2886AFFC, 11, 0x00, 0 } } }, //
  { "blargg_apu_2005.07.30/10.len_halt_timing.nes", MN_TV_NTSC, { { 0x2886AFFC, 15, 0x00, 0 } } }, //
  { "blargg_apu_2005.07.30/11.len_reload_timing.nes", MN_TV_NTSC, { { 0x2886AFFC, 15, 0x00, 0 } } }, //
  { "blargg_nes_cpu_test5/cpu.nes", MN_TV_NTSC, { { 0x80ED81E0, 981, 0, 0 } } }, //
  { "blargg_nes_cpu_test5/official.nes", MN_TV_NTSC, { { 0x80ED81E0, 643, 0, 0 } } }, //
  { "blargg_ppu_tests_2005.09.15b/palette_ram.nes", MN_TV_NTSC, { { 0x2886AFFC, 18, 0x00, 0 } } }, //
  { "blargg_ppu_tests_2005.09.15b/sprite_ram.nes", MN_TV_NTSC, { { 0x2886AFFC, 18, 0x00, 0 } } }, //
  { "blargg_ppu_tests_2005.09.15b/vbl_clear_time.nes", MN_TV_NTSC, { { 0x2886AFFC, 23, 0x00, 0 } } }, //
  { "blargg_ppu_tests_2005.09.15b/vram_access.nes", MN_TV_NTSC, { { 0x2886AFFC, 18, 0x00, 0 } } }, //
  { "branch_timing_tests/1.Branch_Basics.nes", MN_TV_NTSC, { { 0xB6A4872B, 13, 0x00, 0 } } }, //
  { "branch_timing_tests/2.Backward_Branch.nes", MN_TV_NTSC, { { 0x36D590FE, 15, 0x00, 0 } } }, //
  { "branch_timing_tests/3.Forward_Branch.nes", MN_TV_NTSC, { { 0x6D0B3B45, 15, 0x00, 0 } } }, //
  { "cpu_dummy_reads/cpu_dummy_reads.nes", MN_TV_NTSC, { { 0xA942A3CF, 47, 0x00, 0 } } }, //
  { "cpu_dummy_writes/cpu_dummy_writes_oam.nes", MN_TV_NTSC, { { 0x5A174153, 331, 0x00, 0 } } }, //
  { "cpu_dummy_writes/cpu_dummy_writes_ppumem.nes", MN_TV_NTSC, { { 0xB9CCCCC0, 236, 0x00, 0 } } }, //
  { "cpu_exec_space/test_cpu_exec_space_apu.nes", MN_TV_NTSC, { { 0xACA38C28, 301, 0x00, 0 } } }, //
  { "cpu_exec_space/test_cpu_exec_space_ppuio.nes", MN_TV_NTSC, { { 0xB1E7AB62, 45, 0x00, 0 } } }, //
  { "cpu_interrupts_v2/rom_singles/1-cli_latency.nes", MN_TV_NTSC, { { 0x04E43C3C, 14, 0x00, 0 } } }, //
  { "cpu_interrupts_v2/rom_singles/2-nmi_and_brk.nes", MN_TV_NTSC, { { 0xEA3D7CA3, 105, 0x00, 0 } } }, //
  { "cpu_interrupts_v2/rom_singles/3-nmi_and_irq.nes", MN_TV_NTSC, { { 0xB47C1A9A, 125, 0x00, 0 } } }, //
  { "cpu_interrupts_v2/rom_singles/4-irq_and_dma.nes", MN_TV_NTSC, { { 0x62782874, 68, 0x00, 0 } } }, //
  { "cpu_interrupts_v2/rom_singles/5-branch_delays_irq.nes", MN_TV_NTSC, { { 0x709B36CC, 383, 0x00, 0 } } }, //
  { "cpu_interrupts_v2/cpu_interrupts.nes", MN_TV_NTSC, { { 0x12619948, 725, 0, 0 } } }, //
  { "cpu_reset/ram_after_reset.nes",
    MN_TV_NTSC,
    { { 0x5B69B3BD, 100, 0, 0 }, { 0x07673647, 50, 0, 0 }, { 0x68B7E9F1, 20, 0, 1 } } }, //
  { "cpu_reset/registers.nes",
    MN_TV_NTSC,
    { { 0x3235473C, 100, 0, 0 }, { 0x07673647, 50, 0, 0 }, { 0x8233D68A, 20, 0, 1 } } },
  { "cpu_timing_test6/cpu_timing_test.nes", MN_TV_NTSC, { { 0x3751F886, 613, 0, 0 } } }, //
  { "dmc_dma_during_read4/dma_2007_read.nes", MN_TV_NTSC, { { 0x8D3CE006, 23, 0x00, 0 } } }, //
  { "dmc_dma_during_read4/dma_2007_write.nes", MN_TV_NTSC, { { 0x94175082, 28, 0x00, 0 } } }, //
  { "dmc_dma_during_read4/dma_4016_read.nes", MN_TV_NTSC, { { 0x441C2F34, 16, 0x00, 0 } } }, //
  { "dmc_dma_during_read4/double_2007_read.nes", MN_TV_NTSC, { { 0x80B4A112, 16, 0x00, 0 } } }, //
  { "dmc_dma_during_read4/read_write_2007.nes", MN_TV_NTSC, { { 0xEB956420, 18, 0x00, 0 } } }, //
  // exram/mmc5exram.nes // todo
  { "dpcmletterbox/dpcmletterbox.nes", MN_TV_NTSC, { { 0x4C78B1D5, 10, 0x00, 0 }, { 0x649F38C9, 216, 0xA0, 0 } } },
  //
  { "full_palette/flowing_palette.nes", MN_TV_NTSC, { { 0xA23B43F2, 30, 0, 0 } } }, //
  { "full_palette/full_palette_smooth.nes", MN_TV_NTSC, { { 0xB9E4C8FC, 30, 0, 0 } } }, //
  { "full_palette/full_palette.nes", MN_TV_NTSC, { { 0xA6FC042E, 30, 0, 0 } } }, //
  { "instr_misc/rom_singles/01-abs_x_wrap.nes", MN_TV_NTSC, { { 0x290CAB05, 11, 0x00, 0 } } }, //
  { "instr_misc/rom_singles/02-branch_wrap.nes", MN_TV_NTSC, { { 0x54041E4A, 12, 0x00, 0 } } }, //
  { "instr_misc/rom_singles/03-dummy_reads.nes", MN_TV_NTSC, { { 0x4D1E0C1E, 56, 0x00, 0 } } }, //
  { "instr_misc/rom_singles/04-dummy_reads_apu.nes", MN_TV_NTSC, { { 0x8808910F, 140, 0x00, 0 } } }, //
  { "instr_misc/instr_misc.nes", MN_TV_NTSC, { { 0x0AE0BB6B, 226, 0, 0 } } }, //
  { "instr_test-v3/rom_singles/01-implied.nes", MN_TV_NTSC, { { 0x0F76F905, 97, 0x00, 0 } } }, //
  { "instr_test-v3/rom_singles/02-immediate.nes", MN_TV_NTSC, { { 0x60452B1E, 87, 0x00, 0 } } }, //
  { "instr_test-v3/rom_singles/03-zero_page.nes", MN_TV_NTSC, { { 0x75E5E809, 116, 0x00, 0 } } }, //
  { "instr_test-v3/rom_singles/04-zp_xy.nes", MN_TV_NTSC, { { 0x2D7C7D05, 260, 0x00, 0 } } }, //
  { "instr_test-v3/rom_singles/05-absolute.nes", MN_TV_NTSC, { { 0x0E0A1490, 110, 0x00, 0 } } }, //
  { "instr_test-v3/rom_singles/06-abs_xy.nes", MN_TV_NTSC, { { 0x393F728D, 365, 0x00, 0 } } }, //
  { "instr_test-v3/rom_singles/07-ind_x.nes", MN_TV_NTSC, { { 0x8A0D9C19, 146, 0x00, 0 } } }, //
  { "instr_test-v3/rom_singles/08-ind_y.nes", MN_TV_NTSC, { { 0x28E6D4DE, 138, 0x00, 0 } } }, //
  { "instr_test-v3/rom_singles/09-branches.nes", MN_TV_NTSC, { { 0xB23466FB, 43, 0x00, 0 } } }, //
  { "instr_test-v3/rom_singles/10-stack.nes", MN_TV_NTSC, { { 0x17116DAB, 164, 0x00, 0 } } }, //
  { "instr_test-v3/rom_singles/11-jmp_jsr.nes", MN_TV_NTSC, { { 0x21549A14, 17, 0x00, 0 } } }, //
  { "instr_test-v3/rom_singles/12-rts.nes", MN_TV_NTSC, { { 0xF10F555D, 14, 0x00, 0 } } }, //
  { "instr_test-v3/rom_singles/13-rti.nes", MN_TV_NTSC, { { 0x935AD5B5, 14, 0x00, 0 } } }, //
  { "instr_test-v3/rom_singles/14-brk.nes", MN_TV_NTSC, { { 0xDE89D3A9, 25, 0x00, 0 } } }, //
  { "instr_test-v3/rom_singles/15-special.nes", MN_TV_NTSC, { { 0x49A4C0BC, 11, 0x00, 0 } } }, //
  { "instr_test-v3/all_instrs.nes", MN_TV_NTSC, { { 0x8EEA7B5E, 2330, 0x00, 0 } } }, //
  { "instr_test-v3/official_only.nes", MN_TV_NTSC, { { 0x8EEA7B5E, 1812, 0x00, 0 } } }, //
  { "instr_test-v5/rom_singles/01-basics.nes", MN_TV_NTSC, { { 0xE654945D, 17, 0x00, 0 } } }, //
  { "instr_test-v5/rom_singles/02-implied.nes", MN_TV_NTSC, { { 0xB2AF2680, 97, 0x00, 0 } } }, //
  { "instr_test-v5/rom_singles/03-immediate.nes", MN_TV_NTSC, { { 0x8CE0845F, 87, 0x00, 0 } } }, //
  { "instr_test-v5/rom_singles/04-zero_page.nes", MN_TV_NTSC, { { 0xE4C3F180, 116, 0x00, 0 } } }, //
  { "instr_test-v5/rom_singles/05-zp_xy.nes", MN_TV_NTSC, { { 0x9EDDA0AE, 260, 0x00, 0 } } }, //
  { "instr_test-v5/rom_singles/06-absolute.nes", MN_TV_NTSC, { { 0x5DFCDDD1, 110, 0x00, 0 } } }, //
  { "instr_test-v5/rom_singles/07-abs_xy.nes", MN_TV_NTSC, { { 0x0067CDD4, 365, 0x00, 0 } } }, //
  { "instr_test-v5/rom_singles/08-ind_x.nes", MN_TV_NTSC, { { 0x17036DB5, 147, 0x00, 0 } } }, //
  { "instr_test-v5/rom_singles/09-ind_y.nes", MN_TV_NTSC, { { 0x9313623A, 137, 0x00, 0 } } }, //
  { "instr_test-v5/rom_singles/10-branches.nes", MN_TV_NTSC, { { 0x43645E68, 41, 0x00, 0 } } }, //
  { "instr_test-v5/rom_singles/11-stack.nes", MN_TV_NTSC, { { 0xB408FAE0, 164, 0x00, 0 } } }, //
  { "instr_test-v5/rom_singles/12-jmp_jsr.nes", MN_TV_NTSC, { { 0x9C8D4591, 18, 0x00, 0 } } }, //
  { "instr_test-v5/rom_singles/13-rts.nes", MN_TV_NTSC, { { 0x1DAAFA1C, 14, 0x00, 0 } } }, //
  { "instr_test-v5/rom_singles/14-rti.nes", MN_TV_NTSC, { { 0x027CCC3C, 14, 0x00, 0 } } }, //
  { "instr_test-v5/rom_singles/15-brk.nes", MN_TV_NTSC, { { 0x6D280E02, 26, 0x00, 0 } } }, //
  { "instr_test-v5/rom_singles/16-special.nes", MN_TV_NTSC, { { 0x1A5209FD, 13, 0x00, 0 } } }, //
  { "instr_test-v5/all_instrs.nes", MN_TV_NTSC, { { 0xB619C5B3, 2398, 0, 0 } } }, //
  { "instr_test-v5/official_only.nes", MN_TV_NTSC, { { 0xB619C5B3, 1871, 0x00, 0 } } }, //
  { "instr_timing/rom_singles/1-instr_timing.nes", MN_TV_NTSC, { { 0xE82C7F11, 1015, 0x00, 0 } } }, //
  { "instr_timing/rom_singles/2-branch_timing.nes", MN_TV_NTSC, { { 0xB8E3CDB8, 140, 0x00, 0 } } }, //
  { "instr_timing/instr_timing.nes", MN_TV_NTSC, { { 0x6F1BAA40, 1302, 0, 0 } } }, //
  { "MMC1_A12/mmc1_a12.nes", MN_TV_NTSC, { { 0xD19AE5E0, 27, 0x00, 0 } } }, //
  { "mmc3_irq_tests/1.Clocking.nes", MN_TV_NTSC, { { 0xB732D83F, 18, 0x00, 0 } } }, //
  { "mmc3_irq_tests/2.Details.nes", MN_TV_NTSC, { { 0x3E64BDBE, 23, 0x00, 0 } } }, //
  { "mmc3_irq_tests/3.A12_clocking.nes", MN_TV_NTSC, { { 0x9A45FD7C, 18, 0x00, 0 } } }, //
  { "mmc3_irq_tests/4.Scanline_timing.nes", MN_TV_NTSC, { { 0xA6F1F0EA, 79, 0x00, 0 } } }, //
  { "mmc3_irq_tests/5.MMC3_rev_A_m12.nes", MN_TV_NTSC, { { 0x022863D4, 18, 0x00, 0 } } }, //
  { "mmc3_irq_tests/6.MMC3_rev_B.nes", MN_TV_NTSC, { { 0x6DBFFE7B, 18, 0x00, 0 } } }, //
  { "mmc3_test/1-clocking.nes", MN_TV_NTSC, { { 0x3D4C2E04, 24, 0x00, 0 } } }, //
  { "mmc3_test/2-details.nes", MN_TV_NTSC, { { 0x390FC588, 26, 0x00, 0 } } }, //
  { "mmc3_test/3-A12_clocking.nes", MN_TV_NTSC, { { 0xD3189FEE, 24, 0x00, 0 } } }, //
  { "mmc3_test/4-scanline_timing.nes", MN_TV_NTSC, { { 0xF6D14A7E, 315, 0x00, 0 } } }, //
  { "mmc3_test/5-MMC3.nes", MN_TV_NTSC, { { 0xFAF35560, 23, 0x00, 0 } } }, //
  { "mmc3_test/6-MMC6_m12.nes", MN_TV_NTSC, { { 0xAD40DA76, 23, 0x00, 0 } } }, //
  { "mmc3_test_2/rom_singles/1-clocking.nes", MN_TV_NTSC, { { 0x3D4C2E04, 24, 0x00, 0 } } }, //
  { "mmc3_test_2/rom_singles/2-details.nes", MN_TV_NTSC, { { 0x390FC588, 26, 0x00, 0 } } }, //
  { "mmc3_test_2/rom_singles/3-A12_clocking.nes", MN_TV_NTSC, { { 0xD3189FEE, 24, 0x00, 0 } } }, //
  { "mmc3_test_2/rom_singles/4-scanline_timing.nes", MN_TV_NTSC, { { 0xF6D14A7E, 319, 0x00, 0 } } }, //
  { "mmc3_test_2/rom_singles/5-MMC3.nes", MN_TV_NTSC, { { 0xFAF35560, 25, 0x00, 0 } } }, //
  { "mmc3_test_2/rom_singles/6-MMC3_alt_m12.nes", MN_TV_NTSC, { { 0x09093FB9, 24, 0x00, 0 } } }, //
  // { "mmc5test_v2/mmc5test.nes", MN_TV_NTSC, {} }, // todo
  { "nes_instr_test/rom_singles/01-implied.nes", MN_TV_NTSC, { { 0x0F76F905, 63, 0x00, 0 } } }, //
  { "nes_instr_test/rom_singles/02-immediate.nes", MN_TV_NTSC, { { 0x60452B1E, 57, 0x00, 0 } } }, //
  { "nes_instr_test/rom_singles/03-zero_page.nes", MN_TV_NTSC, { { 0x75E5E809, 75, 0x00, 0 } } }, //
  { "nes_instr_test/rom_singles/04-zp_xy.nes", MN_TV_NTSC, { { 0x2D7C7D05, 174, 0x00, 0 } } }, //
  { "nes_instr_test/rom_singles/05-absolute.nes", MN_TV_NTSC, { { 0x0E0A1490, 71, 0x00, 0 } } }, //
  { "nes_instr_test/rom_singles/06-abs_xy.nes", MN_TV_NTSC, { { 0x393F728D, 244, 0x00, 0 } } }, //
  { "nes_instr_test/rom_singles/07-ind_x.nes", MN_TV_NTSC, { { 0x8A0D9C19, 104, 0x00, 0 } } }, //
  { "nes_instr_test/rom_singles/08-ind_y.nes", MN_TV_NTSC, { { 0x28E6D4DE, 98, 0x00, 0 } } }, //
  { "nes_instr_test/rom_singles/09-branches.nes", MN_TV_NTSC, { { 0xB23466FB, 32, 0x00, 0 } } }, //
  { "nes_instr_test/rom_singles/10-stack.nes", MN_TV_NTSC, { { 0x17116DAB, 128, 0x00, 0 } } }, //
  { "nes_instr_test/rom_singles/11-special.nes", MN_TV_NTSC, { { 0x3A5F745A, 10, 0x00, 0 } } }, //
  { "nmi_sync/demo_ntsc.nes",
    MN_TV_NTSC,
    { { 0xB9F1DFBA, 29, 0, 0 }, { 0x1D8EF9BE, 1, 0, 0 }, { 0xB9F1DFBA, 1, 0, 0 }, { 0x1D8EF9BE, 1, 0, 0 } } }, //
  { "nmi_sync/demo_pal.nes",
    MN_TV_PAL,
    { { 0x1E9B3479, 45, 0, 0 }, { 0x644FDA4C, 1, 0, 0 }, { 0x1E9B3479, 1, 0, 0 }, { 0x644FDA4C, 1, 0, 0 } } }, //
  { "oam_read/oam_read.nes", MN_TV_NTSC, { { 0xA35A57E7, 32, 0x00, 0 } } }, //
  { "oam_stress/oam_stress.nes", MN_TV_NTSC, { { 0x596023FD, 1703, 0x00, 0 } } }, //
  { "other/midscanline.nes", MN_TV_NTSC, { { 0x80BE54D6, 20, 0x00, 0 } } }, //
  { "other/nestest.nes",
    MN_TV_NTSC,
    { { 0x4E93D183, 10, 0, 0 },
      { 0x99F7E65B, 20, 0x08, 0 },
      { 0xCA5865AF, 10, 0x04, 0 },
      { 0xB2B3DDF1, 20, 0x08, 0 } } }, //
  { "other/oam3.nes", MN_TV_NTSC, { { 0xA87189F4, 10, 0x00, 0 } } }, //
  { "other/read2004.nes", MN_TV_NTSC, { { 0x519A9E5E, 20, 0x00, 0 } } }, //
  { "pal_apu_tests/01.len_ctr.nes", MN_TV_PAL, { { 0x2660164C, 20, 0x00, 0 } } }, //
  { "pal_apu_tests/02.len_table.nes", MN_TV_PAL, { { 0x82A8B32F, 20, 0x00, 0 } } }, //
  { "pal_apu_tests/03.irq_flag.nes", MN_TV_PAL, { { 0x71517FA8, 20, 0x00, 0 } } }, //
  { "pal_apu_tests/04.clock_jitter.nes", MN_TV_PAL, { { 0x08DFB8EA, 20, 0x00, 0 } } }, //
  { "pal_apu_tests/05.len_timing_mode0.nes", MN_TV_PAL, { { 0x01B854F7, 20, 0x00, 0 } } }, //
  { "pal_apu_tests/06.len_timing_mode1.nes", MN_TV_PAL, { { 0xB4349AE4, 20, 0x00, 0 } } }, //
  { "pal_apu_tests/07.irq_flag_timing.nes", MN_TV_PAL, { { 0x09F210C6, 20, 0x00, 0 } } }, //
  { "pal_apu_tests/08.irq_timing.nes", MN_TV_PAL, { { 0x8F29AE80, 20, 0x00, 0 } } }, //
  { "pal_apu_tests/10.len_halt_timing.nes", MN_TV_PAL, { { 0xA17D33DC, 20, 0x00, 0 } } }, //
  { "pal_apu_tests/11.len_reload_timing.nes", MN_TV_PAL, { { 0xD224579C, 20, 0x00, 0 } } }, //
  { "ppu_open_bus/ppu_open_bus.nes", MN_TV_NTSC, { { 0x3EB4ABE5, 250, 0x00, 0 } } }, //
  { "ppu_read_buffer/test_ppu_read_buffer.nes", MN_TV_NTSC, { { 0x7239FBBE, 1307, 0x00, 0 } } }, //
  { "ppu_sprite_hit/rom_singles/01-basics.nes", MN_TV_NTSC, { { 0xE654945D, 43, 0x00, 0 } } }, //
  { "ppu_sprite_hit/rom_singles/02-alignment.nes", MN_TV_NTSC, { { 0x057D9670, 40, 0x00, 0 } } }, //
  { "ppu_sprite_hit/rom_singles/03-corners.nes", MN_TV_NTSC, { { 0xEFCA28B7, 25, 0x00, 0 } } }, //
  { "ppu_sprite_hit/rom_singles/04-flip.nes", MN_TV_NTSC, { { 0xB297CC81, 21, 0x00, 0 } } }, //
  { "ppu_sprite_hit/rom_singles/05-left_clip.nes", MN_TV_NTSC, { { 0x9DF4E002, 38, 0x00, 0 } } }, //
  { "ppu_sprite_hit/rom_singles/06-right_edge.nes", MN_TV_NTSC, { { 0x601652B5, 28, 0x00, 0 } } }, //
  { "ppu_sprite_hit/rom_singles/07-screen_bottom.nes", MN_TV_NTSC, { { 0x50189868, 33, 0x00, 0 } } }, //
  { "ppu_sprite_hit/rom_singles/08-double_height.nes", MN_TV_NTSC, { { 0x53286670, 24, 0x00, 0 } } }, //
  { "ppu_sprite_hit/rom_singles/09-timing.nes", MN_TV_NTSC, { { 0x00CC842A, 177, 0x00, 0 } } }, //
  { "ppu_sprite_hit/rom_singles/10-timing_order.nes", MN_TV_NTSC, { { 0x94A60512, 69, 0x00, 0 } } }, //
  { "ppu_sprite_hit/ppu_sprite_hit.nes", MN_TV_NTSC, { { 0xDDE50B5E, 584, 0x00, 0 } } }, //
  { "ppu_sprite_overflow/rom_singles/01-basics.nes", MN_TV_NTSC, { { 0xE654945D, 25, 0x00, 0 } } }, //
  { "ppu_sprite_overflow/rom_singles/02-details.nes", MN_TV_NTSC, { { 0x7111D067, 30, 0x00, 0 } } }, //
  { "ppu_sprite_overflow/rom_singles/03-timing.nes", MN_TV_NTSC, { { 0x6E1E7158, 316, 0x00, 0 } } }, //
  { "ppu_sprite_overflow/rom_singles/04-obscure.nes", MN_TV_NTSC, { { 0xCE39FCC6, 26, 0x00, 0 } } }, //
  { "ppu_sprite_overflow/rom_singles/05-emulator.nes", MN_TV_NTSC, { { 0x5E275C0F, 19, 0x00, 0 } } }, //
  { "ppu_sprite_overflow/ppu_sprite_overflow.nes", MN_TV_NTSC, { { 0x12619948, 457, 0x00, 0 } } }, //
  { "ppu_vbl_nmi/rom_singles/01-vbl_basics.nes", MN_TV_NTSC, { { 0x1AF3E9A3, 140, 0x00, 0 } } }, //
  { "ppu_vbl_nmi/rom_singles/02-vbl_set_time.nes", MN_TV_NTSC, { { 0x59B9C83E, 178, 0x00, 0 } } }, //
  { "ppu_vbl_nmi/rom_singles/03-vbl_clear_time.nes", MN_TV_NTSC, { { 0x7B0B8BE1, 169, 0x00, 0 } } }, //
  { "ppu_vbl_nmi/rom_singles/04-nmi_control.nes", MN_TV_NTSC, { { 0xCD67F9C1, 31, 0x00, 0 } } }, //
  { "ppu_vbl_nmi/rom_singles/05-nmi_timing.nes", MN_TV_NTSC, { { 0xE19A001A, 219, 0x00, 0 } } }, //
  { "ppu_vbl_nmi/rom_singles/06-suppression.nes", MN_TV_NTSC, { { 0x4D1533CC, 222, 0x00, 0 } } }, //
  { "ppu_vbl_nmi/rom_singles/07-nmi_on_timing.nes", MN_TV_NTSC, { { 0x1658D4F5, 197, 0x00, 0 } } }, //
  { "ppu_vbl_nmi/rom_singles/08-nmi_off_timing.nes", MN_TV_NTSC, { { 0xA3F47CBF, 221, 0x00, 0 } } }, //
  { "ppu_vbl_nmi/rom_singles/09-even_odd_frames.nes", MN_TV_NTSC, { { 0xA590A81C, 76, 0x00, 0 } } }, //
  { "ppu_vbl_nmi/rom_singles/10-even_odd_timing.nes", MN_TV_NTSC, { { 0xB3F44119, 142, 0x00, 0 } } }, //
  { "ppu_vbl_nmi/ppu_vbl_nmi.nes", MN_TV_NTSC, { { 0xDDE50B5E, 1616, 0, 0 } } }, //
  { "read_joy3/count_errors_fast.nes", MN_TV_NTSC, { { 0xE8E7148C, 35, 0x00, 0 } } }, //
  { "read_joy3/count_errors.nes", MN_TV_NTSC, { { 0x2B5D8852, 59, 0x00, 0 } } }, //
  { "read_joy3/test_buttons.nes",
    MN_TV_NTSC,
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
  { "read_joy3/thorough_test.nes", MN_TV_NTSC, { { 0x6ECF1F4B, 123, 0x00, 0 } } }, //
  { "scanline/scanline.nes", MN_TV_NTSC, { { 0x629D1349, 80, 0, 0 } } }, // area1 flickering pixels after line
  { "scanline-a1/scanline.nes", MN_TV_NTSC, { { 0x629D1349, 80, 0, 0 } } }, // same as above
  { "scrolltest/scroll.nes", MN_TV_NTSC, { { 0x95F078BC, 7, 0, 0 }, { 0xF0427789, 60, 0xA0, 0 } } }, //
  { "sprdma_and_dmc_dma/sprdma_and_dmc_dma.nes", MN_TV_NTSC, { { 0xF4998922, 139, 0x00, 0 } } }, //
  { "sprdma_and_dmc_dma/sprdma_and_dmc_dma_512.nes", MN_TV_NTSC, { { 0x42149129, 143, 0x00, 0 } } }, //
  { "sprite_hit_tests_2005.10.05/01.basics.nes", MN_TV_NTSC, { { 0xAC38F2AD, 33, 0x00, 0 } } }, //
  { "sprite_hit_tests_2005.10.05/02.alignment.nes", MN_TV_NTSC, { { 0x3BF08842, 31, 0x00, 0 } } }, //
  { "sprite_hit_tests_2005.10.05/03.corners.nes", MN_TV_NTSC, { { 0xC53BD4BF, 20, 0x00, 0 } } }, //
  { "sprite_hit_tests_2005.10.05/04.flip.nes", MN_TV_NTSC, { { 0x0B05D9E8, 19, 0x00, 0 } } }, //
  { "sprite_hit_tests_2005.10.05/05.left_clip.nes", MN_TV_NTSC, { { 0x95D74663, 30, 0x00, 0 } } }, //
  { "sprite_hit_tests_2005.10.05/06.right_edge.nes", MN_TV_NTSC, { { 0x420C5953, 23, 0x00, 0 } } }, //
  { "sprite_hit_tests_2005.10.05/07.screen_bottom.nes", MN_TV_NTSC, { { 0xCE4105CF, 24, 0x00, 0 } } }, //
  { "sprite_hit_tests_2005.10.05/08.double_height.nes", MN_TV_NTSC, { { 0xCE51B630, 20, 0x00, 0 } } }, //
  { "sprite_hit_tests_2005.10.05/09.timing_basics.nes", MN_TV_NTSC, { { 0xE68A1C8B, 72, 0x00, 0 } } }, //
  { "sprite_hit_tests_2005.10.05/10.timing_order.nes", MN_TV_NTSC, { { 0xEBEBD82C, 72, 0x00, 0 } } }, //
  { "sprite_hit_tests_2005.10.05/11.edge_timing.nes", MN_TV_NTSC, { { 0x055D6A29, 47, 0x00, 0 } } }, //
  { "sprite_overflow_tests/1.Basics.nes", MN_TV_NTSC, { { 0x03CC909D, 15, 0x00, 0 } } }, //
  { "sprite_overflow_tests/2.Details.nes", MN_TV_NTSC, { { 0xC833B4E1, 24, 0x00, 0 } } }, //
  { "sprite_overflow_tests/3.Timing.nes", MN_TV_NTSC, { { 0x39154B5F, 128, 0x00, 0 } } }, //
  { "sprite_overflow_tests/4.Obscure.nes", MN_TV_NTSC, { { 0xE2E52CC7, 22, 0x00, 0 } } }, //
  { "sprite_overflow_tests/5.Emulator.nes", MN_TV_NTSC, { { 0xBAFE1B96, 13, 0x00, 0 } } }, //
  { "stomper/smwstomp.nes", MN_TV_NTSC, { { 0x01E1CE90, 244, 0, 0 } } }, //
  { "vbl_nmi_timing/1.frame_basics.nes", MN_TV_NTSC, { { 0x509B862D, 176, 0x00, 0 } } }, //
  { "vbl_nmi_timing/2.vbl_timing.nes", MN_TV_NTSC, { { 0xACDD6906, 154, 0x00, 0 } } }, //
  { "vbl_nmi_timing/3.even_odd_frames.nes", MN_TV_NTSC, { { 0xF2D9B6C9, 99, 0x00, 0 } } }, //
  { "vbl_nmi_timing/4.vbl_clear_timing.nes", MN_TV_NTSC, { { 0x3B42F8FA, 117, 0x00, 0 } } }, //
  { "vbl_nmi_timing/5.nmi_suppression.nes", MN_TV_NTSC, { { 0xB2955A03, 166, 0x00, 0 } } }, //
  { "vbl_nmi_timing/6.nmi_disable.nes", MN_TV_NTSC, { { 0xFD58C6AA, 109, 0x00, 0 } } }, //
  { "vbl_nmi_timing/7.nmi_timing.nes", MN_TV_NTSC, { { 0xE5553CE0, 109, 0x00, 0 } } }, //
};

uint calc_crc(const u8* data, uint size)
{
  uint crc = 0xFFFFFFFF;
  for (uint i = 0; i < size; ++i) {
    crc ^= data[i];
    for (uint j = 0; j < 8; j++) {
      crc = (crc >> 1) ^ ((crc & 1) ? 0xEDB88320 : 0);
    }
  }
  return crc ^ 0xFFFFFFFF;
}

void save_tga(const char* img_path, const void* pixels, uint width, uint height)
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
  uint img_path_len = strlen(img_path);
  for (uint i = 0; i <= img_path_len; i++) {
    if (img_path[i] == '/') {
      img_path[i] = 0;
      mkdir(img_path, 0777);
      img_path[i] = '/';
    }
  }
  save_tga(img_path, mn_output(), 256, 240);
}

uint run_test_step(TestStep step)
{
  if (step.reset) {
    mn_reset();
  }
  for (uint i = 0; i < step.frames; ++i) {
    mn_frame(step.joy1, 0);
  }
  return calc_crc((u8*)mn_output(), 256 * 240 * 4);
}

bool run_test_steps(TestParams params, uint* out_hash)
{
  Rom rom = mn_rom_load(params.path, nullptr);
  rom->tv = params.tv;
  mn_rom_set(rom);

  bool result = true;
  for (uint i = 0; i < 32; ++i) {
    if (i > 0 && params.steps[i].frames == 0) {
      break;
    }
    uint hash = run_test_step(params.steps[i]);
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

    uint frames_pass = params.steps[0].frames;
    uint frames_fail = 0;
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

    printf("  { \"%s\", %s, { { 0x%08X, %d, 0x%02X, %d } } }, //\n", params.path,
           params.tv == MN_TV_NTSC    ? "MN_TV_NTSC"
           : params.tv == MN_TV_PAL   ? "MN_TV_PAL"
           : params.tv == MN_TV_DENDY ? "MN_TV_DENDY"
                                      : "MN_TV_AUTO",
           params.steps[0].hash, params.steps[0].frames, params.steps[0].joy1, params.steps[0].reset);
  } else {
    uint hash = 0;
    result = run_test_steps(params, &hash);
    printf("%4s | %08X | %s\n", result ? "OK" : "FAIL", hash, params.path);
  }
  return result;
}

int main(int, char**)
{
  chdir("../../tmp/nes-test-roms");

  uint count = sizeof(tests) / sizeof(tests[0]);
  uint errors = 0;
  for (uint i = 0; i < count; ++i) {
    if (!run_test(tests[i])) {
      ++errors;
    }
  }
  printf("%4s | %-3d/%4d | TOTAL ERRORS\n", errors ? "FAIL" : "OK", errors, count);

  return 0;
}
