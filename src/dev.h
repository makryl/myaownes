#pragma once

#include "myanes.h"

typedef struct
{
  u8 ram[0x0800];
  u8 vram[0x1000];
  u8 eram[0x2000];
  u8 sram[0x2000];
  int cpu_cyc;
  int ppu_cyc;
  int ppu_sl;
  bool vblank;
  bool quit;
} Dev;

extern Rom rom;
extern Dev dev;

void dev_tick();
