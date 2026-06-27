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
  const char* path;
} TestParams;

static TestParams tests[] = {
  { 0xED10494F, 0x000B, "nes-test-roms/apu_test/rom_singles/1-len_ctr.nes" }, //
  { 0x6E6B072C, 0x0007, "nes-test-roms/apu_test/rom_singles/2-len_table.nes" }, //
  { 0xD963B351, 0x000A, "nes-test-roms/apu_test/rom_singles/3-irq_flag.nes" }, //
  { 0x588FF9E4, 0x000B, "nes-test-roms/apu_test/rom_singles/4-jitter.nes" }, //
  { 0x38AE1E6F, 0x0069, "nes-test-roms/apu_test/rom_singles/5-len_timing.nes" }, //
  { 0x52F44286, 0x000D, "nes-test-roms/apu_test/rom_singles/6-irq_flag_timing.nes" }, //
  { 0xF5D0DECC, 0x000F, "nes-test-roms/apu_test/rom_singles/7-dmc_basics.nes" }, //
  { 0x9A56B616, 0x0014, "nes-test-roms/apu_test/rom_singles/8-dmc_rates.nes" }, //
  { 0x1298619C, 0x00F7, "nes-test-roms/apu_test/apu_test.nes" }, //
  { 0xD588F781, 0x000A, "nes-test-roms/branch_timing_tests/1.Branch_Basics.nes" }, //
  { 0x55F9E054, 0x000B, "nes-test-roms/branch_timing_tests/2.Backward_Branch.nes" }, //
  { 0x0E274BEF, 0x000D, "nes-test-roms/branch_timing_tests/3.Forward_Branch.nes" }, //
  { 0xCA6ED365, 0x001D, "nes-test-roms/cpu_dummy_reads/cpu_dummy_reads.nes" }, //
  { 0x6EC03145, 0x00E6, "nes-test-roms/cpu_dummy_writes/cpu_dummy_writes_oam.nes" }, //
  { 0x620852D4, 0x006D, "nes-test-roms/cpu_dummy_writes/cpu_dummy_writes_ppumem.nes" }, //
  { 0xAAA57ACB, 0x0086, "nes-test-roms/cpu_exec_space/test_cpu_exec_space_apu.nes" }, //
  { 0x52E61283, 0x0015, "nes-test-roms/cpu_exec_space/test_cpu_exec_space_ppuio.nes" }, //
  { 0x67C84C96, 0x0006, "nes-test-roms/cpu_interrupts_v2/rom_singles/1-cli_latency.nes" }, //
  { 0x89110C09, 0x0040, "nes-test-roms/cpu_interrupts_v2/rom_singles/2-nmi_and_brk.nes" }, //
  { 0xD7506A30, 0x004C, "nes-test-roms/cpu_interrupts_v2/rom_singles/3-nmi_and_irq.nes" }, //
  { 0x015458DE, 0x002D, "nes-test-roms/cpu_interrupts_v2/rom_singles/4-irq_and_dma.nes" }, //
  { 0x245C9D12, 0x0125, "nes-test-roms/cpu_interrupts_v2/rom_singles/5-branch_delays_irq.nes" }, //
  { 0x644B46ED, 0x0006, "nes-test-roms/cpu_interrupts_v2/cpu_interrupts.nes" }, //
  { 0x644B46ED, 0x0085, "nes-test-roms/cpu_reset/ram_after_reset.nes" }, //
  { 0x644B46ED, 0x0086, "nes-test-roms/cpu_reset/registers.nes" }, //
  { 0x7C4C95C2, 0x0022, "nes-test-roms/cpu_timing_test6/cpu_timing_test.nes" }, //
  { 0xEE1090AC, 0x000C, "nes-test-roms/dmc_dma_during_read4/dma_2007_read.nes" }, //
  { 0xF73B2028, 0x0010, "nes-test-roms/dmc_dma_during_read4/dma_2007_write.nes" }, //
  { 0x27305F9E, 0x0009, "nes-test-roms/dmc_dma_during_read4/dma_4016_read.nes" }, //
  { 0xE398D1B8, 0x000A, "nes-test-roms/dmc_dma_during_read4/double_2007_read.nes" }, //
  { 0x88B9148A, 0x0008, "nes-test-roms/dmc_dma_during_read4/read_write_2007.nes" }, //
  { 0x58C81044, 0x0064, "nes-test-roms/full_palette/flowing_palette.nes" }, //
  { 0xED57A871, 0x0064, "nes-test-roms/full_palette/full_palette_smooth.nes" }, //
  { 0xBE42182E, 0x0064, "nes-test-roms/full_palette/full_palette.nes" }, //
  { 0x4A20DBAF, 0x0006, "nes-test-roms/instr_misc/rom_singles/01-abs_x_wrap.nes" }, //
  { 0x37286EE0, 0x0006, "nes-test-roms/instr_misc/rom_singles/02-branch_wrap.nes" }, //
  { 0x2E327CB4, 0x0022, "nes-test-roms/instr_misc/rom_singles/03-dummy_reads.nes" }, //
  { 0xEB24E1A5, 0x0088, "nes-test-roms/instr_misc/rom_singles/04-dummy_reads_apu.nes" }, //
  { 0x69CCCBC1, 0x00B6, "nes-test-roms/instr_misc/instr_misc.nes" }, //
  { 0x644B46ED, 0x0059, "nes-test-roms/instr_test-v5/all_instrs.nes" }, //
  { 0x2EA87C1D, 0x000A, "nes-test-roms/instr_timing/rom_singles/1-instr_timing.nes" }, //
  { 0xDBCFBD12, 0x0085, "nes-test-roms/instr_timing/rom_singles/2-branch_timing.nes" }, //
  { 0x644B46ED, 0x0002, "nes-test-roms/instr_timing/instr_timing.nes" }, //
  { 0x5B09F770, 0x0019, "nes-test-roms/MMC1_A12/mmc1_a12.nes" }, //
  { 0x5E605EAE, 0x0012, "nes-test-roms/mmc3_test_2/rom_singles/1-clocking.nes" }, //
  { 0x5A23B522, 0x0015, "nes-test-roms/mmc3_test_2/rom_singles/2-details.nes" }, //
  { 0xB034EF44, 0x0011, "nes-test-roms/mmc3_test_2/rom_singles/3-A12_clocking.nes" }, //
  { 0x95FD3AD4, 0x00FB, "nes-test-roms/mmc3_test_2/rom_singles/4-scanline_timing.nes" }, //
  { 0x99DF25CA, 0x0012, "nes-test-roms/mmc3_test_2/rom_singles/5-MMC3.nes" }, //
  { 0xC076274D, 0x0011, "nes-test-roms/oam_read/oam_read.nes" }, //
  { 0x644B46ED, 0x0003, "nes-test-roms/oam_stress/oam_stress.nes" }, //
  { 0x5D98DB4F, 0x00F7, "nes-test-roms/ppu_open_bus/ppu_open_bus.nes" }, //
  { 0x644B46ED, 0x010F, "nes-test-roms/ppu_read_buffer/test_ppu_read_buffer.nes" }, //
  { 0x79DF9909, 0x0083, "nes-test-roms/ppu_vbl_nmi/rom_singles/01-vbl_basics.nes" }, //
  { 0x3A95B894, 0x007D, "nes-test-roms/ppu_vbl_nmi/rom_singles/02-vbl_set_time.nes" }, //
  { 0x1827FB4B, 0x007F, "nes-test-roms/ppu_vbl_nmi/rom_singles/03-vbl_clear_time.nes" }, //
  { 0xAE4B896B, 0x0012, "nes-test-roms/ppu_vbl_nmi/rom_singles/04-nmi_control.nes" }, //
  { 0x82B670B0, 0x00AD, "nes-test-roms/ppu_vbl_nmi/rom_singles/05-nmi_timing.nes" }, //
  { 0x2E394366, 0x00A6, "nes-test-roms/ppu_vbl_nmi/rom_singles/06-suppression.nes" }, //
  { 0x7574A45F, 0x0095, "nes-test-roms/ppu_vbl_nmi/rom_singles/07-nmi_on_timing.nes" }, //
  { 0xC0D80C15, 0x00AF, "nes-test-roms/ppu_vbl_nmi/rom_singles/08-nmi_off_timing.nes" }, //
  { 0xC6BCD8B6, 0x0034, "nes-test-roms/ppu_vbl_nmi/rom_singles/09-even_odd_frames.nes" }, //
  { 0xD0D831B3, 0x0073, "nes-test-roms/ppu_vbl_nmi/rom_singles/10-even_odd_timing.nes" }, //
  { 0x644B46ED, 0x0006, "nes-test-roms/ppu_vbl_nmi/ppu_vbl_nmi.nes" }, //
  { 0x97B5F988, 0x007A, "nes-test-roms/sprdma_and_dmc_dma/sprdma_and_dmc_dma.nes" }, //
  { 0x2138E183, 0x007A, "nes-test-roms/sprdma_and_dmc_dma/sprdma_and_dmc_dma_512.nes" }, //
  { 0xCF148207, 0x0018, "nes-test-roms/sprite_hit_tests_2005.10.05/01.basics.nes" }, //
  { 0x58DCF8E8, 0x0015, "nes-test-roms/sprite_hit_tests_2005.10.05/02.alignment.nes" }, //
  { 0xA617A415, 0x000B, "nes-test-roms/sprite_hit_tests_2005.10.05/03.corners.nes" }, //
  { 0x6829A942, 0x000D, "nes-test-roms/sprite_hit_tests_2005.10.05/04.flip.nes" }, //
  { 0xF6FB36C9, 0x0014, "nes-test-roms/sprite_hit_tests_2005.10.05/05.left_clip.nes" }, //
  { 0x212029F9, 0x000F, "nes-test-roms/sprite_hit_tests_2005.10.05/06.right_edge.nes" }, //
  { 0xAD6D7565, 0x0010, "nes-test-roms/sprite_hit_tests_2005.10.05/07.screen_bottom.nes" }, //
  { 0xAD7DC69A, 0x000D, "nes-test-roms/sprite_hit_tests_2005.10.05/08.double_height.nes" }, //
  { 0x85A66C21, 0x0034, "nes-test-roms/sprite_hit_tests_2005.10.05/09.timing_basics.nes" }, //
  { 0x88C7A886, 0x0038, "nes-test-roms/sprite_hit_tests_2005.10.05/10.timing_order.nes" }, //
  { 0x66711A83, 0x0021, "nes-test-roms/sprite_hit_tests_2005.10.05/11.edge_timing.nes" }, //
  { 0x60E0E037, 0x0009, "nes-test-roms/sprite_overflow_tests/1.Basics.nes" }, //
  { 0xAB1FC44B, 0x000F, "nes-test-roms/sprite_overflow_tests/2.Details.nes" }, //
  { 0x5A393BF5, 0x0069, "nes-test-roms/sprite_overflow_tests/3.Timing.nes" }, //
  { 0x81C95C6D, 0x0012, "nes-test-roms/sprite_overflow_tests/4.Obscure.nes" }, //
  { 0xD9D26B3C, 0x0006, "nes-test-roms/sprite_overflow_tests/5.Emulator.nes" }, //
  { 0x33B7F687, 0x00AD, "nes-test-roms/vbl_nmi_timing/1.frame_basics.nes" }, //
  { 0xCFF119AC, 0x007B, "nes-test-roms/vbl_nmi_timing/2.vbl_timing.nes" }, //
  { 0x91F5C663, 0x004D, "nes-test-roms/vbl_nmi_timing/3.even_odd_frames.nes" }, //
  { 0x586E8850, 0x0060, "nes-test-roms/vbl_nmi_timing/4.vbl_clear_timing.nes" }, //
  { 0xD1B92AA9, 0x0081, "nes-test-roms/vbl_nmi_timing/5.nmi_suppression.nes" }, //
  { 0x9E74B600, 0x0058, "nes-test-roms/vbl_nmi_timing/6.nmi_disable.nes" }, //
  { 0x86794C4A, 0x0056, "nes-test-roms/vbl_nmi_timing/7.nmi_timing.nes" }, //
  // { 0, 0, "nes-test-roms/apu_mixer/dmc.nes" }, // todo
  // { 0, 0, "nes-test-roms/apu_mixer/noise.nes" }, // todo
  // { 0, 0, "nes-test-roms/apu_mixer/square.nes" }, // todo
  // { 0, 0, "nes-test-roms/apu_mixer/triangle.nes" }, // todo
  // { 0, 0, "nes-test-roms/apu_reset/4015_cleared.nes" }, //
  // { 0, 0, "nes-test-roms/apu_reset/4017_timing.nes" }, //
  // { 0, 0, "nes-test-roms/apu_reset/4017_written.nes" }, //
  // { 0, 0, "nes-test-roms/apu_reset/irq_flag_cleared.nes" }, //
  // { 0, 0, "nes-test-roms/apu_reset/len_ctrs_enabled.nes" }, //
  // { 0, 0, "nes-test-roms/apu_reset/works_immediately.nes" }, //
  // { 0, 0, "nes-test-roms/nmi_sync/demo_ntsc.nes" }, //
  // { 0, 0, "nes-test-roms/nmi_sync/demo_pal.nes" }, // todo
  // { 0, 0, "nes-test-roms/pal_apu_tests/*.nes" }, // todo
  // { 0, 0, "nes-test-roms/read_joy3/count_errors_fast.nes" }, //
  // { 0, 0, "nes-test-roms/read_joy3/count_errors.nes" }, //
  // { 0, 0, "nes-test-roms/read_joy3/test_buttons.nes" }, //
  // { 0, 0, "nes-test-roms/read_joy3/thorough_test.nes" }, //
  // { 0, 0, "nes-test-roms/scanline/scanline.nes" }, // area1 flickering pixels after line
  // { 0, 0, "nes-test-roms/scanline-a1/scanline.nes" }, // same as above
  // { 0, 0, "nes-test-roms/scrolltest/scroll.nes" }, // fixed bottom panel flickers
  // { 0, 0, "nes-test-roms/stomper/smwstomp.nes" }, //
  // { 0, 0, "nes-test-roms/stress/NEStress.nes" }, // ppu has errors, same as mesen
  // { 0, 0, "nes-test-roms/tvpassfail/tv.nes" }, // fail square
};

