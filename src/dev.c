#include "dev.h"
#include "myanes.h"
#include "cpu.h"
#include "ppu.h"
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
}

void mn_reset()
{
  cpu_reset();
  ppu_reset();
}

void* mn_frame()
{
  dev.vblank = false;
  while (!dev.vblank) {
    cpu_tick();
  }
  return dev.quit ? 0 : dev.screen;
}

void dev_tick()
{
  ++dev.cpu_cyc;
  ppu_tick();
  ppu_tick();
  ppu_tick();
}
