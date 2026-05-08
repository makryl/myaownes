#include "dev.h"
#include "myanes.h"
#include "cpu.h"
#include "ppu.h"
#include "common.h"
#include <string.h>

Dev dev;

void mn_load(NES nes) { dev.nes = nes; }
void mn_reset() { cpu_reset(); }
void mn_quit() { dev.quit = true; }

void mn_run()
{
  memset(dev.ram, 0, sizeof(dev.ram));
  memset(dev.vram, 0, sizeof(dev.vram));
  memset(dev.pram, 0, sizeof(dev.pram));
  memset(dev.sram, 0, sizeof(dev.sram));
  dev.cpu_cyc = 0;
  dev.ppu_cyc = 0;
  dev.ppu_sl = 0;
  dev.ppu_frame = 0;
  dev.quit = false;
  cpu_init();

#if MN_TRACE_NESTEST
  dev.cpu_cyc = 7;
  dev.ppu_cyc = 21;
#endif

  while (!dev.quit) {
    cpu_tick();

    if (dev.cpu_cyc > 10000000) {
      break;
    }
  }
}

void dev_tick()
{
  ++dev.cpu_cyc;
  ppu_tick();
  ppu_tick();
  ppu_tick();
}
