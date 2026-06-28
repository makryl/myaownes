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
  { "apu_reset/4015_cleared.nes", { { 0x03085F30, 8, 0, 0 }, { 0x68020FC4, 8, 0, 1 } } },
  { "apu_reset/4017_timing.nes", { { 0x543A547B, 15, 0, 0 }, { 0xF9619346, 12, 0, 1 } } },
  { "apu_reset/4017_written.nes", { { 0x03085F30, 11, 0, 0 }, { 0x8BECFD90, 7, 0, 1 }, { 0x2421E3A9, 8, 0, 1 } } },
  { "apu_reset/irq_flag_cleared.nes", { { 0x03085F30, 6, 0, 0 }, { 0x6157F5A6, 7, 0, 1 } } },
  { "apu_reset/len_ctrs_enabled.nes", { { 0x03085F30, 6, 0, 0 }, { 0xB62FEAE3, 10, 0, 1 } } },
  { "apu_reset/works_immediately.nes", { { 0x03085F30, 5, 0, 0 }, { 0x0000839C, 9, 0, 1 } } },
  { "apu_test/rom_singles/1-len_ctr.nes", { { 0x8E3C39E5, 11, 0, 0 } } },
  { "apu_test/rom_singles/2-len_table.nes", { { 0x0D477786, 7, 0, 0 } } },
  { "apu_test/rom_singles/3-irq_flag.nes", { { 0xBA4FC3FB, 10, 0, 0 } } },
  { "apu_test/rom_singles/4-jitter.nes", { { 0x3BA3894E, 11, 0, 0 } } },
  { "apu_test/rom_singles/5-len_timing.nes", { { 0x5B826EC5, 105, 0, 0 } } },
  { "apu_test/rom_singles/6-irq_flag_timing.nes", { { 0x31D8322C, 13, 0, 0 } } },
  { "apu_test/rom_singles/7-dmc_basics.nes", { { 0x96FCAE66, 15, 0, 0 } } },
  { "apu_test/rom_singles/8-dmc_rates.nes", { { 0xF97AC6BC, 20, 0, 0 } } },
  { "apu_test/apu_test.nes", { { 0x71B41136, 247, 0, 0 } } },
  { "branch_timing_tests/1.Branch_Basics.nes", { { 0xB6A4872B, 10, 0, 0 } } },
  { "branch_timing_tests/2.Backward_Branch.nes", { { 0x36D590FE, 11, 0, 0 } } },
  { "branch_timing_tests/3.Forward_Branch.nes", { { 0x6D0B3B45, 13, 0, 0 } } },
  { "cpu_dummy_reads/cpu_dummy_reads.nes", { { 0xA942A3CF, 29, 0, 0 } } },
  { "cpu_dummy_writes/cpu_dummy_writes_oam.nes", { { 0x0DEC41EF, 230, 0, 0 } } },
  { "cpu_dummy_writes/cpu_dummy_writes_ppumem.nes", { { 0x0124227E, 109, 0, 0 } } },
  { "cpu_exec_space/test_cpu_exec_space_apu.nes", { { 0xC9890A61, 134, 0, 0 } } },
  { "cpu_exec_space/test_cpu_exec_space_ppuio.nes", { { 0x31CA6229, 21, 0, 0 } } },
  { "cpu_interrupts_v2/rom_singles/1-cli_latency.nes", { { 0x04E43C3C, 6, 0, 0 } } },
  { "cpu_interrupts_v2/rom_singles/2-nmi_and_brk.nes", { { 0xEA3D7CA3, 64, 0, 0 } } },
  { "cpu_interrupts_v2/rom_singles/3-nmi_and_irq.nes", { { 0xB47C1A9A, 76, 0, 0 } } },
  { "cpu_interrupts_v2/rom_singles/4-irq_and_dma.nes", { { 0x62782874, 45, 0, 0 } } },
  { "cpu_interrupts_v2/rom_singles/5-branch_delays_irq.nes", { { 0x709B36CC, 349, 0, 0 } } },
  { "cpu_interrupts_v2/cpu_interrupts.nes", { { 0x12619948, 562, 0, 0 } } },
  { "cpu_reset/ram_after_reset.nes", { { 0x5B69B3BD, 132, 0, 0 }, { 0x07673647, 1, 0, 0 }, { 0x68B7E9F1, 5, 0, 1 } } },
  { "cpu_reset/registers.nes", { { 0x3235473C, 133, 0, 0 }, { 0x07673647, 1, 0, 0 }, { 0x8233D68A, 8, 0, 1 } } },
  { "cpu_timing_test6/cpu_timing_test.nes", { { 0x3751F886, 609, 0, 0 } } },
  { "dmc_dma_during_read4/dma_2007_read.nes", { { 0x8D3CE006, 12, 0, 0 } } },
  { "dmc_dma_during_read4/dma_2007_write.nes", { { 0x94175082, 16, 0, 0 } } },
  { "dmc_dma_during_read4/dma_4016_read.nes", { { 0x441C2F34, 9, 0, 0 } } },
  { "dmc_dma_during_read4/double_2007_read.nes", { { 0x80B4A112, 10, 0, 0 } } },
  { "dmc_dma_during_read4/read_write_2007.nes", { { 0xEB956420, 8, 0, 0 } } },
  { "full_palette/flowing_palette.nes", { { 0x3BE460EE, 44, 0, 0 } } },
  { "full_palette/full_palette_smooth.nes", { { 0x8E7BD8DB, 44, 0, 0 } } },
  { "full_palette/full_palette.nes", { { 0xDD6E6884, 44, 0, 0 } } },
  { "instr_misc/rom_singles/01-abs_x_wrap.nes", { { 0x290CAB05, 6, 0, 0 } } },
  { "instr_misc/rom_singles/02-branch_wrap.nes", { { 0x54041E4A, 6, 0, 0 } } },
  { "instr_misc/rom_singles/03-dummy_reads.nes", { { 0x4D1E0C1E, 34, 0, 0 } } },
  { "instr_misc/rom_singles/04-dummy_reads_apu.nes", { { 0x8808910F, 136, 0, 0 } } },
  { "instr_misc/instr_misc.nes", { { 0x0AE0BB6B, 182, 0, 0 } } },
  { "instr_test-v5/rom_singles/01-basics.nes", { { 0xE654945D, 11, 0, 0 } } },
  { "instr_test-v5/rom_singles/02-implied.nes", { { 0xB2AF2680, 90, 0, 0 } } },
  { "instr_test-v5/rom_singles/03-immediate.nes", { { 0x8CE0845F, 80, 0, 0 } } },
  { "instr_test-v5/rom_singles/04-zero_page.nes", { { 0xE4C3F180, 110, 0, 0 } } },
  { "instr_test-v5/rom_singles/05-zp_xy.nes", { { 0x9EDDA0AE, 254, 0, 0 } } },
  { "instr_test-v5/rom_singles/06-absolute.nes", { { 0x5DFCDDD1, 104, 0, 0 } } },
  { "instr_test-v5/rom_singles/07-abs_xy.nes", { { 0x0067CDD4, 360, 0, 0 } } },
  { "instr_test-v5/rom_singles/08-ind_x.nes", { { 0x17036DB5, 139, 0, 0 } } },
  { "instr_test-v5/rom_singles/09-ind_y.nes", { { 0x9313623A, 131, 0, 0 } } },
  { "instr_test-v5/rom_singles/10-branches.nes", { { 0x43645E68, 34, 0, 0 } } },
  { "instr_test-v5/rom_singles/11-stack.nes", { { 0xB408FAE0, 157, 0, 0 } } },
  { "instr_test-v5/rom_singles/12-jmp_jsr.nes", { { 0x9C8D4591, 10, 0, 0 } } },
  { "instr_test-v5/rom_singles/13-rts.nes", { { 0x1DAAFA1C, 7, 0, 0 } } },
  { "instr_test-v5/rom_singles/14-rti.nes", { { 0x027CCC3C, 8, 0, 0 } } },
  { "instr_test-v5/rom_singles/15-brk.nes", { { 0x6D280E02, 19, 0, 0 } } },
  { "instr_test-v5/rom_singles/16-special.nes", { { 0x1A5209FD, 6, 0, 0 } } },
  { "instr_test-v5/all_instrs.nes", { { 0xB619C5B3, 2210, 0, 0 } } },
  { "instr_timing/rom_singles/1-instr_timing.nes", { { 0xE82C7F11, 1003, 0, 0 } } },
  { "instr_timing/rom_singles/2-branch_timing.nes", { { 0xB8E3CDB8, 133, 0, 0 } } },
  { "instr_timing/instr_timing.nes", { { 0x6F1BAA40, 1284, 0, 0 } } },
  { "MMC1_A12/mmc1_a12.nes", { { 0x382587DA, 25, 0, 0 } } },
  { "mmc3_test_2/rom_singles/1-clocking.nes", { { 0x3D4C2E04, 18, 0, 0 } } },
  { "mmc3_test_2/rom_singles/2-details.nes", { { 0x390FC588, 21, 0, 0 } } },
  { "mmc3_test_2/rom_singles/3-A12_clocking.nes", { { 0xD3189FEE, 17, 0, 0 } } },
  { "mmc3_test_2/rom_singles/4-scanline_timing.nes", { { 0xF6D14A7E, 251, 0, 0 } } },
  { "mmc3_test_2/rom_singles/5-MMC3.nes", { { 0xFAF35560, 18, 0, 0 } } },
  { "nmi_sync/demo_ntsc.nes",
    { { 0xB9F1DFBA, 100, 0, 0 }, { 0x1D8EF9BE, 1, 0, 0 }, { 0xB9F1DFBA, 1, 0, 0 }, { 0x1D8EF9BE, 1, 0, 0 } } },
  // { 0, 0, "nmi_sync/demo_pal.nes" }, // todo
  { "oam_read/oam_read.nes", { { 0xA35A57E7, 17, 0, 0 } } },
  { "oam_stress/oam_stress.nes", { { 0x596023FD, 1690, 0, 0 } } },
  { "other/nestest.nes",
    { { 0x9E8BA5E6, 4, 0, 0 },
      { 0xDF7DA5E2, 16, 0x08, 0 },
      { 0xD6C2CFA3, 10, 0x04, 0 },
      { 0xDEA9BC46, 12, 0x08, 0 } } },
  // { 0, 0, "pal_apu_tests/*.nes" }, // todo
  { "ppu_open_bus/ppu_open_bus.nes", { { 0x3EB4ABE5, 247, 0, 0 } } },
  { "ppu_read_buffer/test_ppu_read_buffer.nes", { { 0xD22D42A2, 1066, 0, 0 } } },
  { "ppu_vbl_nmi/rom_singles/01-vbl_basics.nes", { { 0x1AF3E9A3, 131, 0, 0 } } },
  { "ppu_vbl_nmi/rom_singles/02-vbl_set_time.nes", { { 0x59B9C83E, 125, 0, 0 } } },
  { "ppu_vbl_nmi/rom_singles/03-vbl_clear_time.nes", { { 0x7B0B8BE1, 127, 0, 0 } } },
  { "ppu_vbl_nmi/rom_singles/04-nmi_control.nes", { { 0xCD67F9C1, 18, 0, 0 } } },
  { "ppu_vbl_nmi/rom_singles/05-nmi_timing.nes", { { 0xE19A001A, 173, 0, 0 } } },
  { "ppu_vbl_nmi/rom_singles/06-suppression.nes", { { 0x4D1533CC, 166, 0, 0 } } },
  { "ppu_vbl_nmi/rom_singles/07-nmi_on_timing.nes", { { 0x1658D4F5, 149, 0, 0 } } },
  { "ppu_vbl_nmi/rom_singles/08-nmi_off_timing.nes", { { 0xA3F47CBF, 175, 0, 0 } } },
  { "ppu_vbl_nmi/rom_singles/09-even_odd_frames.nes", { { 0xA590A81C, 52, 0, 0 } } },
  { "ppu_vbl_nmi/rom_singles/10-even_odd_timing.nes", { { 0xB3F44119, 115, 0, 0 } } },
  { "ppu_vbl_nmi/ppu_vbl_nmi.nes", { { 0xDDE50B5E, 1249, 0, 0 } } },
  { "read_joy3/count_errors_fast.nes", { { 0xE8E7148C, 12, 0, 0 } } },
  { "read_joy3/count_errors.nes", { { 0x2B5D8852, 39, 0, 0 } } },
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
      { 0x9C94D0D4, 10, 0, 0 } } },
  { "read_joy3/thorough_test.nes", { { 0x6ECF1F4B, 116, 0, 0 } } },
  { "scanline/scanline.nes",
    { { 0xAF352CF3, 155, 0, 0 },
      { 0xAF352CF3, 1, 0, 0 },
      { 0xAF352CF3, 1, 0, 0 },
      { 0x02718FFD, 1, 0, 0 },
      { 0xAF352CF3, 1, 0, 0 },
      { 0xAF352CF3, 1, 0, 0 },
      { 0xAF352CF3, 1, 0, 0 },
      { 0x02718FFD, 1, 0, 0 } } }, // area1 flickering pixels after line
  { "scanline-a1/scanline.nes",
    { { 0xAF352CF3, 155, 0, 0 },
      { 0xAF352CF3, 1, 0, 0 },
      { 0xAF352CF3, 1, 0, 0 },
      { 0x02718FFD, 1, 0, 0 },
      { 0xAF352CF3, 1, 0, 0 },
      { 0xAF352CF3, 1, 0, 0 },
      { 0xAF352CF3, 1, 0, 0 },
      { 0x02718FFD, 1, 0, 0 } } }, // same as above
  { "scrolltest/scroll.nes", { { 0xCE84FBEF, 7, 0, 0 }, { 0xC7BD0DCE, 60, 0xA0, 0 } } },
  { "sprdma_and_dmc_dma/sprdma_and_dmc_dma.nes", { { 0xF4998922, 122, 0, 0 } } },
  { "sprdma_and_dmc_dma/sprdma_and_dmc_dma_512.nes", { { 0x42149129, 122, 0, 0 } } },
  { "sprite_hit_tests_2005.10.05/01.basics.nes", { { 0xAC38F2AD, 24, 0, 0 } } },
  { "sprite_hit_tests_2005.10.05/02.alignment.nes", { { 0x3BF08842, 21, 0, 0 } } },
  { "sprite_hit_tests_2005.10.05/03.corners.nes", { { 0xC53BD4BF, 11, 0, 0 } } },
  { "sprite_hit_tests_2005.10.05/04.flip.nes", { { 0x0B05D9E8, 13, 0, 0 } } },
  { "sprite_hit_tests_2005.10.05/05.left_clip.nes", { { 0x95D74663, 20, 0, 0 } } },
  { "sprite_hit_tests_2005.10.05/06.right_edge.nes", { { 0x420C5953, 15, 0, 0 } } },
  { "sprite_hit_tests_2005.10.05/07.screen_bottom.nes", { { 0xCE4105CF, 16, 0, 0 } } },
  { "sprite_hit_tests_2005.10.05/08.double_height.nes", { { 0xCE51B630, 13, 0, 0 } } },
  { "sprite_hit_tests_2005.10.05/09.timing_basics.nes", { { 0xE68A1C8B, 52, 0, 0 } } },
  { "sprite_hit_tests_2005.10.05/10.timing_order.nes", { { 0xEBEBD82C, 56, 0, 0 } } },
  { "sprite_hit_tests_2005.10.05/11.edge_timing.nes", { { 0x055D6A29, 33, 0, 0 } } },
  { "sprite_overflow_tests/1.Basics.nes", { { 0x03CC909D, 9, 0, 0 } } },
  { "sprite_overflow_tests/2.Details.nes", { { 0xC833B4E1, 15, 0, 0 } } },
  { "sprite_overflow_tests/3.Timing.nes", { { 0x39154B5F, 105, 0, 0 } } },
  { "sprite_overflow_tests/4.Obscure.nes", { { 0xE2E52CC7, 18, 0, 0 } } },
  { "sprite_overflow_tests/5.Emulator.nes", { { 0xBAFE1B96, 6, 0, 0 } } },
  { "stomper/smwstomp.nes", { { 0x2612E5CC, 300, 0, 0 } } },
  // { 0, 0, "tvpassfail/tv.nes" }, // fail square
  { "vbl_nmi_timing/1.frame_basics.nes", { { 0x509B862D, 173, 0, 0 } } },
  { "vbl_nmi_timing/2.vbl_timing.nes", { { 0xACDD6906, 123, 0, 0 } } },
  { "vbl_nmi_timing/3.even_odd_frames.nes", { { 0xF2D9B6C9, 77, 0, 0 } } },
  { "vbl_nmi_timing/4.vbl_clear_timing.nes", { { 0x3B42F8FA, 96, 0, 0 } } },
  { "vbl_nmi_timing/5.nmi_suppression.nes", { { 0xB2955A03, 129, 0, 0 } } },
  { "vbl_nmi_timing/6.nmi_disable.nes", { { 0xFD58C6AA, 88, 0, 0 } } },
  { "vbl_nmi_timing/7.nmi_timing.nes", { { 0xE5553CE0, 86, 0, 0 } } },
};

static u8 pixels[256 * 240 * 4] = {};
static u8 joy1 = 0;
static u8 joy2 = 0;

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

void save_tga(const char* img_path, const u8* pixels, u16 width, u16 height)
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
  save_tga(img_path, pixels, 256, 240);
}

u32 run_test_step(TestStep step)
{
  joy1 = step.joy1;
  if (step.reset) {
    mn_reset();
  }
  for (u32 i = 0; i < step.frames; ++i) {
    mn_frame();
  }
  return calc_crc(pixels, sizeof(pixels));
}

bool run_test_steps(TestParams params, u32* out_hash)
{
  memset(pixels, 0, sizeof(pixels));
  Rom rom = mn_rom_file(params.path);
  mn_rom_load(rom);
  mn_power();

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
      params.steps[0].frames = 300;
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

  mn_input(&joy1, &joy2);
  mn_output((u32*)pixels);

  u32 count = sizeof(tests) / sizeof(tests[0]);
  u32 errors = 0;
  for (u32 i = 0; i < count; ++i) {
    if (!run_test(tests[i])) {
      ++errors;
    }
  }
  printf("%4s | %8d | TOTAL ERRORS\n", errors ? "FAIL" : "OK", errors);

  return 0;
}
