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
  u16 irq_counter;
  u8 irq_enabled;
  u8 irq_counter_enabled;
  u8 command;
} map_fme7;

static bool map_fme7_cpu_write(u16 addr, u8 val)
{
  if (addr < 0x8000) {
    return false;
  }
  map_fme7* reg = (map_fme7*)map_reg();
  mn_rom rom = mn_rom_get();
  bool prg_use_ram = false;
  bool prg_ram_disabled = false;
  switch (addr & 0xE000) {
    case 0x8000: reg->command = val & 0x0F; break;
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
          prg_ram_disabled = !(val & 0x80) && (rom->prg_ram_size == 8 * 1024); //
        case 0x9:
        case 0xA:
        case 0xB:
          uint dp = reg->command - 5;
          if (prg_use_ram) {
            if (prg_ram_disabled) {
              map_prg_clear_page_8k(dp);
            } else {
              map_prg_ram_page_8k(val & 0x3F, dp, false);
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
      break;
  }
  return true;
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
  // todo: audio
}

typedef struct
{
  u16 irq_reload_val;
  u16 irq_counter;
  u8 irq_ctrl;
  u8 prg[3];
  u8 chr[8];
} map_jalecoss;

static bool map_jalecoss_cpu_write(u16 addr, u8 val)
{
  if (addr < 0x8000) {
    return false;
  }
  map_jalecoss* reg = (map_jalecoss*)map_reg();
  switch (addr & 0xF003) {
    case 0x8000:
    case 0x8002:
    case 0x9000: {
      uint idx = ((addr & 0x1000) >> 11) | ((addr & 0x02) >> 1);
      reg->prg[idx] = (reg->prg[idx] & 0x30) | (val & 0x0F);
      map_prg_rom_page_8k(reg->prg[idx], idx + 4);
      break;
    }
    case 0x8001:
    case 0x8003:
    case 0x9001: {
      uint idx = ((addr & 0x1000) >> 11) | ((addr & 0x02) >> 1);
      reg->prg[idx] = (reg->prg[idx] & 0x0F) | ((val & 0x03) << 4);
      map_prg_rom_page_8k(reg->prg[idx], idx + 4);
      break;
    }
    case 0x9002: {
      if (val & 1) {
        map_prg_ram_page_8k(val & 0x3F, 3, !(val & 2));
      } else {
        map_prg_clear_page_8k(3);
      }
      break;
    }
    case 0xA000:
    case 0xA002:
    case 0xB000:
    case 0xB002:
    case 0xC000:
    case 0xC002:
    case 0xD000:
    case 0xD002: {
      uint idx = (((addr - 0xA000) & 0x3000) >> 11) | ((addr & 0x02) >> 1);
      reg->chr[idx] = (reg->chr[idx] & 0xF0) | (val & 0x0F);
      map_chr_page_1k(reg->chr[idx], idx);
      break;
    }
    case 0xA001:
    case 0xA003:
    case 0xB001:
    case 0xB003:
    case 0xC001:
    case 0xC003:
    case 0xD001:
    case 0xD003: {
      uint idx = (((addr - 0xA000) & 0x3000) >> 11) | ((addr & 0x02) >> 1);
      reg->chr[idx] = (reg->chr[idx] & 0x0F) | ((val & 0x0F) << 4);
      map_chr_page_1k(reg->chr[idx], idx);
      break;
    }
    case 0xE000: reg->irq_reload_val = (reg->irq_reload_val & 0xFFF0) | (val & 0x0F); break;
    case 0xE001: reg->irq_reload_val = (reg->irq_reload_val & 0xFF0F) | ((val & 0x0F) << 4); break;
    case 0xE002: reg->irq_reload_val = (reg->irq_reload_val & 0xF0FF) | ((val & 0x0F) << 8); break;
    case 0xE003: reg->irq_reload_val = (reg->irq_reload_val & 0x0FFF) | ((val & 0x0F) << 12); break;
    case 0xF000:
      reg->irq_counter = reg->irq_reload_val;
      map_irq(false);
      break;
    case 0xF001:
      reg->irq_ctrl = val;
      map_irq(false);
      break;
    case 0xF002:
      switch (val & 0x03) {
        case 0: map_ppu_nt_horiz_mirror(); break;
        case 1: map_ppu_nt_vert_mirror(); break;
        case 2: map_ppu_nt_single_low(); break;
        case 3: map_ppu_nt_single_high(); break;
      }
      break;
    case 0xF003:
      // todo: audio
      break;
  }
  return true;
}

static void map_jalecoss_cpu_cyc()
{
  map_jalecoss* reg = (map_jalecoss*)map_reg();
  if (reg->irq_ctrl & 0x01) {
    if ((reg->irq_ctrl & 0x08) && (reg->irq_counter & 0x000F) == 0) {
      reg->irq_counter |= 0x000F;
      map_irq(true);
    } else if ((reg->irq_ctrl & 0x04) && (reg->irq_counter & 0x00FF) == 0) {
      reg->irq_counter |= 0x00FF;
      map_irq(true);
    } else if ((reg->irq_ctrl & 0x02) && (reg->irq_counter & 0x0FFF) == 0) {
      reg->irq_counter |= 0x0FFF;
      map_irq(true);
    } else if (reg->irq_counter == 0) {
      reg->irq_counter = 0xFFFF;
      map_irq(true);
    } else {
      --reg->irq_counter;
    }
  }
}

static void map_jalecoss_load()
{
  map_set_cpu_write_cb(map_jalecoss_cpu_write);
  map_set_cpu_cyc_cb(map_jalecoss_cpu_cyc);
}

bool map_other_load()
{
  mn_rom rom = mn_rom_get();
  switch (rom->mapper) {
    case 18: map_jalecoss_load(); break;
    case 28: map_set_cpu_write_cb(map_action53_cpu_write); break;
    case 69: map_fme7_load(); break;
    default: return false;
  }
  return true;
}
