#include "myaownes.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <math.h>
#include <SDL3/SDL.h>

static const char* base_path = "";
static const char* out_path = "";

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
  uint joy1;
  bool reset;
} TestStep;

typedef struct
{
  const char* path;
  uint region;
  bool audio;
  bool apu_test_mode;
  TestStep steps[32];
} TestParams;

static TestParams tests[] = {
  { "APU/apu_mixer/dmc.nes", MN_REGION_NTSC, 1, 0, { { 0xA7D6BE06, 900, 0, 0 } } }, //
  { "APU/apu_mixer/noise.nes", MN_REGION_NTSC, 1, 0, { { 0xDEFCDB67, 1400, 0, 0 } } }, //
  { "APU/apu_mixer/square.nes", MN_REGION_NTSC, 1, 0, { { 0x5670E7C3, 1200, 0, 0 } } }, //
  { "APU/apu_mixer/triangle.nes", MN_REGION_NTSC, 1, 0, { { 0x66153440, 800, 0, 0 } } }, //
  { "APU/apu_phase_reset/apu_phase_reset.nes", MN_REGION_NTSC, 1, 0, { { 0x0B7574EF, 600, 0, 0 } } }, //
  { "APU/apu_register_activation_test_wip1/apu_register_activation_test.nes",
    MN_REGION_NTSC,
    0,
    0,
    { { 0xC1210397, 10, 0x00, 0 } } }, //
  { "APU/apu_reset/4015_cleared.nes", MN_REGION_NTSC, 0, 0, { { 0x03085F30, 20, 0, 0 }, { 0x68020FC4, 20, 0, 1 } } }, //
  { "APU/apu_reset/4017_timing.nes", MN_REGION_NTSC, 0, 0, { { 0x65F6087F, 20, 0, 0 }, { 0x92964251, 20, 0, 1 } } }, //
  { "APU/apu_reset/4017_written.nes",
    MN_REGION_NTSC,
    0,
    0,
    { { 0x03085F30, 20, 0, 0 }, { 0x8BECFD90, 20, 0, 1 }, { 0x2421E3A9, 20, 0, 1 } } }, //
  { "APU/apu_reset/irq_flag_cleared.nes",
    MN_REGION_NTSC,
    0,
    0,
    { { 0x03085F30, 20, 0, 0 }, { 0x6157F5A6, 20, 0, 1 } } },
  { "APU/apu_reset/len_ctrs_enabled.nes",
    MN_REGION_NTSC,
    0,
    0,
    { { 0x03085F30, 20, 0, 0 }, { 0xB62FEAE3, 20, 0, 1 } } },
  { "APU/apu_reset/works_immediately.nes",
    MN_REGION_NTSC,
    0,
    0,
    { { 0x03085F30, 20, 0, 0 }, { 0x0000839C, 20, 0, 1 } } },
  { "APU/apu_test/rom_singles/1-len_ctr.nes", MN_REGION_NTSC, 0, 0, { { 0x8E3C39E5, 19, 0x00, 0 } } }, //
  { "APU/apu_test/rom_singles/2-len_table.nes", MN_REGION_NTSC, 0, 0, { { 0x0D477786, 14, 0x00, 0 } } }, //
  { "APU/apu_test/rom_singles/3-irq_flag.nes", MN_REGION_NTSC, 0, 0, { { 0xBA4FC3FB, 18, 0x00, 0 } } }, //
  { "APU/apu_test/rom_singles/4-jitter.nes", MN_REGION_NTSC, 0, 0, { { 0x3BA3894E, 17, 0x00, 0 } } }, //
  { "APU/apu_test/rom_singles/5-len_timing.nes", MN_REGION_NTSC, 0, 0, { { 0x5B826EC5, 111, 0x00, 0 } } }, //
  { "APU/apu_test/rom_singles/6-irq_flag_timing.nes", MN_REGION_NTSC, 0, 0, { { 0x31D8322C, 20, 0x00, 0 } } }, //
  { "APU/apu_test/rom_singles/7-dmc_basics.nes", MN_REGION_NTSC, 0, 0, { { 0x96FCAE66, 25, 0x00, 0 } } }, //
  { "APU/apu_test/rom_singles/8-dmc_rates.nes", MN_REGION_NTSC, 0, 0, { { 0xF97AC6BC, 27, 0x00, 0 } } }, //
  { "APU/apu_test/apu_test.nes", MN_REGION_NTSC, 0, 0, { { 0x71B41136, 300, 0, 0 } } }, //
  { "APU/blargg_apu_2005.07.30/01.len_ctr.nes", MN_REGION_NTSC, 0, 0, { { 0x2886AFFC, 24, 0x00, 0 } } }, //
  { "APU/blargg_apu_2005.07.30/02.len_table.nes", MN_REGION_NTSC, 0, 0, { { 0x2886AFFC, 11, 0x00, 0 } } }, //
  { "APU/blargg_apu_2005.07.30/03.irq_flag.nes", MN_REGION_NTSC, 0, 0, { { 0x2886AFFC, 18, 0x00, 0 } } }, //
  { "APU/blargg_apu_2005.07.30/04.clock_jitter.nes", MN_REGION_NTSC, 0, 0, { { 0x2886AFFC, 15, 0x00, 0 } } }, //
  { "APU/blargg_apu_2005.07.30/05.len_timing_mode0.nes", MN_REGION_NTSC, 0, 0, { { 0x2886AFFC, 21, 0x00, 0 } } }, //
  { "APU/blargg_apu_2005.07.30/06.len_timing_mode1.nes", MN_REGION_NTSC, 0, 0, { { 0x2886AFFC, 23, 0x00, 0 } } }, //
  { "APU/blargg_apu_2005.07.30/07.irq_flag_timing.nes", MN_REGION_NTSC, 0, 0, { { 0x2886AFFC, 17, 0x00, 0 } } }, //
  { "APU/blargg_apu_2005.07.30/08.irq_timing.nes", MN_REGION_NTSC, 0, 0, { { 0x2886AFFC, 15, 0x00, 0 } } }, //
  { "APU/blargg_apu_2005.07.30/09.reset_timing.nes", MN_REGION_NTSC, 0, 0, { { 0x2886AFFC, 11, 0x00, 0 } } }, //
  { "APU/blargg_apu_2005.07.30/10.len_halt_timing.nes", MN_REGION_NTSC, 0, 0, { { 0x2886AFFC, 15, 0x00, 0 } } }, //
  { "APU/blargg_apu_2005.07.30/11.len_reload_timing.nes", MN_REGION_NTSC, 0, 0, { { 0x2886AFFC, 15, 0x00, 0 } } }, //
  { "APU/dmc_dma/dmc_dma_explicit_stop_test_wip1.nes", MN_REGION_NTSC, 0, 0, { { 0x2317DDB1, 10, 0x00, 0 } } }, //
  { "APU/dmc_dma/dmc_dma_implicit_stop_level_test_wip1.nes", MN_REGION_NTSC, 0, 1, { { 0xF27419F3, 10, 0x00, 0 } } }, //
  { "APU/dmc_dma/dmc_dma_implicit_stop_test_wip1.nes", MN_REGION_NTSC, 0, 0, { { 0xE2999877, 10, 0x00, 0 } } }, //
  { "APU/dmc_dma/dmc_dma_status_test_wip1.nes", MN_REGION_NTSC, 0, 0, { { 0x238BE815, 10, 0x00, 0 } } }, //
  { "APU/dmc_dma/DMC_IRQ.nes", MN_REGION_NTSC, 1, 0, { { 0x50E1386B, 100, 0x00, 0 } } }, //
  { "APU/dpcmletterbox/dpcmletterbox.nes",
    MN_REGION_NTSC,
    0,
    0,
    { { 0x4C78B1D5, 10, 0x00, 0 }, { 0x649F38C9, 216, 0xA0, 0 } } }, //
  { "APU/duty_cycles_test/duty_cycles_test_ntsc.nes",
    MN_REGION_NTSC,
    1,
    0,
    { { 0x473792C4, 20, 0, 0 },
      { 0x7828214F, 20, MN_INPUT_SELECT, 0 },
      { 0xFC6F7EC8, 20, 0, 0 },
      { 0x201CCDAF, 20, MN_INPUT_START, 0 },
      { 0x158503BB, 20, 0, 0 },
      { 0x1FAD5D4A, 20, MN_INPUT_B, 0 },
      { 0x3F49EA37, 20, 0, 0 },
      { 0x6AA54F0E, 20, MN_INPUT_A, 0 } } }, //
  { "APU/duty_cycles_test/duty_cycles_test_dendy.nes",
    MN_REGION_DENDY,
    1,
    0,
    { { 0x473792C4, 20, 0, 0 },
      { 0x7828214F, 20, MN_INPUT_SELECT, 0 },
      { 0x0D09641F, 20, 0, 0 },
      { 0xDC172485, 20, MN_INPUT_START, 0 },
      { 0x9C60E6C0, 20, 0, 0 },
      { 0x4DF7D0AB, 20, MN_INPUT_B, 0 },
      { 0xA5265E05, 20, 0, 0 },
      { 0xBAE1D0EB, 20, MN_INPUT_A, 0 } } }, //
  { "APU/pal_apu_tests/01.len_ctr.nes", MN_REGION_PAL, 0, 0, { { 0x2660164C, 20, 0x00, 0 } } }, //
  { "APU/pal_apu_tests/02.len_table.nes", MN_REGION_PAL, 0, 0, { { 0x82A8B32F, 20, 0x00, 0 } } }, //
  { "APU/pal_apu_tests/03.irq_flag.nes", MN_REGION_PAL, 0, 0, { { 0x71517FA8, 20, 0x00, 0 } } }, //
  { "APU/pal_apu_tests/04.clock_jitter.nes", MN_REGION_PAL, 0, 0, { { 0x08DFB8EA, 20, 0x00, 0 } } }, //
  { "APU/pal_apu_tests/05.len_timing_mode0.nes", MN_REGION_PAL, 0, 0, { { 0x01B854F7, 20, 0x00, 0 } } }, //
  { "APU/pal_apu_tests/06.len_timing_mode1.nes", MN_REGION_PAL, 0, 0, { { 0xB4349AE4, 20, 0x00, 0 } } }, //
  { "APU/pal_apu_tests/07.irq_flag_timing.nes", MN_REGION_PAL, 0, 0, { { 0x09F210C6, 20, 0x00, 0 } } }, //
  { "APU/pal_apu_tests/08.irq_timing.nes", MN_REGION_PAL, 0, 0, { { 0x8F29AE80, 20, 0x00, 0 } } }, //
  { "APU/pal_apu_tests/10.len_halt_timing.nes", MN_REGION_PAL, 0, 0, { { 0xA17D33DC, 20, 0x00, 0 } } }, //
  { "APU/pal_apu_tests/11.len_reload_timing.nes", MN_REGION_PAL, 0, 0, { { 0xD224579C, 20, 0x00, 0 } } }, //
  { "APU/square_timer_div2/square_timer_div2.nes", MN_REGION_NTSC, 1, 0, { { 0xDC603B1D, 200, 0, 0 } } }, //
  { "APU/test_apu_2/test_1.nes", MN_REGION_NTSC, 0, 0, { { 0xD2EB556B, 6, 0x00, 0 } } }, //
  { "APU/test_apu_2/test_2.nes", MN_REGION_NTSC, 0, 0, { { 0xD2EB556B, 6, 0x00, 0 } } }, //
  { "APU/test_apu_2/test_3.nes", MN_REGION_NTSC, 0, 0, { { 0xD2EB556B, 6, 0x00, 0 } } }, //
  { "APU/test_apu_2/test_4.nes", MN_REGION_NTSC, 0, 0, { { 0xD2EB556B, 6, 0x00, 0 } } }, //
  { "APU/test_apu_2/test_5.nes", MN_REGION_NTSC, 0, 0, { { 0xD2EB556B, 6, 0x00, 0 } } }, //
  { "APU/test_apu_2/test_6.nes", MN_REGION_NTSC, 0, 0, { { 0xD2EB556B, 7, 0x00, 0 } } }, //
  { "APU/test_apu_2/test_7.nes", MN_REGION_NTSC, 0, 0, { { 0xD2EB556B, 6, 0x00, 0 } } }, //
  { "APU/test_apu_2/test_8.nes", MN_REGION_NTSC, 0, 0, { { 0xD2EB556B, 6, 0x00, 0 } } }, //
  { "APU/test_apu_2/test_9.nes", MN_REGION_NTSC, 0, 0, { { 0xD2EB556B, 6, 0x00, 0 } } }, //
  { "APU/test_apu_2/test_10.nes", MN_REGION_NTSC, 0, 0, { { 0xD2EB556B, 7, 0x00, 0 } } }, //
  { "APU/test_apu_env/test_apu_env.nes", MN_REGION_NTSC, 1, 0, { { 0xD54E3ABF, 600, 0, 0 } } }, //
  { "APU/test_apu_m/test_9.nes", MN_REGION_NTSC, 0, 0, { { 0xD2EB556B, 6, 0x00, 0 } } }, //
  { "APU/test_apu_m/test_10.nes", MN_REGION_NTSC, 0, 0, { { 0xD2EB556B, 6, 0x00, 0 } } }, //
  { "APU/test_apu_m/test_11.nes", MN_REGION_NTSC, 0, 0, { { 0xD2EB556B, 8, 0x00, 0 } } }, //
  { "APU/test_apu_sweep/sweep_cutoff.nes", MN_REGION_NTSC, 1, 0, { { 0xFF012A10, 300, 0, 0 } } }, //
  { "APU/test_apu_sweep/sweep_sub.nes", MN_REGION_NTSC, 1, 0, { { 0x7DAFD50A, 300, 0, 0 } } }, //
  { "APU/test_apu_timers/dmc_pitch.nes", MN_REGION_NTSC, 1, 0, { { 0x43A348B5, 1800, 0, 0 } } }, //
  { "APU/test_apu_timers/noise_pitch.nes", MN_REGION_NTSC, 1, 0, { { 0x7EA89665, 200, 0, 0 } } }, //
  { "APU/test_apu_timers/square_pitch.nes", MN_REGION_NTSC, 1, 0, { { 0xC0D5F366, 200, 0, 0 } } }, //
  { "APU/test_apu_timers/triangle_pitch.nes", MN_REGION_NTSC, 1, 0, { { 0x505A1FFC, 200, 0, 0 } } }, //
  { "APU/test_tri_lin_ctr/lin_ctr.nes", MN_REGION_NTSC, 1, 0, { { 0x090B7261, 600, 0, 0 } } }, //
  { "APU/volume_tests/volumes.nes", MN_REGION_NTSC, 1, 0, { { 0x54CEA1EE, 10, 0, 0 }, { 0xDA52A8F5, 1000, 1, 0 } } }, //
  { "APU/4015_open_bus_test.nes", MN_REGION_NTSC, 0, 0, { { 0xF96B60A2, 5, 0x00, 0 } } }, //
  { "CPU/blargg_nes_cpu_test5/cpu.nes", MN_REGION_NTSC, 0, 0, { { 0x80ED81E0, 981, 0, 0 } } }, //
  { "CPU/blargg_nes_cpu_test5/official.nes", MN_REGION_NTSC, 0, 0, { { 0x80ED81E0, 643, 0, 0 } } }, //
  { "CPU/branch_timing_tests/1.Branch_Basics.nes", MN_REGION_NTSC, 0, 0, { { 0xB6A4872B, 13, 0x00, 0 } } }, //
  { "CPU/branch_timing_tests/2.Backward_Branch.nes", MN_REGION_NTSC, 0, 0, { { 0x36D590FE, 15, 0x00, 0 } } }, //
  { "CPU/branch_timing_tests/3.Forward_Branch.nes", MN_REGION_NTSC, 0, 0, { { 0x6D0B3B45, 15, 0x00, 0 } } }, //
  { "CPU/cpu_dummy_reads/cpu_dummy_reads.nes", MN_REGION_NTSC, 0, 0, { { 0xA942A3CF, 47, 0x00, 0 } } }, //
  { "CPU/cpu_dummy_writes/cpu_dummy_writes_oam.nes", MN_REGION_NTSC, 0, 0, { { 0x5A174153, 331, 0x00, 0 } } }, //
  { "CPU/cpu_dummy_writes/cpu_dummy_writes_ppumem.nes", MN_REGION_NTSC, 0, 0, { { 0xB9CCCCC0, 236, 0x00, 0 } } }, //
  { "CPU/cpu_exec_space/test_cpu_exec_space_apu.nes", MN_REGION_NTSC, 0, 0, { { 0xACA38C28, 301, 0x00, 0 } } }, //
  { "CPU/cpu_exec_space/test_cpu_exec_space_ppuio.nes", MN_REGION_NTSC, 0, 0, { { 0xB1E7AB62, 45, 0x00, 0 } } }, //
  { "CPU/cpu_flag_concurrency/test_cpu_flag_concurrency.nes", MN_REGION_NTSC, 0, 0, { { 0x55FE9C5D, 855, 0x00, 0 } } },
  { "CPU/cpu_interrupts_v2/rom_singles/1-cli_latency.nes", MN_REGION_NTSC, 0, 0, { { 0x04E43C3C, 14, 0x00, 0 } } }, //
  { "CPU/cpu_interrupts_v2/rom_singles/2-nmi_and_brk.nes", MN_REGION_NTSC, 0, 0, { { 0xEA3D7CA3, 105, 0x00, 0 } } }, //
  { "CPU/cpu_interrupts_v2/rom_singles/3-nmi_and_irq.nes", MN_REGION_NTSC, 0, 0, { { 0xB47C1A9A, 125, 0x00, 0 } } }, //
  { "CPU/cpu_interrupts_v2/rom_singles/4-irq_and_dma.nes", MN_REGION_NTSC, 0, 0, { { 0x62782874, 68, 0x00, 0 } } }, //
  { "CPU/cpu_interrupts_v2/rom_singles/5-branch_delays_irq.nes",
    MN_REGION_NTSC,
    0,
    0,
    { { 0x709B36CC, 383, 0x00, 0 } } },
  { "CPU/cpu_interrupts_v2/cpu_interrupts.nes", MN_REGION_NTSC, 0, 0, { { 0x12619948, 725, 0, 0 } } }, //
  { "CPU/cpu_reset/ram_after_reset.nes",
    MN_REGION_NTSC,
    0,
    0,
    { { 0x5B69B3BD, 100, 0, 0 }, { 0x07673647, 50, 0, 0 }, { 0x68B7E9F1, 20, 0, 1 } } }, //
  { "CPU/cpu_reset/registers.nes",
    MN_REGION_NTSC,
    0,
    0,
    { { 0x3235473C, 100, 0, 0 }, { 0x07673647, 50, 0, 0 }, { 0x8233D68A, 20, 0, 1 } } }, //
  { "CPU/cpu_timing_test6/cpu_timing_test.nes", MN_REGION_NTSC, 0, 0, { { 0x3751F886, 613, 0, 0 } } }, //
  { "CPU/dma_sync_test_v2/dma_sync_test.nes",
    MN_REGION_NTSC,
    0,
    0,
    { { 0x07673647, 20, 0, 0 }, { 0xFCE2DB72, 10, MN_INPUT_RIGHT, 0 } } }, //
  { "CPU/dma_sync_test/dma_sync_test_odd.nes", MN_REGION_NTSC, 0, 0, { { 0xFCE2DB72, 20, 0, 0 } } }, //
  { "CPU/dma_sync_test/dma_sync_test.nes",
    MN_REGION_NTSC,
    0,
    0,
    { { 0x07673647, 20, 0, 0 }, { 0xFCE2DB72, 10, MN_INPUT_RIGHT, 0 } } }, //
  { "CPU/instr_misc/rom_singles/01-abs_x_wrap.nes", MN_REGION_NTSC, 0, 0, { { 0x290CAB05, 11, 0x00, 0 } } }, //
  { "CPU/instr_misc/rom_singles/02-branch_wrap.nes", MN_REGION_NTSC, 0, 0, { { 0x54041E4A, 12, 0x00, 0 } } }, //
  { "CPU/instr_misc/rom_singles/03-dummy_reads.nes", MN_REGION_NTSC, 0, 0, { { 0x4D1E0C1E, 56, 0x00, 0 } } }, //
  { "CPU/instr_misc/rom_singles/04-dummy_reads_apu.nes", MN_REGION_NTSC, 0, 0, { { 0x8808910F, 140, 0x00, 0 } } }, //
  { "CPU/instr_misc/instr_misc.nes", MN_REGION_NTSC, 0, 0, { { 0x0AE0BB6B, 226, 0, 0 } } }, //
  { "CPU/instr_test-v3/rom_singles/01-implied.nes", MN_REGION_NTSC, 0, 0, { { 0x0F76F905, 97, 0x00, 0 } } }, //
  { "CPU/instr_test-v3/rom_singles/02-immediate.nes", MN_REGION_NTSC, 0, 0, { { 0x60452B1E, 87, 0x00, 0 } } }, //
  { "CPU/instr_test-v3/rom_singles/03-zero_page.nes", MN_REGION_NTSC, 0, 0, { { 0x75E5E809, 116, 0x00, 0 } } }, //
  { "CPU/instr_test-v3/rom_singles/04-zp_xy.nes", MN_REGION_NTSC, 0, 0, { { 0x2D7C7D05, 260, 0x00, 0 } } }, //
  { "CPU/instr_test-v3/rom_singles/05-absolute.nes", MN_REGION_NTSC, 0, 0, { { 0x0E0A1490, 110, 0x00, 0 } } }, //
  { "CPU/instr_test-v3/rom_singles/06-abs_xy.nes", MN_REGION_NTSC, 0, 0, { { 0x393F728D, 365, 0x00, 0 } } }, //
  { "CPU/instr_test-v3/rom_singles/07-ind_x.nes", MN_REGION_NTSC, 0, 0, { { 0x8A0D9C19, 146, 0x00, 0 } } }, //
  { "CPU/instr_test-v3/rom_singles/08-ind_y.nes", MN_REGION_NTSC, 0, 0, { { 0x28E6D4DE, 138, 0x00, 0 } } }, //
  { "CPU/instr_test-v3/rom_singles/09-branches.nes", MN_REGION_NTSC, 0, 0, { { 0xB23466FB, 43, 0x00, 0 } } }, //
  { "CPU/instr_test-v3/rom_singles/10-stack.nes", MN_REGION_NTSC, 0, 0, { { 0x17116DAB, 164, 0x00, 0 } } }, //
  { "CPU/instr_test-v3/rom_singles/11-jmp_jsr.nes", MN_REGION_NTSC, 0, 0, { { 0x21549A14, 17, 0x00, 0 } } }, //
  { "CPU/instr_test-v3/rom_singles/12-rts.nes", MN_REGION_NTSC, 0, 0, { { 0xF10F555D, 14, 0x00, 0 } } }, //
  { "CPU/instr_test-v3/rom_singles/13-rti.nes", MN_REGION_NTSC, 0, 0, { { 0x935AD5B5, 14, 0x00, 0 } } }, //
  { "CPU/instr_test-v3/rom_singles/14-brk.nes", MN_REGION_NTSC, 0, 0, { { 0xDE89D3A9, 25, 0x00, 0 } } }, //
  { "CPU/instr_test-v3/rom_singles/15-special.nes", MN_REGION_NTSC, 0, 0, { { 0x49A4C0BC, 11, 0x00, 0 } } }, //
  { "CPU/instr_test-v3/all_instrs.nes", MN_REGION_NTSC, 0, 0, { { 0x8EEA7B5E, 2330, 0x00, 0 } } }, //
  { "CPU/instr_test-v3/official_only.nes", MN_REGION_NTSC, 0, 0, { { 0x8EEA7B5E, 1812, 0x00, 0 } } }, //
  { "CPU/instr_test-v5/rom_singles/01-basics.nes", MN_REGION_NTSC, 0, 0, { { 0xE654945D, 17, 0x00, 0 } } }, //
  { "CPU/instr_test-v5/rom_singles/02-implied.nes", MN_REGION_NTSC, 0, 0, { { 0xB2AF2680, 97, 0x00, 0 } } }, //
  { "CPU/instr_test-v5/rom_singles/03-immediate.nes", MN_REGION_NTSC, 0, 0, { { 0x8CE0845F, 87, 0x00, 0 } } }, //
  { "CPU/instr_test-v5/rom_singles/04-zero_page.nes", MN_REGION_NTSC, 0, 0, { { 0xE4C3F180, 116, 0x00, 0 } } }, //
  { "CPU/instr_test-v5/rom_singles/05-zp_xy.nes", MN_REGION_NTSC, 0, 0, { { 0x9EDDA0AE, 260, 0x00, 0 } } }, //
  { "CPU/instr_test-v5/rom_singles/06-absolute.nes", MN_REGION_NTSC, 0, 0, { { 0x5DFCDDD1, 110, 0x00, 0 } } }, //
  { "CPU/instr_test-v5/rom_singles/07-abs_xy.nes", MN_REGION_NTSC, 0, 0, { { 0x0067CDD4, 365, 0x00, 0 } } }, //
  { "CPU/instr_test-v5/rom_singles/08-ind_x.nes", MN_REGION_NTSC, 0, 0, { { 0x17036DB5, 147, 0x00, 0 } } }, //
  { "CPU/instr_test-v5/rom_singles/09-ind_y.nes", MN_REGION_NTSC, 0, 0, { { 0x9313623A, 137, 0x00, 0 } } }, //
  { "CPU/instr_test-v5/rom_singles/10-branches.nes", MN_REGION_NTSC, 0, 0, { { 0x43645E68, 41, 0x00, 0 } } }, //
  { "CPU/instr_test-v5/rom_singles/11-stack.nes", MN_REGION_NTSC, 0, 0, { { 0xB408FAE0, 164, 0x00, 0 } } }, //
  { "CPU/instr_test-v5/rom_singles/12-jmp_jsr.nes", MN_REGION_NTSC, 0, 0, { { 0x9C8D4591, 18, 0x00, 0 } } }, //
  { "CPU/instr_test-v5/rom_singles/13-rts.nes", MN_REGION_NTSC, 0, 0, { { 0x1DAAFA1C, 14, 0x00, 0 } } }, //
  { "CPU/instr_test-v5/rom_singles/14-rti.nes", MN_REGION_NTSC, 0, 0, { { 0x027CCC3C, 14, 0x00, 0 } } }, //
  { "CPU/instr_test-v5/rom_singles/15-brk.nes", MN_REGION_NTSC, 0, 0, { { 0x6D280E02, 26, 0x00, 0 } } }, //
  { "CPU/instr_test-v5/rom_singles/16-special.nes", MN_REGION_NTSC, 0, 0, { { 0x1A5209FD, 13, 0x00, 0 } } }, //
  { "CPU/instr_test-v5/all_instrs.nes", MN_REGION_NTSC, 0, 0, { { 0xB619C5B3, 2398, 0, 0 } } }, //
  { "CPU/instr_test-v5/official_only.nes", MN_REGION_NTSC, 0, 0, { { 0xB619C5B3, 1871, 0x00, 0 } } }, //
  { "CPU/instr_timing/rom_singles/1-instr_timing.nes", MN_REGION_NTSC, 0, 0, { { 0xE82C7F11, 1015, 0x00, 0 } } }, //
  { "CPU/instr_timing/rom_singles/2-branch_timing.nes", MN_REGION_NTSC, 0, 0, { { 0xB8E3CDB8, 140, 0x00, 0 } } }, //
  { "CPU/instr_timing/instr_timing.nes", MN_REGION_NTSC, 0, 0, { { 0x6F1BAA40, 1302, 0, 0 } } }, //
  { "CPU/nes_instr_test/rom_singles/01-implied.nes", MN_REGION_NTSC, 0, 0, { { 0x0F76F905, 63, 0x00, 0 } } }, //
  { "CPU/nes_instr_test/rom_singles/02-immediate.nes", MN_REGION_NTSC, 0, 0, { { 0x60452B1E, 57, 0x00, 0 } } }, //
  { "CPU/nes_instr_test/rom_singles/03-zero_page.nes", MN_REGION_NTSC, 0, 0, { { 0x75E5E809, 75, 0x00, 0 } } }, //
  { "CPU/nes_instr_test/rom_singles/04-zp_xy.nes", MN_REGION_NTSC, 0, 0, { { 0x2D7C7D05, 174, 0x00, 0 } } }, //
  { "CPU/nes_instr_test/rom_singles/05-absolute.nes", MN_REGION_NTSC, 0, 0, { { 0x0E0A1490, 71, 0x00, 0 } } }, //
  { "CPU/nes_instr_test/rom_singles/06-abs_xy.nes", MN_REGION_NTSC, 0, 0, { { 0x393F728D, 244, 0x00, 0 } } }, //
  { "CPU/nes_instr_test/rom_singles/07-ind_x.nes", MN_REGION_NTSC, 0, 0, { { 0x8A0D9C19, 104, 0x00, 0 } } }, //
  { "CPU/nes_instr_test/rom_singles/08-ind_y.nes", MN_REGION_NTSC, 0, 0, { { 0x28E6D4DE, 98, 0x00, 0 } } }, //
  { "CPU/nes_instr_test/rom_singles/09-branches.nes", MN_REGION_NTSC, 0, 0, { { 0xB23466FB, 32, 0x00, 0 } } }, //
  { "CPU/nes_instr_test/rom_singles/10-stack.nes", MN_REGION_NTSC, 0, 0, { { 0x17116DAB, 128, 0x00, 0 } } }, //
  { "CPU/nes_instr_test/rom_singles/11-special.nes", MN_REGION_NTSC, 0, 0, { { 0x3A5F745A, 10, 0x00, 0 } } }, //
  { "CPU/nestest/nestest.nes",
    MN_REGION_NTSC,
    0,
    0,
    { { 0x4E93D183, 10, 0, 0 },
      { 0x99F7E65B, 20, 0x08, 0 },
      { 0xCA5865AF, 10, 0x04, 0 },
      { 0xB2B3DDF1, 20, 0x08, 0 } } }, //
  { "CPU/shxdma/shxdma.nes", MN_REGION_NTSC, 0, 0, { { 0xDED20D8F, 101, 0x00, 0 } } }, //
  { "CPU/shxing1/shxing1.nes", MN_REGION_NTSC, 0, 0, { { 0xA07B475E, 186, 0x00, 0 } } }, //
  { "CPU/shxing2/shxing2.nes", MN_REGION_NTSC, 0, 0, { { 0xA07B475E, 111, 0x00, 0 } } }, //
  { "CPU/sprdma_and_dmc_dma/sprdma_and_dmc_dma.nes", MN_REGION_NTSC, 0, 0, { { 0xF4998922, 143, 0x00, 0 } } }, //
  { "CPU/sprdma_and_dmc_dma/sprdma_and_dmc_dma_512.nes", MN_REGION_NTSC, 0, 0, { { 0x42149129, 143, 0x00, 0 } } }, //
  { "CPU/out_timing_test.nes",
    MN_REGION_NTSC,
    0,
    0,
    { { 0x64B962A8, 5, 0x00, 0 },
      { 0x64B962A8, 5, MN_INPUT_A, 0 },
      { 0x7C518553, 5, 0, 0 },
      { 0x7C518553, 5, MN_INPUT_B, 0 },
      { 0x7C518553, 5, 0, 0 } } }, //
  { "Demos/8bitpeoples_-_deadline_console_invitro.nes", MN_REGION_NTSC, 1, 0, { { 0xB4977AD9, 900, 0x00, 0 } } }, //
  { "Demos/midscanline.nes", MN_REGION_NTSC, 0, 0, { { 0x80BE54D6, 20, 0x00, 0 } } }, //
  { "Demos/scrolltest/scroll.nes", MN_REGION_NTSC, 0, 0, { { 0x95F078BC, 7, 0, 0 }, { 0xF0427789, 60, 0xA0, 0 } } }, //
  { "Demos/stomper/smwstomp.nes", MN_REGION_NTSC, 0, 0, { { 0x3E04F985, 244, 0, 0 } } }, //
  { "Input/ControllerStrobeTest.nes",
    MN_REGION_NTSC,
    0,
    0,
    { { 0x66C3A864, 15, 0, 0 }, { 0xFE658A55, 10, MN_INPUT_A, 0 }, { 0xFA310F74, 10, MN_INPUT_B, 0 } } }, //
  { "Input/read_joy3/count_errors_fast.nes", MN_REGION_NTSC, 0, 0, { { 0x86C178F5, 40, 0x00, 0 } } }, //
  { "Input/read_joy3/count_errors.nes", MN_REGION_NTSC, 0, 0, { { 0xDCC8122C, 59, 0x00, 0 } } }, //
  { "Input/read_joy3/test_buttons.nes",
    MN_REGION_NTSC,
    0,
    0,
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
  { "Input/read_joy3/thorough_test.nes", MN_REGION_NTSC, 0, 0, { { 0x6ECF1F4B, 123, 0x00, 0 } } }, //
  { "Interactive/AccuracyCoin.nes",
    MN_REGION_NTSC,
    0,
    0,
    { { 0x6121DCA1, 40, 0, 0 }, { 0x56FC8DFD, 2, MN_INPUT_START, 0 }, { 0xD8C4B0C4, 4200, 0, 0 } } }, //
  { "Mappers/2_test_src/2_test_0.nes", MN_REGION_NTSC, 0, 0, { { 0x017D7F4C, 46, 0x00, 0 } } }, //
  { "Mappers/2_test_src/2_test_1.nes", MN_REGION_NTSC, 0, 0, { { 0x4A859513, 46, 0x00, 0 } } }, //
  { "Mappers/2_test_src/2_test_2.nes", MN_REGION_NTSC, 0, 0, { { 0x017D7F4C, 46, 0x00, 0 } } }, //
  { "Mappers/3_test_src/3_test_0.nes", MN_REGION_NTSC, 0, 0, { { 0x414DEEF5, 44, 0x00, 0 } } }, //
  { "Mappers/3_test_src/3_test_1.nes", MN_REGION_NTSC, 0, 0, { { 0x3D7DCD16, 44, 0x00, 0 } } }, //
  { "Mappers/3_test_src/3_test_2.nes", MN_REGION_NTSC, 0, 0, { { 0x414DEEF5, 44, 0x00, 0 } } }, //
  { "Mappers/7_test_src/7_test_0.nes", MN_REGION_NTSC, 0, 0, { { 0x31643FF1, 46, 0x00, 0 } } }, //
  { "Mappers/7_test_src/7_test_1.nes", MN_REGION_NTSC, 0, 0, { { 0x31643FF1, 46, 0x00, 0 } } }, //
  { "Mappers/7_test_src/7_test_2.nes", MN_REGION_NTSC, 0, 0, { { 0x417AB4E6, 46, 0x00, 0 } } }, //
  { "Mappers/31_test_roms/31_test_16.nes", MN_REGION_NTSC, 0, 0, { { 0x76940301, 116, 0x00, 0 } } }, //
  { "Mappers/31_test_roms/31_test_32.nes", MN_REGION_NTSC, 0, 0, { { 0x8828FA76, 116, 0x00, 0 } } }, //
  { "Mappers/31_test_roms/31_test_64.nes", MN_REGION_NTSC, 0, 0, { { 0x4B58E3E6, 116, 0x00, 0 } } }, //
  { "Mappers/31_test_roms/31_test_128.nes", MN_REGION_NTSC, 0, 0, { { 0x8853D728, 116, 0x00, 0 } } }, //
  { "Mappers/31_test_roms/31_test_256.nes", MN_REGION_NTSC, 0, 0, { { 0xC46DF783, 116, 0x00, 0 } } }, //
  { "Mappers/31_test_roms/31_test_512.nes", MN_REGION_NTSC, 0, 0, { { 0x58DE4C5F, 116, 0x00, 0 } } }, //
  { "Mappers/31_test_roms/31_test_1024.nes", MN_REGION_NTSC, 0, 0, { { 0xCB66CDDB, 116, 0x00, 0 } } }, //
  { "Mappers/34_test_src/34_test_1.nes", MN_REGION_NTSC, 0, 0, { { 0xFCDB7FD7, 54, 0x00, 0 } } }, //
  { "Mappers/34_test_src/34_test_2.nes", MN_REGION_NTSC, 0, 0, { { 0xA01A3864, 58, 0x00, 0 } } }, //
  { "Mappers/bntest/bntest_aorom.nes", MN_REGION_NTSC, 0, 0, { { 0x2904F001, 10, 0x00, 0 } } }, //
  { "Mappers/bntest/bntest_h.nes", MN_REGION_NTSC, 0, 0, { { 0x3B40E181, 10, 0x00, 0 } } }, //
  { "Mappers/bntest/bntest_v.nes", MN_REGION_NTSC, 0, 0, { { 0x13DACC05, 10, 0x00, 0 } } }, //
  { "Mappers/bxrom_512k_test_src/bxrom_512k_test.nes", MN_REGION_NTSC, 0, 0, { { 0x3B545602, 457, 0x00, 0 } } }, //
  { "Mappers/exram/mmc5exram.nes", MN_REGION_NTSC, 0, 0, { { 0x03366E31, 60, 0, 0 } } }, //
  { "Mappers/fme7acktest-r1/fme7acktest.nes", MN_REGION_NTSC, 0, 0, { { 0xD9408008, 16, 0x00, 0 } } }, //
  { "Mappers/fme7ramtest-r1/fme7ramtest_128k.nes", MN_REGION_NTSC, 0, 0, { { 0x0EB36897, 13, 0x00, 0 } } }, //
  { "Mappers/fme7ramtest-r1/fme7ramtest.nes", MN_REGION_NTSC, 0, 0, { { 0x20F7E45B, 13, 0x00, 0 } } }, //
  { "Mappers/holy-mapperel/M0_P32K_C8K_V.nes", MN_REGION_NTSC, 0, 0, { { 0x981BD784, 5, 0x00, 0 } } }, //
  { "Mappers/holy-mapperel/M0_P32K_CR8K_V.nes", MN_REGION_NTSC, 0, 0, { { 0xE0E298B7, 76, 0x00, 0 } } }, //
  { "Mappers/holy-mapperel/M0_P32K_CR32K_V.nes", MN_REGION_NTSC, 0, 0, { { 0xE0E298B7, 76, 0x00, 0 } } }, //
  { "Mappers/holy-mapperel/M1_P128K_C32K_S8K.nes", MN_REGION_NTSC, 0, 0, { { 0x8E756874, 81, 0x00, 0 } } }, //
  { "Mappers/holy-mapperel/M1_P128K_C32K_W8K.nes", MN_REGION_NTSC, 0, 0, { { 0x62412ED9, 81, 0x00, 0 } } }, //
  { "Mappers/holy-mapperel/M1_P128K_C32K.nes", MN_REGION_NTSC, 0, 0, { { 0x5AAC6663, 6, 0x00, 0 } } }, //
  { "Mappers/holy-mapperel/M1_P128K_C128K_S8K.nes", MN_REGION_NTSC, 0, 0, { { 0x701554BB, 82, 0x00, 0 } } }, //
  { "Mappers/holy-mapperel/M1_P128K_C128K_W8K.nes", MN_REGION_NTSC, 0, 0, { { 0x9C211216, 82, 0x00, 0 } } }, //
  { "Mappers/holy-mapperel/M1_P128K_C128K.nes", MN_REGION_NTSC, 0, 0, { { 0x3AB7DB20, 6, 0x00, 0 } } }, //
  { "Mappers/holy-mapperel/M1_P128K_CR8K.nes", MN_REGION_NTSC, 0, 0, { { 0x197C6D62, 77, 0x00, 0 } } }, //
  { "Mappers/holy-mapperel/M1_P128K.nes", MN_REGION_NTSC, 0, 0, { { 0xE6FC43EE, 77, 0x00, 0 } } }, //
  { "Mappers/holy-mapperel/M1_P512K_CR8K_S8K.nes", MN_REGION_NTSC, 0, 0, { { 0xEAAA9550, 155, 0x00, 0 } } }, //
  { "Mappers/holy-mapperel/M1_P512K_CR8K_S32K.nes", MN_REGION_NTSC, 0, 0, { { 0x68E2595B, 451, 0x00, 0 } } }, //
  { "Mappers/holy-mapperel/M1_P512K_S8K.nes", MN_REGION_NTSC, 0, 0, { { 0x152ABBDC, 155, 0x00, 0 } } }, //
  { "Mappers/holy-mapperel/M1_P512K_S32K.nes", MN_REGION_NTSC, 0, 0, { { 0x976277D7, 451, 0x00, 0 } } }, //
  { "Mappers/holy-mapperel/M2_P128K_CR8K_V.nes", MN_REGION_NTSC, 0, 0, { { 0xB12E2EEB, 76, 0x00, 0 } } }, //
  { "Mappers/holy-mapperel/M2_P128K_V.nes", MN_REGION_NTSC, 0, 0, { { 0x4EAE0067, 76, 0x00, 0 } } }, //
  { "Mappers/holy-mapperel/M3_P32K_C32K_H.nes", MN_REGION_NTSC, 0, 0, { { 0x0D541701, 5, 0x00, 0 } } }, //
  { "Mappers/holy-mapperel/M4_P128K_CR8K.nes", MN_REGION_NTSC, 0, 0, { { 0x5B785B84, 78, 0x00, 0 } } }, //
  { "Mappers/holy-mapperel/M4_P128K_CR32K.nes", MN_REGION_NTSC, 0, 0, { { 0x6A06D7E0, 284, 0x00, 0 } } }, //
  { "Mappers/holy-mapperel/M4_P128K.nes", MN_REGION_NTSC, 0, 0, { { 0xA4F87508, 78, 0x00, 0 } } }, //
  { "Mappers/holy-mapperel/M4_P256K_C256K.nes", MN_REGION_NTSC, 0, 0, { { 0x67842BC7, 13, 0x00, 0 } } }, //
  { "Mappers/holy-mapperel/M7_P128K_CR8K.nes", MN_REGION_NTSC, 0, 0, { { 0x63328D8D, 76, 0x00, 0 } } }, //
  { "Mappers/holy-mapperel/M7_P128K.nes", MN_REGION_NTSC, 0, 0, { { 0x9CB2A301, 76, 0x00, 0 } } }, //
  { "Mappers/holy-mapperel/M9_P128K_C64K.nes", MN_REGION_NTSC, 0, 0, { { 0x0B81BA06, 6, 0x00, 0 } } }, //
  { "Mappers/holy-mapperel/M10_P128K_C64K_S8K.nes", MN_REGION_NTSC, 0, 0, { { 0x9D8A3F35, 113, 0x00, 0 } } }, //
  { "Mappers/holy-mapperel/M10_P128K_C64K_W8K.nes", MN_REGION_NTSC, 0, 0, { { 0x71BE7998, 81, 0x00, 0 } } }, //
  { "Mappers/holy-mapperel/M11_P64K_C64K_V.nes", MN_REGION_NTSC, 0, 0, { { 0x5E238DB4, 5, 0x00, 0 } } }, //
  { "Mappers/holy-mapperel/M11_P64K_CR32K_V.nes", MN_REGION_NTSC, 0, 0, { { 0x87691334, 282, 0x00, 0 } } }, //
  { "Mappers/holy-mapperel/M28_P512K_CR32K.nes", MN_REGION_NTSC, 0, 0, { { 0xF96705C9, 403, 0x00, 0 } } }, //
  { "Mappers/holy-mapperel/M28_P512K.nes", MN_REGION_NTSC, 0, 0, { { 0xF12687C9, 198, 0x00, 0 } } }, //
  { "Mappers/holy-mapperel/M34_P128K_CR8K_H.nes", MN_REGION_NTSC, 0, 0, { { 0x2CE60CEA, 76, 0x00, 0 } } }, //
  { "Mappers/holy-mapperel/M34_P128K_H.nes", MN_REGION_NTSC, 0, 0, { { 0xD3662266, 76, 0x00, 0 } } }, //
  { "Mappers/holy-mapperel/M66_P64K_C16K_V.nes", MN_REGION_NTSC, 0, 0, { { 0xE8B5DD23, 5, 0x00, 0 } } }, //
  { "Mappers/holy-mapperel/M69_P128K_C64K_S8K.nes", MN_REGION_NTSC, 0, 0, { { 0x0255D129, 113, 0x00, 0 } } }, //
  { "Mappers/holy-mapperel/M69_P128K_C64K_W8K.nes", MN_REGION_NTSC, 0, 0, { { 0xEE619784, 83, 0x00, 0 } } }, //
  { "Mappers/holy-mapperel/M78.3_P128K_C64K.nes", MN_REGION_NTSC, 0, 0, { { 0x7A52DE0F, 5, 0x00, 0 } } }, //
  { "Mappers/holy-mapperel/M118_P128K_C64K.nes", MN_REGION_NTSC, 0, 0, { { 0x965FFB01, 9, 0x00, 0 } } }, //
  { "Mappers/holy-mapperel/M180_P128K_CR8K_H.nes", MN_REGION_NTSC, 0, 0, { { 0xD91F39C2, 76, 0x00, 0 } } }, //
  { "Mappers/holy-mapperel/M180_P128K_H.nes", MN_REGION_NTSC, 0, 0, { { 0x269F174E, 76, 0x00, 0 } } }, //
  { "Mappers/MMC1_A12/mmc1_a12.nes", MN_REGION_NTSC, 0, 0, { { 0xD19AE5E0, 27, 0x00, 0 } } }, //
  { "Mappers/mmc1atest-0.01/mmc1atest-sn.nes", MN_REGION_NTSC, 0, 0, { { 0x34AB9BFB, 5, 0x00, 0 } } }, //
  { "Mappers/mmc1atest-0.01/mmc1atest.nes", MN_REGION_NTSC, 0, 0, { { 0x34AB9BFB, 5, 0x00, 0 } } }, //
  { "Mappers/mmc3_irq_tests/1.Clocking.nes", MN_REGION_NTSC, 0, 0, { { 0xB732D83F, 18, 0x00, 0 } } }, //
  { "Mappers/mmc3_irq_tests/2.Details.nes", MN_REGION_NTSC, 0, 0, { { 0x3E64BDBE, 23, 0x00, 0 } } }, //
  { "Mappers/mmc3_irq_tests/3.A12_clocking.nes", MN_REGION_NTSC, 0, 0, { { 0x9A45FD7C, 18, 0x00, 0 } } }, //
  { "Mappers/mmc3_irq_tests/4.Scanline_timing.nes", MN_REGION_NTSC, 0, 0, { { 0xA6F1F0EA, 79, 0x00, 0 } } }, //
  { "Mappers/mmc3_irq_tests/5.MMC3_rev_A_m12.nes", MN_REGION_NTSC, 0, 0, { { 0x022863D4, 18, 0x00, 0 } } }, //
  { "Mappers/mmc3_irq_tests/6.MMC3_rev_B.nes", MN_REGION_NTSC, 0, 0, { { 0x6DBFFE7B, 18, 0x00, 0 } } }, //
  { "Mappers/mmc3_test/1-clocking.nes", MN_REGION_NTSC, 0, 0, { { 0x3D4C2E04, 24, 0x00, 0 } } }, //
  { "Mappers/mmc3_test/2-details.nes", MN_REGION_NTSC, 0, 0, { { 0x390FC588, 26, 0x00, 0 } } }, //
  { "Mappers/mmc3_test/3-A12_clocking.nes", MN_REGION_NTSC, 0, 0, { { 0xD3189FEE, 24, 0x00, 0 } } }, //
  { "Mappers/mmc3_test/4-scanline_timing.nes", MN_REGION_NTSC, 0, 0, { { 0xF6D14A7E, 315, 0x00, 0 } } }, //
  { "Mappers/mmc3_test/5-MMC3.nes", MN_REGION_NTSC, 0, 0, { { 0xFAF35560, 23, 0x00, 0 } } }, //
  { "Mappers/mmc3_test/6-MMC6_m12.nes", MN_REGION_NTSC, 0, 0, { { 0xAD40DA76, 23, 0x00, 0 } } }, //
  { "Mappers/mmc3_test_2/rom_singles/1-clocking.nes", MN_REGION_NTSC, 0, 0, { { 0x3D4C2E04, 24, 0x00, 0 } } }, //
  { "Mappers/mmc3_test_2/rom_singles/2-details.nes", MN_REGION_NTSC, 0, 0, { { 0x390FC588, 26, 0x00, 0 } } }, //
  { "Mappers/mmc3_test_2/rom_singles/3-A12_clocking.nes", MN_REGION_NTSC, 0, 0, { { 0xD3189FEE, 24, 0x00, 0 } } }, //
  { "Mappers/mmc3_test_2/rom_singles/4-scanline_timing.nes",
    MN_REGION_NTSC,
    0,
    0,
    { { 0xF6D14A7E, 319, 0x00, 0 } } }, //
  { "Mappers/mmc3_test_2/rom_singles/5-MMC3.nes", MN_REGION_NTSC, 0, 0, { { 0xFAF35560, 25, 0x00, 0 } } }, //
  { "Mappers/mmc3_test_2/rom_singles/6-MMC3_alt_m12.nes", MN_REGION_NTSC, 0, 0, { { 0x09093FB9, 24, 0x00, 0 } } }, //
  { "Mappers/mmc3bigchrram-0.01/mmc3bigchrram.nes",
    MN_REGION_NTSC,
    0,
    0,
    { { 0xF54230B4, 10, 0x00, 0 }, { 0x44A5C30F, 70, MN_INPUT_START, 0 } } }, //
  { "Mappers/mmc3irqtest.nes", MN_REGION_NTSC, 0, 0, { { 0x14A7B673, 20, 0x00, 0 } } }, //
  { "Mappers/mmc5test/mmc5test.nes", MN_REGION_NTSC, 0, 0, { { 0xE3DAEE3D, 120, 0, 0 } } },
  // { "mmc5test_v2/mmc5test.nes", MN_REGION_NTSC, 0, 0, {} }, // todo
  { "Mappers/serom/serom.nes", MN_REGION_NTSC, 0, 0, { { 0x6C5F9EDD, 10, 0x00, 0 } } }, //
  { "Mappers/test28-0.04/test28-8Mbit.nes",
    MN_REGION_NTSC,
    0,
    0,
    { { 0xF1D3104A, 10, 0x00, 0 }, { 0x28BF334B, 2100, MN_INPUT_START, 0 }, { 0xE87350E8, 10, 0, 1 } } }, //
  { "Mappers/test28-0.04/test28.nes",
    MN_REGION_NTSC,
    0,
    0,
    { { 0xF1D3104A, 10, 0x00, 0 }, { 0xE4FDA8DC, 1100, MN_INPUT_START, 0 }, { 0xE87350E8, 10, 0, 1 } } }, //
  { "Mappers/vrc6test/vrc6test24.nes", MN_REGION_NTSC, 0, 0, { { 0xB164284C, 540, 0x00, 0 } } }, //
  { "Mappers/vrc6test/vrc6test26.nes", MN_REGION_NTSC, 0, 0, { { 0xB164284C, 540, 0x00, 0 } } }, //
  { "Mappers/vrctest/vrctest21s0.nes", MN_REGION_NTSC, 0, 0, { { 0x579C0323, 5, 0x00, 0 } } }, //
  { "Mappers/vrctest/vrctest21s1.nes", MN_REGION_NTSC, 0, 0, { { 0x24AFA780, 5, 0x00, 0 } } }, //
  { "Mappers/vrctest/vrctest21s2.nes", MN_REGION_NTSC, 0, 0, { { 0x3AB4BA0C, 5, 0x00, 0 } } }, //
  { "Mappers/vrctest/vrctest22.nes", MN_REGION_NTSC, 0, 0, { { 0x7271DB9B, 5, 0x00, 0 } } }, //
  { "Mappers/vrctest/vrctest23s0.nes", MN_REGION_NTSC, 0, 0, { { 0xED3AACB2, 5, 0x00, 0 } } }, //
  { "Mappers/vrctest/vrctest23s1.nes", MN_REGION_NTSC, 0, 0, { { 0xFFE71C79, 5, 0x00, 0 } } }, //
  { "Mappers/vrctest/vrctest23s2.nes", MN_REGION_NTSC, 0, 0, { { 0x771EDC58, 5, 0x00, 0 } } }, //
  { "Mappers/vrctest/vrctest23s3.nes", MN_REGION_NTSC, 0, 0, { { 0xAD21A096, 5, 0x00, 0 } } }, //
  { "Mappers/vrctest/vrctest25s0.nes", MN_REGION_NTSC, 0, 0, { { 0x2EA25E99, 5, 0x00, 0 } } }, //
  { "Mappers/vrctest/vrctest25s1.nes", MN_REGION_NTSC, 0, 0, { { 0x7B2EA541, 5, 0x00, 0 } } }, //
  { "Mappers/vrctest/vrctest25s2.nes", MN_REGION_NTSC, 0, 0, { { 0x081F36C0, 5, 0x00, 0 } } }, //
  { "Mappers/vrctest/vrctest25s3.nes", MN_REGION_NTSC, 0, 0, { { 0x78CE440A, 5, 0x00, 0 } } }, //
  { "PPU/blargg_ppu_tests_2005.09.15b/palette_ram.nes", MN_REGION_NTSC, 0, 0, { { 0x2886AFFC, 18, 0x00, 0 } } }, //
  { "PPU/blargg_ppu_tests_2005.09.15b/sprite_ram.nes", MN_REGION_NTSC, 0, 0, { { 0x2886AFFC, 18, 0x00, 0 } } }, //
  { "PPU/blargg_ppu_tests_2005.09.15b/vbl_clear_time.nes", MN_REGION_NTSC, 0, 0, { { 0x2886AFFC, 23, 0x00, 0 } } }, //
  { "PPU/blargg_ppu_tests_2005.09.15b/vram_access.nes", MN_REGION_NTSC, 0, 0, { { 0x2886AFFC, 18, 0x00, 0 } } }, //
  { "PPU/dmc_dma_during_read4/dma_2007_read.nes", MN_REGION_NTSC, 0, 0, { { 0x8D3CE006, 25, 0x00, 0 } } }, //
  { "PPU/dmc_dma_during_read4/dma_2007_write.nes", MN_REGION_NTSC, 0, 0, { { 0x94175082, 28, 0x00, 0 } } }, //
  { "PPU/dmc_dma_during_read4/dma_4016_read.nes", MN_REGION_NTSC, 0, 0, { { 0x441C2F34, 20, 0x00, 0 } } }, //
  { "PPU/dmc_dma_during_read4/double_2007_read.nes", MN_REGION_NTSC, 0, 0, { { 0x80B4A112, 16, 0x00, 0 } } }, //
  { "PPU/dmc_dma_during_read4/read_write_2007.nes", MN_REGION_NTSC, 0, 0, { { 0xEB956420, 18, 0x00, 0 } } }, //
  { "PPU/full_palette/flowing_palette.nes", MN_REGION_NTSC, 0, 0, { { 0x26E5F0A2, 31, 0, 0 } } }, //
  { "PPU/full_palette/full_palette_smooth.nes", MN_REGION_NTSC, 0, 0, { { 0x208EC12D, 31, 0, 0 } } }, //
  { "PPU/full_palette/full_palette.nes", MN_REGION_NTSC, 0, 0, { { 0xDE3FEC30, 31, 0, 0 } } }, //
  { "PPU/nmi_sync/demo_ntsc.nes",
    MN_REGION_NTSC,
    0,
    0,
    { { 0xB9F1DFBA, 29, 0, 0 }, { 0x1D8EF9BE, 1, 0, 0 }, { 0xB9F1DFBA, 1, 0, 0 }, { 0x1D8EF9BE, 1, 0, 0 } } }, //
  { "PPU/nmi_sync/demo_pal.nes",
    MN_REGION_PAL,
    0,
    0,
    { { 0x1E9B3479, 45, 0, 0 }, { 0x644FDA4C, 1, 0, 0 }, { 0x1E9B3479, 1, 0, 0 }, { 0x644FDA4C, 1, 0, 0 } } }, //
  { "PPU/oam_read/oam_read.nes", MN_REGION_NTSC, 0, 0, { { 0xA35A57E7, 32, 0x00, 0 } } }, //
  { "PPU/oam_stress/oam_stress.nes", MN_REGION_NTSC, 0, 0, { { 0x596023FD, 1703, 0x00, 0 } } }, //
  { "PPU/oamtest3/oam3.nes", MN_REGION_NTSC, 0, 0, { { 0xA87189F4, 10, 0x00, 0 } } }, //
  { "PPU/2nd2006_next_level.nes",
    MN_REGION_NTSC,
    0,
    0,
    { { 0xA728E1F7, 60, 0, 0 },
      { 0xA728E1F7, 1, 0, 0 },
      { 0xBB7B0AC9, 2, MN_INPUT_RIGHT, 0 },
      { 0xBB7B0AC9, 1, 0, 0 },
      { 0xE4A6D7DE, 2, MN_INPUT_RIGHT, 0 },
      { 0xE4A6D7DE, 1, 0, 0 },
      { 0xBED32914, 2, MN_INPUT_RIGHT, 0 },
      { 0xBED32914, 1, 0, 0 },
      { 0x554FE502, 2, MN_INPUT_RIGHT, 0 },
      { 0xAF734861, 1, 0, 0 },
      { 0xF86B7CD3, 2, MN_INPUT_RIGHT, 0 },
      { 0xF86B7CD3, 1, 0, 0 },
      { 0x4B5B49EB, 2, MN_INPUT_RIGHT, 0 },
      { 0x4B5B49EB, 1, 0, 0 },
      { 0xCBA86936, 2, MN_INPUT_RIGHT, 0 },
      { 0xCBA86936, 1, 0, 0 },
      { 0x0BA45811, 2, MN_INPUT_RIGHT, 0 },
      { 0x0BA45811, 1, 0, 0 } } }, //
  { "PPU/2003-test/2003-test.nes", MN_REGION_NTSC, 0, 0, { { 0x49BDA4AF, 15, 0x00, 0 } } }, //
  { "PPU/oam_read_vbl_wait.nes", MN_REGION_NTSC, 0, 0, { { 0xEE5E359A, 47, 0x00, 0 } } }, //
  { "PPU/read2004/read2004.nes", MN_REGION_NTSC, 0, 0, { { 0x519A9E5E, 20, 0x00, 0 } } }, //
  { "PPU/ppu_open_bus/ppu_open_bus.nes", MN_REGION_NTSC, 0, 0, { { 0x3EB4ABE5, 250, 0x00, 0 } } }, //
  { "PPU/ppu_read_buffer/test_ppu_read_buffer.nes", MN_REGION_NTSC, 0, 0, { { 0x7239FBBE, 1307, 0x00, 0 } } }, //
  { "PPU/ppu_sprite_hit/rom_singles/01-basics.nes", MN_REGION_NTSC, 0, 0, { { 0xE654945D, 43, 0x00, 0 } } }, //
  { "PPU/ppu_sprite_hit/rom_singles/02-alignment.nes", MN_REGION_NTSC, 0, 0, { { 0x057D9670, 40, 0x00, 0 } } }, //
  { "PPU/ppu_sprite_hit/rom_singles/03-corners.nes", MN_REGION_NTSC, 0, 0, { { 0xEFCA28B7, 25, 0x00, 0 } } }, //
  { "PPU/ppu_sprite_hit/rom_singles/04-flip.nes", MN_REGION_NTSC, 0, 0, { { 0xB297CC81, 21, 0x00, 0 } } }, //
  { "PPU/ppu_sprite_hit/rom_singles/05-left_clip.nes", MN_REGION_NTSC, 0, 0, { { 0x9DF4E002, 38, 0x00, 0 } } }, //
  { "PPU/ppu_sprite_hit/rom_singles/06-right_edge.nes", MN_REGION_NTSC, 0, 0, { { 0x601652B5, 28, 0x00, 0 } } }, //
  { "PPU/ppu_sprite_hit/rom_singles/07-screen_bottom.nes", MN_REGION_NTSC, 0, 0, { { 0x50189868, 33, 0x00, 0 } } }, //
  { "PPU/ppu_sprite_hit/rom_singles/08-double_height.nes", MN_REGION_NTSC, 0, 0, { { 0x53286670, 24, 0x00, 0 } } }, //
  { "PPU/ppu_sprite_hit/rom_singles/09-timing.nes", MN_REGION_NTSC, 0, 0, { { 0x00CC842A, 177, 0x00, 0 } } }, //
  { "PPU/ppu_sprite_hit/rom_singles/10-timing_order.nes", MN_REGION_NTSC, 0, 0, { { 0x94A60512, 69, 0x00, 0 } } }, //
  { "PPU/ppu_sprite_hit/ppu_sprite_hit.nes", MN_REGION_NTSC, 0, 0, { { 0xDDE50B5E, 584, 0x00, 0 } } }, //
  { "PPU/ppu_sprite_overflow/rom_singles/01-basics.nes", MN_REGION_NTSC, 0, 0, { { 0xE654945D, 25, 0x00, 0 } } }, //
  { "PPU/ppu_sprite_overflow/rom_singles/02-details.nes", MN_REGION_NTSC, 0, 0, { { 0x7111D067, 30, 0x00, 0 } } }, //
  { "PPU/ppu_sprite_overflow/rom_singles/03-timing.nes", MN_REGION_NTSC, 0, 0, { { 0x6E1E7158, 316, 0x00, 0 } } }, //
  { "PPU/ppu_sprite_overflow/rom_singles/04-obscure.nes", MN_REGION_NTSC, 0, 0, { { 0xCE39FCC6, 26, 0x00, 0 } } }, //
  { "PPU/ppu_sprite_overflow/rom_singles/05-emulator.nes", MN_REGION_NTSC, 0, 0, { { 0x5E275C0F, 19, 0x00, 0 } } }, //
  { "PPU/ppu_sprite_overflow/ppu_sprite_overflow.nes", MN_REGION_NTSC, 0, 0, { { 0x12619948, 457, 0x00, 0 } } }, //
  { "PPU/ppu_vbl_nmi/rom_singles/01-vbl_basics.nes", MN_REGION_NTSC, 0, 0, { { 0x1AF3E9A3, 140, 0x00, 0 } } }, //
  { "PPU/ppu_vbl_nmi/rom_singles/02-vbl_set_time.nes", MN_REGION_NTSC, 0, 0, { { 0x59B9C83E, 178, 0x00, 0 } } }, //
  { "PPU/ppu_vbl_nmi/rom_singles/03-vbl_clear_time.nes", MN_REGION_NTSC, 0, 0, { { 0x7B0B8BE1, 169, 0x00, 0 } } }, //
  { "PPU/ppu_vbl_nmi/rom_singles/04-nmi_control.nes", MN_REGION_NTSC, 0, 0, { { 0xCD67F9C1, 31, 0x00, 0 } } }, //
  { "PPU/ppu_vbl_nmi/rom_singles/05-nmi_timing.nes", MN_REGION_NTSC, 0, 0, { { 0xE19A001A, 219, 0x00, 0 } } }, //
  { "PPU/ppu_vbl_nmi/rom_singles/06-suppression.nes", MN_REGION_NTSC, 0, 0, { { 0x4D1533CC, 222, 0x00, 0 } } }, //
  { "PPU/ppu_vbl_nmi/rom_singles/07-nmi_on_timing.nes", MN_REGION_NTSC, 0, 0, { { 0x1658D4F5, 197, 0x00, 0 } } }, //
  { "PPU/ppu_vbl_nmi/rom_singles/08-nmi_off_timing.nes", MN_REGION_NTSC, 0, 0, { { 0xA3F47CBF, 221, 0x00, 0 } } }, //
  { "PPU/ppu_vbl_nmi/rom_singles/09-even_odd_frames.nes", MN_REGION_NTSC, 0, 0, { { 0xA590A81C, 76, 0x00, 0 } } }, //
  { "PPU/ppu_vbl_nmi/rom_singles/10-even_odd_timing.nes", MN_REGION_NTSC, 0, 0, { { 0xB3F44119, 142, 0x00, 0 } } }, //
  { "PPU/ppu_vbl_nmi/ppu_vbl_nmi.nes", MN_REGION_NTSC, 0, 0, { { 0xDDE50B5E, 1616, 0, 0 } } }, //
  { "PPU/scanline/scanline.nes",
    MN_REGION_NTSC,
    0,
    0,
    { { 0x629D1349, 80, 0, 0 } } }, // area1 flicker pixels after line
  { "PPU/scanline-a1/scanline.nes", MN_REGION_NTSC, 0, 0, { { 0x629D1349, 80, 0, 0 } } }, // same as above
  { "PPU/sprite_hit_tests_2005.10.05/01.basics.nes", MN_REGION_NTSC, 0, 0, { { 0xAC38F2AD, 33, 0x00, 0 } } }, //
  { "PPU/sprite_hit_tests_2005.10.05/02.alignment.nes", MN_REGION_NTSC, 0, 0, { { 0x3BF08842, 31, 0x00, 0 } } }, //
  { "PPU/sprite_hit_tests_2005.10.05/03.corners.nes", MN_REGION_NTSC, 0, 0, { { 0xC53BD4BF, 20, 0x00, 0 } } }, //
  { "PPU/sprite_hit_tests_2005.10.05/04.flip.nes", MN_REGION_NTSC, 0, 0, { { 0x0B05D9E8, 19, 0x00, 0 } } }, //
  { "PPU/sprite_hit_tests_2005.10.05/05.left_clip.nes", MN_REGION_NTSC, 0, 0, { { 0x95D74663, 30, 0x00, 0 } } }, //
  { "PPU/sprite_hit_tests_2005.10.05/06.right_edge.nes", MN_REGION_NTSC, 0, 0, { { 0x420C5953, 23, 0x00, 0 } } }, //
  { "PPU/sprite_hit_tests_2005.10.05/07.screen_bottom.nes", MN_REGION_NTSC, 0, 0, { { 0xCE4105CF, 24, 0x00, 0 } } }, //
  { "PPU/sprite_hit_tests_2005.10.05/08.double_height.nes", MN_REGION_NTSC, 0, 0, { { 0xCE51B630, 20, 0x00, 0 } } }, //
  { "PPU/sprite_hit_tests_2005.10.05/09.timing_basics.nes", MN_REGION_NTSC, 0, 0, { { 0xE68A1C8B, 72, 0x00, 0 } } }, //
  { "PPU/sprite_hit_tests_2005.10.05/10.timing_order.nes", MN_REGION_NTSC, 0, 0, { { 0xEBEBD82C, 72, 0x00, 0 } } }, //
  { "PPU/sprite_hit_tests_2005.10.05/11.edge_timing.nes", MN_REGION_NTSC, 0, 0, { { 0x055D6A29, 47, 0x00, 0 } } }, //
  { "PPU/sprite_overflow_tests/1.Basics.nes", MN_REGION_NTSC, 0, 0, { { 0x03CC909D, 15, 0x00, 0 } } }, //
  { "PPU/sprite_overflow_tests/2.Details.nes", MN_REGION_NTSC, 0, 0, { { 0xC833B4E1, 24, 0x00, 0 } } }, //
  { "PPU/sprite_overflow_tests/3.Timing.nes", MN_REGION_NTSC, 0, 0, { { 0x39154B5F, 128, 0x00, 0 } } }, //
  { "PPU/sprite_overflow_tests/4.Obscure.nes", MN_REGION_NTSC, 0, 0, { { 0xE2E52CC7, 22, 0x00, 0 } } }, //
  { "PPU/sprite_overflow_tests/5.Emulator.nes", MN_REGION_NTSC, 0, 0, { { 0xBAFE1B96, 13, 0x00, 0 } } }, //
  { "PPU/vbl_nmi_timing/1.frame_basics.nes", MN_REGION_NTSC, 0, 0, { { 0x509B862D, 176, 0x00, 0 } } }, //
  { "PPU/vbl_nmi_timing/2.vbl_timing.nes", MN_REGION_NTSC, 0, 0, { { 0xACDD6906, 154, 0x00, 0 } } }, //
  { "PPU/vbl_nmi_timing/3.even_odd_frames.nes", MN_REGION_NTSC, 0, 0, { { 0xF2D9B6C9, 99, 0x00, 0 } } }, //
  { "PPU/vbl_nmi_timing/4.vbl_clear_timing.nes", MN_REGION_NTSC, 0, 0, { { 0x3B42F8FA, 117, 0x00, 0 } } }, //
  { "PPU/vbl_nmi_timing/5.nmi_suppression.nes", MN_REGION_NTSC, 0, 0, { { 0xB2955A03, 166, 0x00, 0 } } }, //
  { "PPU/vbl_nmi_timing/6.nmi_disable.nes", MN_REGION_NTSC, 0, 0, { { 0xFD58C6AA, 109, 0x00, 0 } } }, //
  { "PPU/vbl_nmi_timing/7.nmi_timing.nes", MN_REGION_NTSC, 0, 0, { { 0xE5553CE0, 109, 0x00, 0 } } }, //
};

static uint calc_crc(const u8* data, uint size)
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

// static void save_tga(const char* img_path, const void* pixels, uint width, uint height)
// {
//   FILE* f = fopen(img_path, "wb");
//   if (!f) {
//     return;
//   }

// u8 header[18] = {};
// header[2] = 2;
// header[12] = width & 0xFF;
// header[13] = (width >> 8) & 0xFF;
// header[14] = height & 0xFF;
// header[15] = (height >> 8) & 0xFF;
// header[16] = 32;
// header[17] = 0x20;

// fwrite(header, 1, 18, f);
// fwrite(pixels, 4, (size_t)width * height, f);

// fclose(f);
// }

// typedef struct
// {
//   u16 magic;
//   uint file_size;
//   uint reserved;
//   uint offset;
//   uint info_size;
//   int width;
//   int height;
//   u16 planes;
//   u16 bpp;
//   uint compression;
//   uint image_size;
//   int ppm_x;
//   int ppm_y;
//   uint colors_used;
//   uint colors_important;
// } __attribute__((packed)) BMPHeader;

// static void save_bmp(const char* img_path, const void* pixels, uint width, uint height)
// {
//   FILE* f = fopen(img_path, "wb");
//   if (!f) {
//     return;
//   }

// uint image_size = width * height * 4;

// BMPHeader header;
// header.magic = 0x4D42;
// header.file_size = sizeof(BMPHeader) + image_size;
// header.reserved = 0;
// header.offset = sizeof(BMPHeader);
// header.info_size = 40;
// header.width = width;
// header.height = -height;
// header.planes = 1;
// header.bpp = 32;
// header.compression = 0;
// header.image_size = image_size;
// header.ppm_x = 2835;
// header.ppm_y = 2835;
// header.colors_used = 0;
// header.colors_important = 0;

// fwrite(&header, 1, sizeof(BMPHeader), f);
// fwrite(pixels, 4, (size_t)width * height, f);
// fclose(f);
// }

static void save_png(const char* img_path, const void* pixels, uint width, uint height)
{
  SDL_Surface* surface = SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_XRGB8888, (void*)pixels, width * 4);
  SDL_SavePNG(surface, img_path);
  SDL_DestroySurface(surface);
}

