#pragma once

#include "myanes.h"

enum NESFlag : u8
{
  NES_FLAG_MIRROR = (1 << 0),
  NES_FLAG_BATTERY = (1 << 1),
  NES_FLAG_TRAINER = (1 << 2),
  NES_FLAG_VRAM = (1 << 3),
};

struct NES
{
  const void* rom;
  const void* vrom;
  const void* eram;
  u8 rom_pages;
  u8 vrom_pages;
  u8 flags;
  u8 mapper;
};
