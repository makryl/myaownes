#include "dev.h"
#include "myanes.h"
#include "cpu.h"
#include "ppu.h"
#include "imp.h"
#include <string.h>

Rom rom;
Dev dev;

void mn_load(Rom rom_) { rom = rom_; }

void mn_power()
{
  memset(&dev, 0, sizeof(dev));
  dev.cpu_cyc = 5;
  dev.ppu_cyc = 15;

  cpu_power();
  ppu_power();
  imp_power();
}

void mn_reset()
{
  cpu_reset();
  ppu_reset();
  imp_reset();
}

void mn_frame()
{
  imp_input();

  dev.vblank = false;
  while (!dev.vblank) {
    cpu_tick();
  }

  imp_render();
}

void dev_tick()
{
  ++dev.cpu_cyc;
  ppu_tick();
  ppu_tick();
  ppu_tick();
}
