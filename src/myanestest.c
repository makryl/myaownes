#include "myanes.h"

int main(int, const char**)
{
  // NES nes = mn_nes_file("../../data/nestest.nes");
  NES nes = mn_nes_file("../../data/ppu_vbl_nmi.nes");
  if (!nes) {
    return 1;
  }
  mn_load(nes);
  mn_run();
  mn_nes_release(nes);
  return 0;
}
