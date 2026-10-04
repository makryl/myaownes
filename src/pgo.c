#include "myaownes.h"
#include <stdio.h>

#ifdef __EMSCRIPTEN__
extern int __llvm_profile_write_file(void);
#endif

static const char* base_path = "";

static void run_test(const char* rel_path, uint frames)
{
  char rom_path[512];
  sprintf(rom_path, "%s/%s", base_path, rel_path);

  printf("run %s", rom_path);
  mn_rom rom = mn_rom_load(rom_path, nullptr);
  if (!rom) {
    printf(" fail\n");
    return;
  }
  mn_rom_set(rom);
  for (uint i = 0; i < frames; ++i) {
    mn_frame(0, 0);
  }
  mn_rom_release(rom);
  printf(" done\n");
}

int main(int argc, char** argv)
{
  if (argc < 2) {
    printf("Usage: %s [test-roms-dir]\n", argv[0]);
    return 1;
  }
  base_path = argv[1];

  run_test("blargg_nes_cpu_test5/cpu.nes", 981);
  run_test("apu_test/apu_test.nes", 300);
  run_test("ppu_sprite_hit/ppu_sprite_hit.nes", 584);
  run_test("stomper/smwstomp.nes", 2000);

#ifdef __EMSCRIPTEN__
  __llvm_profile_write_file();
#endif

  return 0;
}
