#pragma once

#include "map_internal.h"
#include "map_discrete.h"

typedef struct
{
  u8 select;
  u8 mirror_mode;
  u8 bank_mode;
  u8 bank_mask;
  u8 outer_bank;
  u8 prg_page;
  u8 chr_page;
} map_action53;

static void map_action53_update()
{
  map_action53* reg = (map_action53*)map_reg();
  switch (reg->mirror_mode) {
    case 0: map_ppu_nt_single_low(); break;
    case 1: map_ppu_nt_single_high(); break;
    case 2: map_ppu_nt_vert_mirror(); break;
    case 3: map_ppu_nt_horiz_mirror(); break;
  }
  if (reg->bank_mode < 2) {
    map_prg_rom_page_32k((reg->outer_bank & ~reg->bank_mask) | (reg->prg_page & reg->bank_mask), 1);
  } else {
    uint mask_16k = ((reg->bank_mask << 1) | 1);
    uint outer_16k = (reg->outer_bank << 1);
    uint outer_masked_16k = (outer_16k & ~mask_16k);
    uint page_masked_16k = (reg->prg_page & mask_16k);
    if (reg->bank_mode == 2) {
      map_prg_rom_page_16k(outer_16k | 0, 2);
      map_prg_rom_page_16k(outer_masked_16k | page_masked_16k, 3);
    } else {
      map_prg_rom_page_16k(outer_masked_16k | page_masked_16k, 2);
      map_prg_rom_page_16k(outer_16k | 1, 3);
    }
  }
  map_chr_page_8k(reg->chr_page, 0);
}

static bool map_action53_cpu_write(u16 addr, u8 val)
{
  map_action53* reg = (map_action53*)map_reg();
  if ((addr & 0xF000) == 0x5000) {
    reg->select = (val & 0x81);
    return true;
  } else if ((addr & 0x8000) == 0x8000) {
    if (reg->select == 0x80) {
      reg->mirror_mode = (val & 0x03);
      reg->bank_mode = (val & 0x0C) >> 2;
      uint outer_size = (val & 0x30) >> 4;
      reg->bank_mask = (1 << outer_size) - 1;
      map_action53_update();
    } else if (reg->select == 0x81) {
      reg->outer_bank = val;
      map_action53_update();
    } else {
      if ((reg->mirror_mode & 2) == 0) {
        reg->mirror_mode = (val & 0x10) >> 4;
      }
      if (reg->select) {
        reg->prg_page = (val & 0x0F);
      } else {
        reg->chr_page = (val & 0x03);
      }
      map_action53_update();
    }
    return true;
  }
  return false;
}

bool map_other_load()
{
  mn_rom rom = mn_rom_get();
  switch (rom->mapper) {
    case 28: map_set_cpu_write_cb(map_action53_cpu_write); break;
    default: return false;
  }
  return true;
}
