#pragma once

#include "myanes.h"

struct Rom
{
  const u8* prg;
  const u8* chr;
  u8 prg_pages;
  u8 chr_pages;
  u8 mapper;
  bool vert_mirror;
  bool has_sram;
  bool has_trainer;
  bool alt_mirror;
};
