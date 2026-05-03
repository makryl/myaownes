#include "dev.h"
#include "myanes.h"
#include "cpu.h"
#include "ppu.h"

Dev dev;

void mn_load(NES nes) { dev.nes = nes; }
void mn_reset() { cpu_reset(); }

void mn_tick()
{
  ppu_tick();
  ppu_tick();
  ppu_tick();
  cpu_tick();
}
