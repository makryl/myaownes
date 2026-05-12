#include "myanes.h"

int main(int, const char**)
{
  // Rom rom = mn_nes_file("../../data/nestest.nes");
  Rom rom = mn_nes_file("../../data/ppu_vbl_nmi.nes");
  if (!rom) {
    return 1;
  }
  mn_load(rom);
  mn_power();
  for (int i = 0; i < 1000; i++) {
    mn_frame();
  }
  mn_nes_release(rom);
  return 0;
}
