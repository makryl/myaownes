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

typedef struct
{
  u8 command;
  u8 irq_enabled;
  u8 irq_counter_enabled;
  u16 irq_counter;
} map_fme7;

static bool map_fme7_cpu_write(u16 addr, u8 val)
{
  map_fme7* reg = (map_fme7*)map_reg();
  mn_rom rom = mn_rom_get();
  bool prg_use_ram = false;
  bool prg_ram_protect = false;
  switch (addr & 0xE000) {
    case 0x8000: reg->command = val & 0x0F; return true;
    case 0xA000:
      switch (reg->command) {
        case 0x0:
        case 0x1:
        case 0x2:
        case 0x3:
        case 0x4:
        case 0x5:
        case 0x6:
        case 0x7: map_chr_page_1k(val, reg->command); break;
        case 0x8:
          prg_use_ram = (val & 0x40); //
          prg_ram_protect = !(val & 0x80) && (rom->prg_ram_size == 8 * 1024); //
        case 0x9:
        case 0xA:
        case 0xB:
          uint dp = reg->command - 5;
          if (prg_use_ram) {
            if (prg_ram_protect) {
              map_prg_clear_page_8k(dp);
            } else {
              map_prg_ram_page_8k(val & 0x3F, dp);
            }
          } else {
            map_prg_rom_page_8k(val & 0x3F, dp);
          }
          break;
        case 0xC:
          switch (val & 0x03) {
            case 0: map_ppu_nt_vert_mirror(); break;
            case 1: map_ppu_nt_horiz_mirror(); break;
            case 2: map_ppu_nt_single_low(); break;
            case 3: map_ppu_nt_single_high(); break;
          }
          break;
        case 0xD:
          reg->irq_counter_enabled = (val >> 7);
          reg->irq_enabled = (val & 1);
          map_irq(false);
          break;
        case 0xE: reg->irq_counter = (reg->irq_counter & 0xFF00) | val; break;
        case 0xF: reg->irq_counter = (reg->irq_counter & 0x00FF) | (val << 8); break;
      }
      return true;
  }
  return false;
}

static void map_fme7_cpu_cyc()
{
  map_fme7* reg = (map_fme7*)map_reg();
  if (reg->irq_counter_enabled) {
    if (--reg->irq_counter == 0xFFFF) {
      if (reg->irq_enabled) {
        map_irq(true);
      }
    }
  }
}

static void map_fme7_load()
{
  map_set_cpu_cyc_cb(map_fme7_cpu_cyc);
  map_set_cpu_write_cb(map_fme7_cpu_write);
}

bool map_other_load()
{
  mn_rom rom = mn_rom_get();
  switch (rom->mapper) {
    case 28: map_set_cpu_write_cb(map_action53_cpu_write); break;
    case 69: map_fme7_load(); break;
    default: return false;
  }
  return true;
}
