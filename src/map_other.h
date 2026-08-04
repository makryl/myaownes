#pragma once

#include "map_internal.h"

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
static_assert(sizeof(map_action53) <= MAP_REG_SIZE);

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
static_assert(sizeof(map_fme7) <= MAP_REG_SIZE);

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
static_assert(sizeof(map_jalecoss) <= MAP_REG_SIZE);

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

typedef struct
{
  u16 prescaler;
  u8 ctrl;
  u8 latch;
  u8 counter;
} map_vrc_irq;

static void map_vrc_irq_ctrl(u8 val)
{
  map_vrc_irq* reg = (map_vrc_irq*)map_reg();
  reg->ctrl = val;
  if (val & 0x02) {
    reg->counter = reg->latch;
  }
  map_irq(false);
  reg->prescaler = 0;
}

static void map_vrc_irq_ack()
{
  map_vrc_irq* reg = (map_vrc_irq*)map_reg();
  reg->ctrl = (reg->ctrl & 0x05) | ((reg->ctrl & 0x01) << 1);
  map_irq(false);
}

static void map_vrc_cpu_cyc()
{
  map_vrc_irq* reg = (map_vrc_irq*)map_reg();
  if (reg->ctrl & 0x02) {
    bool clock = false;
    if (reg->ctrl & 0x04) {
      clock = true;
    } else {
      reg->prescaler += 3;
      if (reg->prescaler >= 341) {
        reg->prescaler -= 341;
        clock = true;
      }
    }
    if (clock) {
      if (reg->counter == 0xFF) {
        reg->counter = reg->latch;
        map_irq(true); // todo: probably should trig later
      } else {
        ++reg->counter;
      }
    }
  }
}

typedef struct
{
  map_vrc_irq irq;
  u8 is_vrc4;
  u8 is_vrc2a;
  u8 a0_mask;
  u8 a1_mask;
  u8 prg_ctrl;
  u8 microwire_latch;
  u8 prg0;
  u8 prg1;
  u16 chr[8];
} map_vrc2_vrc4;
static_assert(sizeof(map_vrc2_vrc4) <= MAP_REG_SIZE);

static void map_vrc2_vrc4_update_prg()
{
  map_vrc2_vrc4* reg = (map_vrc2_vrc4*)map_reg();
  if (reg->is_vrc4) {
    if (reg->prg_ctrl & 0x01) {
      map_prg_ram_page_8k(0, 3, false);
    } else {
      map_prg_clear_page_8k(3);
    }
    if (reg->prg_ctrl & 0x02) {
      map_prg_rom_page_8k(-2, 4);
      map_prg_rom_page_8k(reg->prg1, 5);
      map_prg_rom_page_8k(reg->prg0, 6);
    } else {
      map_prg_rom_page_8k(reg->prg0, 4);
      map_prg_rom_page_8k(reg->prg1, 5);
      map_prg_rom_page_8k(-2, 6);
    }
  } else {
    map_prg_rom_page_8k(reg->prg0, 4);
    map_prg_rom_page_8k(reg->prg1, 5);
  }
}

