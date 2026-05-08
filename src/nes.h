#pragma once

#include "myanes.h"

struct NES
{
  const u8* rom;
  const u8* vrom;
  const u8* eram;
  u8 rom_pages;
  u8 vrom_pages;
  u8 mapper;
  bool vert_mirror;
  bool has_sram;
  bool has_trainer;
  bool has_vram;
};
