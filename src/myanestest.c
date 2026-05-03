#include "myanes.h"

int main(int, const char**)
{
  NES nes = mn_nes_file("../../data/nestest.nes");
  if (!nes) {
    return 1;
  }
  mn_load(nes);
  mn_reset();
  for (int i = 0; i < 8991; ++i) {
    mn_tick();
  };
  mn_nes_release(nes);
  return 0;
}