static void make_dirs(char* path)
{
  uint path_len = strlen(path);
  for (uint i = 0; i <= path_len; i++) {
    if (path[i] == '/') {
      path[i] = 0;
      SDL_CreateDirectory(path);
      // mkdir(path, 0777);
      path[i] = '/';
    }
  }
}

static uint audio_data[512 * 4096];
static uint audio_size;

static uint db_to_uint(double db, double min_db)
{
  double scaled = ((db - min_db) / (-min_db)) * 256.0;
  uint result = (uint)scaled;
  return result > 255 ? 255 : result;
}

static uint fit_amplitude(uint amplitude)
{
  if (amplitude == 0) {
    return 0;
  }

  const double min_db = -60.0;
  double db = 20.0 * log10((double)amplitude / 0x7FFF);
  if (db < min_db) {
    return 0;
  }

  return db_to_uint(db, min_db);
}

static uint fit_zero_cross_count(uint zero_cross_count, uint max_count)
{
  if (zero_cross_count == 0) {
    return 0;
  }
  if (zero_cross_count >= max_count) {
    return 255;
  }

  double log_current = log2((double)zero_cross_count + 1.0);
  double log_max = log2((double)max_count + 1.0);
  double scaled = (log_current / log_max) * 256.0;

  uint result = (uint)scaled;
  return result > 255 ? 255 : result;
}

