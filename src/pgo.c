#include "myaownes.h"
#include <stdio.h>

#ifdef __EMSCRIPTEN__
extern int __llvm_profile_write_file(void);
#endif

static void run_test(const char* rom_path, uint frames)
{
  printf("run %s\n", rom_path);
  mn_rom rom = mn_rom_load(rom_path, nullptr);
  if (!rom) {
    printf("fail\n");
    return;
  }
  mn_rom_set(rom);
  for (uint i = 0; i < frames; ++i) {
    mn_frame(0, 0);
  }
  mn_rom_release(rom);
  printf("done\n");
}

int main(int, char**)
{
  run_test("external/nes-test-roms/CPU/blargg_nes_cpu_test5/cpu.nes", 981);
  run_test("external/nes-test-roms/APU/apu_test/apu_test.nes", 300);
  run_test("external/nes-test-roms/PPU/ppu_sprite_hit/ppu_sprite_hit.nes", 584);
  run_test("external/nes-test-roms/Demos/stomper/smwstomp.nes", 2000);

#ifdef __EMSCRIPTEN__
  __llvm_profile_write_file();
#endif

  return 0;
}
