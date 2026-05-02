#pragma once

#include "common.h"

enum DeviceFlag : u8
{
  NES_FLAG_MIRROR = (1 << 0),
  NES_FLAG_BATTERY = (1 << 1),
  NES_FLAG_TRAINER = (1 << 2),
  NES_FLAG_VRAM = (1 << 3),
};

typedef struct
{
  const void* rom;
  const void* vrom;
  const void* eram;
  u8 ram[0x0800];
  u8 vram[0x0800];
  u8 sram[0x2000];
  u8 rom_pages;
  u8 vrom_pages;
  u8 flags;
  u8 mapper;
} Device;

extern Device device;