static void visualize_audio_frame()
{
  if (audio_size >= 4096) {
    return;
  }
  i16* data = mn_audio_data();
  uint count = mn_audio_size() / sizeof(i16);
  uint min_amplitude = -1;
  uint max_amplitude = 0;
  uint zero_cross_count = 0;
  bool last_positive = false;
  for (uint i = 0; i < count; ++i) {
    bool is_positive = (data[i] >= 0);
    uint amplitude = is_positive ? data[i] : -data[i];
    if (amplitude > max_amplitude) {
      max_amplitude = amplitude;
    }
    if (amplitude < min_amplitude) {
      min_amplitude = amplitude;
    }
    if (amplitude > 1) { // ignore dither
      if (is_positive != last_positive) {
        ++zero_cross_count;
      }
      last_positive = is_positive;
    }
  }

  min_amplitude = fit_amplitude(min_amplitude);
  max_amplitude = fit_amplitude(max_amplitude);
  zero_cross_count = fit_zero_cross_count(zero_cross_count, count);

  uint y = audio_size++;

  for (uint x = 0; x <= max_amplitude; ++x) {
    uint color;
    switch (x / 42) {
      case 0: color = 0xFF0000FF; break;
      case 1: color = 0xFF0080FF; break;
      case 2: color = 0xFF00FF80; break;
      case 3: color = 0xFF00FF00; break;
      case 4: color = 0xFF80FF00; break;
      case 5: color = 0xFFFF8000; break;
      default: color = 0xFFFF0000; break;
    }
    audio_data[y * 512 + x] = color;
  }

  for (uint x = 0; x <= zero_cross_count; ++x) {
    uint color;
    switch (zero_cross_count / 42) {
      case 0: color = 0xFFFF00FF; break;
      case 1: color = 0xFF0000FF; break;
      case 2: color = 0xFF00FFFF; break;
      case 3: color = 0xFF00FF00; break;
      case 4: color = 0xFFFFFF00; break;
      case 5: color = 0xFFFF8000; break;
      default: color = 0xFFFF0000; break;
    }
    audio_data[y * 512 + 512 - x] = color;
  }
}

