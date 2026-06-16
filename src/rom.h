#pragma once

#include "myanes.h"

struct Rom
{
  const u8* prg_rom;
  const u8* chr_rom;
  u32 prg_rom_size;
  u32 chr_rom_size;
  u32 prg_ram_size;
  u32 chr_ram_size;
  u16 mapper;
  u8 submapper;
  u8 hw_type;
  bool vert_mirror;
  bool has_prg_battery;
  bool has_chr_battery;
  bool has_trainer;
  bool alt_mirror;
  bool ntsc;
  bool pal;
  bool dendy;
};
