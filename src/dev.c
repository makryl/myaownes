#include "dev.h"
#include "myanes.h"
#include "cpu.h"
#include "ppu.h"
#include <string.h>

Dev dev;

void mn_load(NES nes) { dev.nes = nes; }

void mn_power()
{
  memset(dev.ram, 0, sizeof(dev.ram));
  memset(dev.vram, 0, sizeof(dev.vram));
  memset(dev.sram, 0, sizeof(dev.sram));
  dev.cpu_cyc = 5;
  dev.ppu_cyc = 15;
  dev.ppu_sl = 0;
  dev.vblank = false;
  cpu_power();
  ppu_power();
}

void mn_reset()
{
  cpu_reset();
  ppu_reset();
}

void mn_frame()
{
  // todo: impl input

  dev.vblank = false;
  while (!dev.vblank) {
    cpu_tick();
  }

  // todo: impl render
}

void dev_tick()
{
  ++dev.cpu_cyc;
  ppu_tick();
  ppu_tick();
  ppu_tick();
}
