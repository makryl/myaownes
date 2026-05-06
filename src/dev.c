#include "dev.h"
#include "myanes.h"
#include "cpu.h"
#include "ppu.h"
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
  cpu_init();
  dev.cpu_cyc = 5;
  dev.ppu_cyc = 15;
  dev.ppu_sl = 0;
  dev.ppu_frame = 0;

  while (!dev.quit) {
    int cpu_cyc = dev.cpu_cyc;
    cpu_tick();
    int ppu_cyc = (dev.cpu_cyc - cpu_cyc) * 3;
    for (int i = 0; i < ppu_cyc; ++i) {
      ppu_tick();
    }

    if (dev.cpu_cyc > 26554) {
      break;
    }
  }
}