static u8 pixels[256 * 240 * 4] = {};

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
  header[16] = 24;
  header[17] = 0x20;

  fwrite(header, 1, 18, f);

  for (int i = 0; i < width * height; i++) {
    u8 bgr[3];
    bgr[2] = pixels[i * 4 + 0];
    bgr[1] = pixels[i * 4 + 1];
    bgr[0] = pixels[i * 4 + 2];
    fwrite(bgr, 1, 3, f);
  }

  fclose(f);
}

u32 run_test_frames(const char* rom_path, u32 frames)
{
  memset(pixels, 0, sizeof(pixels));
  mn_output((u32*)pixels);

  u8 joy1 = 0;
  u8 joy2 = 0;
  mn_input(&joy1, &joy2);

  Rom rom = mn_rom_file(rom_path);
  mn_rom_load(rom);
  mn_power();
  for (u32 i = 0; i < frames; ++i) {
    mn_frame();
  }
  mn_rom_release(rom);

  return calc_crc(pixels, sizeof(pixels));
}

void run_test(const char* rom_path, u32 frames, u32 pass_hash)
{
  if (pass_hash && frames) {
    u32 hash = run_test_frames(rom_path, frames);
    printf("%s | %08X | %4d | %s\n", hash == pass_hash ? "PASS" : "FAIL", hash, frames, rom_path);
    return;
  }

  frames = 300;
  pass_hash = run_test_frames(rom_path, frames);

  u32 frames_pass = frames;
  u32 frames_fail = 0;
  do {
    frames = (frames_pass + frames_fail) / 2;
    if (run_test_frames(rom_path, frames) == pass_hash) {
      frames_pass = frames;
    } else {
      frames_fail = frames;
    }
  } while (frames_pass - frames_fail > 1);

  run_test_frames(rom_path, frames_pass);

  char img_path[512] = {};
  strcat(img_path, "tests/");
  strcat(img_path, rom_path);
  size_t img_path_len = strlen(img_path);
  strcpy(img_path + img_path_len - 3, "tga");
  for (size_t i = 0; i <= img_path_len; i++) {
    if (img_path[i] == '/') {
      img_path[i] = 0;
      mkdir(img_path, 0777);
      img_path[i] = '/';
    }
  }
  save_tga(img_path, pixels, 256, 240);

  printf("  { 0x%08X, 0x%04X, \"%s\" }, //\n", pass_hash, frames_pass, rom_path);
}

int main(int, char**)
{
  chdir("../../tmp");

  u32 count = sizeof(tests) / sizeof(tests[0]);
  for (u32 i = 0; i < count; ++i) {
    run_test(tests[i].path, tests[i].frames, tests[i].hash);
  }

  return 0;
}
