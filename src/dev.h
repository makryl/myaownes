#pragma once

#include "myanes.h"

typedef struct
{
  NES nes;
  u8 ram[0x0800];
  u8 vram[0x0800];
  u8 sram[0x2000];
} Dev;

extern Dev dev;