static void save_test_step_img(const char* rom_path, uint step)
{
  char img_path[512] = {};
  sprintf(img_path, "%s/screenshots/%s.%d.png", out_path, rom_path, step);
  make_dirs(img_path);
  if (audio_size) {
    save_png(img_path, audio_data, 512, audio_size);
  } else {
    save_png(img_path, mn_video_data(), 256, 240);
  }
}

static uint run_test_step(TestStep step, bool audio)
{
  if (step.reset) {
    mn_reset();
  }
  for (uint i = 0; i < step.frames; ++i) {
    mn_frame(step.joy1, 0);
    if (audio) {
      visualize_audio_frame();
    }
  }
  if (audio_size) {
    return calc_crc((u8*)audio_data, audio_size * 256 * 4);
  } else {
    return calc_crc((u8*)mn_video_data(), 256 * 240 * 4);
  }
}

static bool run_test_steps(TestParams params, uint* out_hash)
{
  memset(mn_video_data(), 0, 256ULL * 240 * 4);
  memset(audio_data, 0, sizeof(audio_data));
  audio_size = 0;
  char rom_path[512];
  sprintf(rom_path, "%s/%s", base_path, params.path);
  char sram_path[512];
  sprintf(sram_path, "%s/saves/%s.sav", out_path, params.path);
  make_dirs(sram_path);
  mn_rom rom = mn_rom_load(rom_path, sram_path);
  mn_region_set(params.region);
  mn_rom_set(rom);
  mn_apu_test_mode(params.apu_test_mode);

  bool result = true;
  for (uint i = 0; i < 32; ++i) {
    if (i > 0 && params.steps[i].frames == 0) {
      break;
    }
    uint hash = run_test_step(params.steps[i], params.audio);
    if (out_hash) {
      *out_hash = hash;
    }
    save_test_step_img(params.path, i);
    if (hash != params.steps[i].hash) {
      result = false;
      break;
    }
  }

  mn_rom_save(sram_path);
  mn_rom_release(rom);
  return result;
}

