#include "dev.h"
#include "myanes.h"
#include "cpu.h"
#include "ppu.h"
#include <string.h>

Dev dev;

void mn_load(NES nes) { dev.nes = nes; }

void mn_quit() { dev.quit = true; }

void mn_reset()
{
  cpu_reset();
  ppu_reset();
}

void mn_run()
{
  memset(dev.ram, 0, sizeof(dev.ram));
  memset(dev.vram, 0, sizeof(dev.vram));
  memset(dev.sram, 0, sizeof(dev.sram));
  dev.cpu_cyc = 5;
  dev.ppu_cyc = 15;
  dev.ppu_sl = 0;
  dev.quit = false;
  cpu_init();
  ppu_init();

  while (!dev.quit) {
    cpu_tick();

    if (dev.cpu_cyc > 100000000) {
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