static bool map_vrc2_vrc4_cpu_write(u16 addr, u8 val)
{
  map_vrc2_vrc4* reg = (map_vrc2_vrc4*)map_reg();
  if (!reg->is_vrc4 && addr >= 0x6000 && addr <= 0x6FFF) {
    reg->microwire_latch = (val & 1);
    return true;
  }
  if (addr < 0x8000) {
    return false;
  }
  uint a0 = (addr & reg->a0_mask) ? 1 : 0;
  uint a1 = (addr & reg->a1_mask) ? 2 : 0;
  addr = (addr & 0xF000) | a1 | a0;
  switch (addr) {
    case 0x8000:
    case 0x8001:
    case 0x8002:
    case 0x8003:
      reg->prg0 = (val & 0x1F);
      map_vrc2_vrc4_update_prg();
      break;
    case 0xA000:
    case 0xA001:
    case 0xA002:
    case 0xA003:
      reg->prg1 = (val & 0x1F);
      map_vrc2_vrc4_update_prg();
      break;
    case 0x9002:
      if (reg->is_vrc4) {
        reg->prg_ctrl = val;
        map_vrc2_vrc4_update_prg();
        break;
      }
    case 0x9001:
    case 0x9003:
      if (reg->is_vrc4) {
        break;
      }
    case 0x9000:
      switch (val & (reg->is_vrc4 ? 0x03 : 0x01)) {
        case 0: map_ppu_nt_vert_mirror(); break;
        case 1: map_ppu_nt_horiz_mirror(); break;
        case 2: map_ppu_nt_single_low(); break;
        case 3: map_ppu_nt_single_high(); break;
      }
      break;
    case 0xB000:
    case 0xB002:
    case 0xC000:
    case 0xC002:
    case 0xD000:
    case 0xD002:
    case 0xE000:
    case 0xE002: {
      uint idx = (((addr - 0xB000) & 0x3000) >> 11) | ((addr & 0x02) >> 1);
      if (reg->is_vrc2a) {
        val >>= 1;
      }
      reg->chr[idx] = (reg->chr[idx] & 0xFFF0) | (val & 0x0F);
      map_chr_page_1k(reg->chr[idx], idx);
      break;
    }
    case 0xB001:
    case 0xB003:
    case 0xC001:
    case 0xC003:
    case 0xD001:
    case 0xD003:
    case 0xE001:
    case 0xE003: {
      uint idx = (((addr - 0xB000) & 0x3000) >> 11) | ((addr & 0x02) >> 1);
      if (reg->is_vrc2a) {
        val >>= 1;
      }
      reg->chr[idx] = (reg->chr[idx] & 0x000F) | ((val & (reg->is_vrc4 ? 0x1F : 0x0F)) << 4);
      map_chr_page_1k(reg->chr[idx], idx);
      break;
    }
    case 0xF000: reg->irq.latch = (reg->irq.latch & 0xF0) | (val & 0x0F); break;
    case 0xF001: reg->irq.latch = (reg->irq.latch & 0x0F) | ((val & 0x0F) << 4); break;
    case 0xF002: map_vrc_irq_ctrl(val); break;
    case 0xF003: map_vrc_irq_ack(); break;
  }
  return true;
}

static bool map_vrc2_cpu_read(u16 addr, u8* val, bool)
{
  if (addr >= 0x6000 && addr <= 0x6FFF) {
    map_vrc2_vrc4* reg = (map_vrc2_vrc4*)map_reg();
    *val = ((*val) & 0xFE) | reg->microwire_latch;
    return true;
  }
  return false;
}

static void map_vrc2_vrc4_load()
{
  mn_rom rom = mn_rom_get();
  map_vrc2_vrc4* reg = (map_vrc2_vrc4*)map_reg();
  reg->is_vrc4 = (rom->submapper < 3);
  uint a0_lo = 0;
  uint a1_lo = 0;
  uint a0_hi = 0;
  uint a1_hi = 0;
  switch (rom->mapper) {
    case 21:
      a0_lo = 0x02;
      a1_lo = 0x04;
      a0_hi = 0x40;
      a1_hi = 0x80;
      break;
    case 22:
      reg->is_vrc4 = false;
      reg->is_vrc2a = true;
      a0_lo = 0x02;
      a1_lo = 0x01;
      break;
    case 23:
      a0_lo = 0x01;
      a1_lo = 0x02;
      a0_hi = 0x04;
      a1_hi = 0x08;
      break;
    case 25:
      a0_lo = 0x02;
      a1_lo = 0x01;
      a0_hi = 0x08;
      a1_hi = 0x04;
      break;
    case 27: // clone VRC4f 23-1
      a0_lo = 0x01;
      a1_lo = 0x02;
      break;
  }
  if (rom->submapper) {
    if (rom->submapper & 1) {
      a0_hi = 0;
      a1_hi = 0;
    } else {
      a0_lo = 0;
      a1_lo = 0;
    }
  }
  reg->a0_mask = (a0_hi | a0_lo);
  reg->a1_mask = (a1_hi | a1_lo);
  if (reg->is_vrc4) {
    map_set_cpu_cyc_cb(map_vrc_cpu_cyc);
  } else {
    map_set_cpu_read_cb(map_vrc2_cpu_read);
  }
  map_set_cpu_write_cb(map_vrc2_vrc4_cpu_write);
}

static void map_vrc1_load()
{
  // todo
}

static void map_vrc3_load()
{
  // todo
}

static void map_vrc6_load()
{
  // todo
}

static void map_vrc7_load()
{
  // todo
}

bool map_other_load()
{
  mn_rom rom = mn_rom_get();
  switch (rom->mapper) {
    case 18: map_jalecoss_load(); break;
    case 28: map_set_cpu_write_cb(map_action53_cpu_write); break;
    case 69: map_fme7_load(); break;
    case 21:
    case 22:
    case 23:
    case 25:
    case 27: map_vrc2_vrc4_load(); break;
    case 24:
    case 26: map_vrc6_load(); break;
    case 73: map_vrc3_load(); break;
    case 75: map_vrc1_load(); break;
    case 85: map_vrc7_load(); break;
    default: return false;
  }
  return true;
}