static bool run_test(TestParams params)
{
  bool result = false;
  if (!params.steps[0].hash || !params.steps[0].frames) {
    if (!params.steps[0].frames) {
      params.steps[0].frames = 600;
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

    printf("  { \"%s\", %s, %d, %d, { { 0x%08X, %d, 0x%02X, %d } } }, //\n", params.path,
           params.region == MN_REGION_NTSC    ? "MN_REGION_NTSC"
           : params.region == MN_REGION_PAL   ? "MN_REGION_PAL"
           : params.region == MN_REGION_DENDY ? "MN_REGION_DENDY"
                                              : "MN_REGION_AUTO",
           params.audio, params.apu_test_mode, params.steps[0].hash, params.steps[0].frames, params.steps[0].joy1,
           params.steps[0].reset);
  } else {
    uint hash = 0;
    result = run_test_steps(params, &hash);
    printf("%4s | 0x%08X | %s\n", result ? "OK" : "FAIL", hash, params.path);
  }
  return result;
}

int main(int argc, char** argv)
{
  if (argc < 3) {
    printf("Usage: %s [test-roms-dir] [output-dir]\n", argv[0]);
    return 1;
  }

  base_path = argv[1];
  out_path = argv[2];

  uint count = sizeof(tests) / sizeof(tests[0]);
  uint errors = 0;
  for (uint i = 0; i < count; ++i) {
    if (!run_test(tests[i])) {
      ++errors;
    }
  }
  printf("%4s | %-3d/%4d | TOTAL ERRORS\n", errors ? "FAIL" : "OK", errors, count);

  return errors;
}
