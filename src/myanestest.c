#include "myanes.h"

int main(int, const char**)
{
  NES nes = mn_nes_file("../../data/nestest.nes");
  if (!nes) {
    return 1;
  }
  mn_load(nes);
  mn_run();
  mn_nes_release(nes);
  return 0;
}
