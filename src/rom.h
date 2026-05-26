#pragma once

#include "myanes.h"

struct Rom
{
  const u8* prg;
  const u8* chr;
  u32 prg_size;
  u32 chr_size;
  u32 prg_ram_size;
  u32 chr_ram_size;
  u32 prg_sram_size;
  u32 chr_sram_size;
  u16 mapper;
  u8 hw_type;
  bool vert_mirror;
  bool has_sram;
  bool has_trainer;
  bool alt_mirror;
  bool ntsc;
  bool pal;
  bool dendy;
};
