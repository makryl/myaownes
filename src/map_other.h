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
    case 0: map_ciram_single_low(); break;
    case 1: map_ciram_single_high(); break;
    case 2: map_ciram_vert_mirror(); break;
    case 3: map_ciram_horiz_mirror(); break;
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
  u8 irq_latch;
  u8 irq_enabled;
  u8 command;
} map_sunsoft3;
static_assert(sizeof(map_sunsoft3) <= MAP_REG_SIZE);

static bool map_sunsoft3_cpu_write(u16 addr, u8 val)
{
  if (addr < 0x8000) {
    return false;
  }
  map_sunsoft3* reg = (map_sunsoft3*)map_reg();
  bool irq_ack = (addr & 0x8800);
  switch (addr & 0xF800) {
    case 0x8800: map_chr_page_2k(val & 0x3F, 0); break;
    case 0x9800: map_chr_page_2k(val & 0x3F, 1); break;
    case 0xA800: map_chr_page_2k(val & 0x3F, 2); break;
    case 0xB800: map_chr_page_2k(val & 0x3F, 3); break;
    case 0xC800:
      if (reg->irq_latch) {
        reg->irq_counter = (reg->irq_counter & 0xFF00) | val;
        reg->irq_latch = 0;
      } else {
        reg->irq_counter = (reg->irq_counter & 0x00FF) | (val << 8);
        reg->irq_latch = 1;
      }
      break;
    case 0xD800:
      reg->irq_enabled = (val & 0x10);
      irq_ack = false;
      break;
    case 0xE800:
      switch (val & 3) {
        case 0: map_ciram_vert_mirror(); break;
        case 1: map_ciram_horiz_mirror(); break;
        case 2: map_ciram_single_low(); break;
        case 3: map_ciram_single_high(); break;
      }
      break;
    case 0xF800: map_prg_rom_page_16k(val & 0x0F, 2); break;
  }
  if (irq_ack) {
    map_irq(false);
  }
  return true;
}

static void map_sunsoft3_cpu_cyc()
{
  map_sunsoft3* reg = (map_sunsoft3*)map_reg();
  if (reg->irq_enabled) {
    if (reg->irq_counter == 0) {
      reg->irq_counter = 0xFFFF;
      reg->irq_enabled = 0;
      reg->irq_latch = 0;
      map_irq(true);
    } else {
      --reg->irq_counter;
    }
  }
}

static void map_sunsoft3_load()
{
  map_set_cpu_cyc_cb(map_sunsoft3_cpu_cyc);
  map_set_cpu_write_cb(map_sunsoft3_cpu_write);
}

typedef struct
{
  uint timer;
  u8 has_prg_ext;
  u8 prg_ext;
  u8 wram_enabled;
  u8 ctrl;
  u8 nt[2];
} map_sunsoft4;
static_assert(sizeof(map_sunsoft4) <= MAP_REG_SIZE);

static void map_sunsoft4_nt_update()
{
  map_sunsoft4* reg = (map_sunsoft4*)map_reg();
  bool use_chr = (reg->ctrl & 0x10);
  uint sp0 = use_chr ? reg->nt[0] : 0;
  uint sp1 = use_chr ? reg->nt[1] : 1;
  switch (reg->ctrl & 3) {
    case 0:
      map_nt_page(sp0, 0x8, use_chr);
      map_nt_page(sp1, 0x9, use_chr);
      map_nt_page(sp0, 0xA, use_chr);
      map_nt_page(sp1, 0xB, use_chr);
      break;
    case 1:
      map_nt_page(sp0, 0x8, use_chr);
      map_nt_page(sp0, 0x9, use_chr);
      map_nt_page(sp1, 0xA, use_chr);
      map_nt_page(sp1, 0xB, use_chr);
      break;
    case 2:
      map_nt_page(sp0, 0x8, use_chr);
      map_nt_page(sp0, 0x9, use_chr);
      map_nt_page(sp0, 0xA, use_chr);
      map_nt_page(sp0, 0xB, use_chr);
      break;
    case 3:
      map_nt_page(sp1, 0x8, use_chr);
      map_nt_page(sp1, 0x9, use_chr);
      map_nt_page(sp1, 0xA, use_chr);
      map_nt_page(sp1, 0xB, use_chr);
      break;
  }
}

static bool map_sunsoft4_cpu_read(u16 addr, u8*, bool)
{
  if (addr >= 0x8000 && addr <= 0xBFFF) {
    map_sunsoft4* reg = (map_sunsoft4*)map_reg();
    if (reg->prg_ext && !reg->timer) {
      return true;
    }
  }
  return false;
}

static bool map_sunsoft4_cpu_write(u16 addr, u8 val)
{
  map_sunsoft4* reg = (map_sunsoft4*)map_reg();
  if (!reg->wram_enabled && addr >= 0x6000 && addr < 0x7FFF) {
    reg->timer = 1024 * 105;
    return true;
  }
  if (addr < 0x8000) {
    return false;
  }
  switch (addr & 0xF000) {
    case 0x8000: map_chr_page_2k(val, 0); break;
    case 0x9000: map_chr_page_2k(val, 1); break;
    case 0xA000: map_chr_page_2k(val, 2); break;
    case 0xB000: map_chr_page_2k(val, 3); break;
    case 0xC000:
      reg->nt[0] = (val | 0x80);
      map_sunsoft4_nt_update();
      break;
    case 0xD000:
      reg->nt[1] = (val | 0x80);
      map_sunsoft4_nt_update();
      break;
    case 0xE000:
      reg->ctrl = val;
      map_sunsoft4_nt_update();
      break;
    case 0xF000: {
      reg->wram_enabled = (val & 0x10);
      if (reg->wram_enabled) {
        map_prg_ram_page_8k(0, 3, false);
      } else {
        map_prg_clear_page_8k(3);
      }
      uint prg_page = 0;
      if (reg->has_prg_ext) {
        reg->prg_ext = !(val & 0x08);
        prg_page = (val & 0x07) | (reg->prg_ext ? 0x08 : 0);
      } else {
        prg_page = (val & 0x0F);
      }
      map_prg_rom_page_16k(prg_page, 2);
      break;
    }
  }
  return true;
}

static void map_sunsoft4_cpu_cyc()
{
  map_sunsoft4* reg = (map_sunsoft4*)map_reg();
  if (reg->timer) {
    --reg->timer;
  }
}

static void map_sunsoft4_load()
{
  map_sunsoft4* reg = (map_sunsoft4*)map_reg();
  mn_rom rom = mn_rom_get();
  reg->has_prg_ext = (rom->submapper == 1);
  if (reg->has_prg_ext) {
    map_prg_rom_page_16k(0x07, 3);
    map_set_cpu_cyc_cb(map_sunsoft4_cpu_cyc);
    map_set_cpu_read_cb(map_sunsoft4_cpu_read);
  }
  map_set_cpu_write_cb(map_sunsoft4_cpu_write);
}

typedef struct
{
  u16 irq_counter;
  u8 irq_enabled;
  u8 irq_counter_enabled;
  u8 command;
} map_sunsoft_fme7;
static_assert(sizeof(map_sunsoft_fme7) <= MAP_REG_SIZE);

static bool map_sunsoft_fme7_cpu_write(u16 addr, u8 val)
{
  if (addr < 0x8000) {
    return false;
  }
  map_sunsoft_fme7* reg = (map_sunsoft_fme7*)map_reg();
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
            case 0: map_ciram_vert_mirror(); break;
            case 1: map_ciram_horiz_mirror(); break;
            case 2: map_ciram_single_low(); break;
            case 3: map_ciram_single_high(); break;
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

static void map_sunsoft_fme7_cpu_cyc()
{
  map_sunsoft_fme7* reg = (map_sunsoft_fme7*)map_reg();
  if (reg->irq_counter_enabled) {
    if (reg->irq_counter == 0) {
      reg->irq_counter = 0xFFFF;
      if (reg->irq_enabled) {
        map_irq(true);
      }
    } else {
      --reg->irq_counter;
    }
  }
}

static void map_sunsoft_fme7_load()
{
  map_set_cpu_cyc_cb(map_sunsoft_fme7_cpu_cyc);
  map_set_cpu_write_cb(map_sunsoft_fme7_cpu_write);
  // todo: audio
}

typedef struct
{
  u16 irq_reload_val;
  u16 irq_counter;
  u8 irq_ctrl;
  u8 prg[3];
  u8 chr[8];
} map_jaleco_ss;
static_assert(sizeof(map_jaleco_ss) <= MAP_REG_SIZE);

static bool map_jaleco_ss_cpu_write(u16 addr, u8 val)
{
  if (addr < 0x8000) {
    return false;
  }
  map_jaleco_ss* reg = (map_jaleco_ss*)map_reg();
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
        case 0: map_ciram_horiz_mirror(); break;
        case 1: map_ciram_vert_mirror(); break;
        case 2: map_ciram_single_low(); break;
        case 3: map_ciram_single_high(); break;
      }
      break;
    case 0xF003:
      // todo: audio
      break;
  }
  return true;
}

static void map_jaleco_ss_cpu_cyc()
{
  map_jaleco_ss* reg = (map_jaleco_ss*)map_reg();
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

static void map_jaleco_ss_load()
{
  map_set_cpu_write_cb(map_jaleco_ss_cpu_write);
  map_set_cpu_cyc_cb(map_jaleco_ss_cpu_cyc);
}

typedef struct
{
  u16 prescaler;
  u16 latch;
  u16 counter;
  u8 ctrl;
  u8 vrc3;
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
    if (reg->vrc3 || (reg->ctrl & 0x04)) {
      clock = true;
    } else {
      reg->prescaler += 3;
      if (reg->prescaler >= 341) {
        reg->prescaler -= 341;
        clock = true;
      }
    }
    if (clock) {
      uint mask = (reg->vrc3 && (reg->ctrl & 0x04) == 0) ? 0xFFFF : 0xFF;
      if ((reg->counter & mask) == (0xFFFF & mask)) {
        reg->counter = (reg->latch & mask);
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
        case 0: map_ciram_vert_mirror(); break;
        case 1: map_ciram_horiz_mirror(); break;
        case 2: map_ciram_single_low(); break;
        case 3: map_ciram_single_high(); break;
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

typedef struct
{
  map_vrc_irq irq;
  u8 a0_mask;
  u8 a1_mask;
  u8 ctrl;
  u8 prg0;
  u8 prg1;
  u16 chr[8];
} map_vrc6;
static_assert(sizeof(map_vrc6) <= MAP_REG_SIZE);

static void map_vrc6_update()
{
  map_vrc6* reg = (map_vrc6*)map_reg();
  if (reg->ctrl & 0x80) {
    map_prg_ram_page_8k(0, 3, false);
  } else {
    map_prg_clear_page_8k(3);
  }

  map_prg_rom_page_16k(reg->prg0, 2);
  map_prg_rom_page_8k(reg->prg1, 6);

  switch (reg->ctrl & 3) {
    case 0:
      map_chr_page_1k(reg->chr[0], 0);
      map_chr_page_1k(reg->chr[1], 1);
      map_chr_page_1k(reg->chr[2], 2);
      map_chr_page_1k(reg->chr[3], 3);
      map_chr_page_1k(reg->chr[4], 4);
      map_chr_page_1k(reg->chr[5], 5);
      map_chr_page_1k(reg->chr[6], 6);
      map_chr_page_1k(reg->chr[7], 7);
      break;
    case 1:
      if (reg->ctrl & 0x20) {
        map_chr_page_2k(reg->chr[0] >> 1, 0);
        map_chr_page_2k(reg->chr[1] >> 1, 1);
        map_chr_page_2k(reg->chr[2] >> 1, 2);
        map_chr_page_2k(reg->chr[3] >> 1, 3);
      } else {
        map_chr_page_1k(reg->chr[0], 0);
        map_chr_page_1k(reg->chr[0], 1);
        map_chr_page_1k(reg->chr[1], 2);
        map_chr_page_1k(reg->chr[1], 3);
        map_chr_page_1k(reg->chr[2], 4);
        map_chr_page_1k(reg->chr[2], 5);
        map_chr_page_1k(reg->chr[3], 6);
        map_chr_page_1k(reg->chr[3], 7);
      }
      break;
    case 2:
    case 3:
      map_chr_page_1k(reg->chr[0], 0);
      map_chr_page_1k(reg->chr[1], 1);
      map_chr_page_1k(reg->chr[2], 2);
      map_chr_page_1k(reg->chr[3], 3);
      if (reg->ctrl & 0x20) {
        map_chr_page_2k(reg->chr[4] >> 1, 2);
        map_chr_page_2k(reg->chr[5] >> 1, 3);
      } else {
        map_chr_page_1k(reg->chr[4], 4);
        map_chr_page_1k(reg->chr[4], 5);
        map_chr_page_1k(reg->chr[5], 6);
        map_chr_page_1k(reg->chr[5], 7);
      }
      break;
  }

  uint nt4 = reg->chr[4];
  uint nt5 = reg->chr[5];
  uint nt6 = reg->chr[6];
  uint nt7 = reg->chr[7];
  uint nt60 = (nt6 & 0xFE) | 0;
  uint nt61 = (nt6 & 0xFE) | 1;
  uint nt70 = (nt7 & 0xFE) | 0;
  uint nt71 = (nt7 & 0xFE) | 1;
  bool use_chr = (reg->ctrl & 0x10);
  if (!use_chr) {
    nt4 &= 1;
    nt5 &= 1;
    nt6 &= 1;
    nt7 &= 1;
    nt60 &= 1;
    nt61 &= 1;
    nt70 &= 1;
    nt71 &= 1;
  }

  switch (reg->ctrl & 0x2F) {
    case 0x20:
    case 0x27:
      map_nt_page(nt60, 0x8, use_chr);
      map_nt_page(nt61, 0x9, use_chr);
      map_nt_page(nt70, 0xA, use_chr);
      map_nt_page(nt71, 0xB, use_chr);
      break;
    case 0x23:
    case 0x24:
      map_nt_page(nt60, 0x8, use_chr);
      map_nt_page(nt70, 0x9, use_chr);
      map_nt_page(nt61, 0xA, use_chr);
      map_nt_page(nt71, 0xB, use_chr);
      break;
    case 0x28:
    case 0x2F:
      map_nt_page(nt60, 0x8, use_chr);
      map_nt_page(nt60, 0x9, use_chr);
      map_nt_page(nt70, 0xA, use_chr);
      map_nt_page(nt70, 0xB, use_chr);
      break;
    case 0x2B:
    case 0x2C:
      map_nt_page(nt61, 0x8, use_chr);
      map_nt_page(nt71, 0x9, use_chr);
      map_nt_page(nt61, 0xA, use_chr);
      map_nt_page(nt71, 0xB, use_chr);
      break;
    default:
      switch (reg->ctrl & 7) {
        case 0:
        case 6:
        case 7:
          map_nt_page(nt6, 0x8, use_chr);
          map_nt_page(nt6, 0x9, use_chr);
          map_nt_page(nt7, 0xA, use_chr);
          map_nt_page(nt7, 0xB, use_chr);
          break;
        case 1:
        case 5:
          map_nt_page(nt4, 0x8, use_chr);
          map_nt_page(nt5, 0x9, use_chr);
          map_nt_page(nt6, 0xA, use_chr);
          map_nt_page(nt7, 0xB, use_chr);
          break;
        case 2:
        case 3:
        case 4:
          map_nt_page(nt6, 0x8, use_chr);
          map_nt_page(nt7, 0x9, use_chr);
          map_nt_page(nt6, 0xA, use_chr);
          map_nt_page(nt7, 0xB, use_chr);
          break;
      }
  }
}

static bool map_vrc6_cpu_write(u16 addr, u8 val)
{
  if (addr < 0x8000) {
    return false;
  }
  map_vrc6* reg = (map_vrc6*)map_reg();
  uint a0 = (addr & reg->a0_mask) ? 1 : 0;
  uint a1 = (addr & reg->a1_mask) ? 2 : 0;
  addr = (addr & 0xF000) | a1 | a0;
  switch (addr) {
    case 0x8000:
    case 0x8001:
    case 0x8002:
    case 0x8003: reg->prg0 = (val & 0x0F); break;
    case 0x9000:
    case 0x9001:
    case 0x9002:
    case 0x9003:
    case 0xA000:
    case 0xA001:
    case 0xA002:
    case 0xA003:
    case 0xB000:
    case 0xB001:
    case 0xB002:
      // todo: audio
      break;
    case 0xB003: reg->ctrl = val; break;
    case 0xC000:
    case 0xC001:
    case 0xC002:
    case 0xC003: reg->prg1 = (val & 0x1F); break;
    case 0xD000:
    case 0xD001:
    case 0xD002:
    case 0xD003:
    case 0xE000:
    case 0xE001:
    case 0xE002:
    case 0xE003: reg->chr[((addr & 0x2000) >> 11) | (addr & 3)] = val; break;
    case 0xF000: reg->irq.latch = val; break;
    case 0xF001: map_vrc_irq_ctrl(val); break;
    case 0xF002: map_vrc_irq_ack(); break;
  }
  map_vrc6_update();
  return true;
}

static void map_vrc6_load()
{
  map_set_cpu_cyc_cb(map_vrc_cpu_cyc);
  map_set_cpu_write_cb(map_vrc6_cpu_write);
}

static void map_vrc6a_load()
{
  map_vrc6* reg = (map_vrc6*)map_reg();
  reg->a0_mask = 1;
  reg->a1_mask = 2;
  map_vrc6_load();
}

static void map_vrc6b_load()
{
  map_vrc6* reg = (map_vrc6*)map_reg();
  reg->a0_mask = 2;
  reg->a1_mask = 1;
  map_vrc6_load();
}

static bool map_vrc3_cpu_write(u16 addr, u8 val)
{
  if (addr < 0x8000) {
    return false;
  }
  map_vrc_irq* reg = (map_vrc_irq*)map_reg();
  switch (addr & 0xF000) {
    case 0xF000: map_prg_rom_page_16k(val & 7, 2); break;
    case 0x8000: reg->latch = (reg->latch & 0xFFF0) | (val & 0x0F); break;
    case 0x9000: reg->latch = (reg->latch & 0xFF0F) | ((val & 0x0F) << 4); break;
    case 0xA000: reg->latch = (reg->latch & 0xF0FF) | ((val & 0x0F) << 8); break;
    case 0xB000: reg->latch = (reg->latch & 0x0FFF) | ((val & 0x0F) << 12); break;
    case 0xC000: map_vrc_irq_ctrl(val); break;
    case 0xD000: map_vrc_irq_ack(); break;
  }
  return true;
}

static void map_vrc3_load()
{
  map_vrc_irq* reg = (map_vrc_irq*)map_reg();
  reg->vrc3 = true;
  map_set_cpu_cyc_cb(map_vrc_cpu_cyc);
  map_set_cpu_write_cb(map_vrc3_cpu_write);
}

typedef struct
{
  u8 chr0;
  u8 chr1;
} map_vrc1;
static_assert(sizeof(map_vrc1) <= MAP_REG_SIZE);

static bool map_vrc1_cpu_write(u16 addr, u8 val)
{
  if (addr < 0x8000) {
    return false;
  }
  map_vrc1* reg = (map_vrc1*)map_reg();
  mn_rom rom = mn_rom_get();
  switch (addr & 0xF000) {
    case 0x8000: map_prg_rom_page_8k(val & 0x0F, 4); break;
    case 0xA000: map_prg_rom_page_8k(val & 0x0F, 5); break;
    case 0xC000: map_prg_rom_page_8k(val & 0x0F, 6); break;
    case 0x9000:
      if (!rom->alt_mirror) {
        if (val & 1) {
          map_ciram_horiz_mirror();
        } else {
          map_ciram_vert_mirror();
        }
      }
      reg->chr0 = (reg->chr0 & 0x0F) | ((val & 0x02) << 3);
      reg->chr1 = (reg->chr1 & 0x0F) | ((val & 0x04) << 2);
      map_chr_page_4k(reg->chr0, 0);
      map_chr_page_4k(reg->chr1, 1);
      break;
    case 0xE000:
      reg->chr0 = (reg->chr0 & 0x10) | (val & 0x0F);
      map_chr_page_4k(reg->chr0, 0);
      break;
    case 0xF000:
      reg->chr1 = (reg->chr1 & 0x10) | (val & 0x0F);
      map_chr_page_4k(reg->chr1, 1);
      break;
  }
  return true;
}

static void map_vrc1_load() { map_set_cpu_write_cb(map_vrc1_cpu_write); }


typedef struct
{
  map_vrc_irq irq;
  u8 a43_mask;
} map_vrc7;
static_assert(sizeof(map_vrc7) <= MAP_REG_SIZE);

static bool map_vrc7_cpu_write(u16 addr, u8 val)
{
  if (addr < 0x8000) {
    return false;
  }
  map_vrc7* reg = (map_vrc7*)map_reg();
  switch (addr & 0xF018) {
    case 0x8000: map_prg_rom_page_8k(val & 0x3F, 4); break;
    case 0x8008:
    case 0x8010:
      if (addr & reg->a43_mask) {
        map_prg_rom_page_8k(val & 0x3F, 5);
      }
      break;
    case 0x9000: map_prg_rom_page_8k(val & 0x3F, 6); break;
    case 0xA000:
    case 0xA008:
    case 0xA010:
    case 0xB000:
    case 0xB008:
    case 0xB010:
    case 0xC000:
    case 0xC008:
    case 0xC010:
    case 0xD000:
    case 0xD008:
    case 0xD010: map_chr_page_1k(val, (((addr - 0xA000) & 0x3000) >> 11) | ((addr & reg->a43_mask) ? 1 : 0)); break;
    case 0xE000:
      switch (val & 3) {
        case 0: map_ciram_vert_mirror(); break;
        case 1: map_ciram_horiz_mirror(); break;
        case 2: map_ciram_single_low(); break;
        case 3: map_ciram_single_high(); break;
      }
      if (val & 0x80) {
        map_prg_ram_page_8k(0, 3, false);
      } else {
        map_prg_clear_page_8k(3);
      }
      if (val & 0x40) {
        // todo: silence expansion sound if set
      }
      break;
    case 0xE008:
    case 0xE010:
      if (addr & reg->a43_mask) {
        reg->irq.latch = val;
      }
      break;
    case 0xF000: map_vrc_irq_ctrl(val); break;
    case 0xF008:
    case 0xF010:
      if (addr & reg->a43_mask) {
        map_vrc_irq_ack();
      }
      break;
    case 0x9010:
    case 0x9030:
      // todo: audio (VRC7a)
      break;
  }
  return true;
}

static void map_vrc7_load()
{
  map_vrc7* reg = (map_vrc7*)map_reg();
  mn_rom rom = mn_rom_get();
  switch (rom->submapper) {
    case 1: reg->a43_mask = 0x0008; break; // VRC7b
    case 2: reg->a43_mask = 0x0010; break; // VRC7a
    default: reg->a43_mask = 0x0018; break; // both
  }
  map_set_cpu_cyc_cb(map_vrc_cpu_cyc);
  map_set_cpu_write_cb(map_vrc7_cpu_write);
}

typedef struct
{
  u16 irq_counter;
  u8 ctrl;
  u8 addr;
  u8 namco_175;
  u8 namco_340;
  u8 chr[12];
  u8 data[0x80]; // todo: save this to ".sav" or map this to prg-ram?
} map_namco_163;
static_assert(sizeof(map_namco_163) <= MAP_REG_SIZE);

static void map_namco_163_chr_update()
{
  map_namco_163* reg = (map_namco_163*)map_reg();
  if (reg->namco_175 || reg->namco_340) {
    for (uint i = 0; i < 8; ++i) {
      map_chr_page_1k(reg->chr[i], i);
    }
  } else {
    for (uint i = 0; i < 12; ++i) {
      bool use_ciram = false;
      if (reg->chr[i] >= 0xE0) {
        if (i < 4) {
          use_ciram = !(reg->ctrl & 0x40);
        } else if (i < 8) {
          use_ciram = !(reg->ctrl & 0x80);
        } else {
          use_ciram = true;
        }
      }
      map_nt_page(use_ciram ? (reg->chr[i] & 1) : reg->chr[i], i, !use_ciram);
    }
  }
}

static bool map_namco_163_cpu_read(u16 addr, u8* val, bool trace)
{
  map_namco_163* reg = (map_namco_163*)map_reg();
  switch (addr & 0xF800) {
    case 0x4800:
      *val = reg->data[reg->addr & 0x7F];
      if ((reg->addr & 0x80) && reg->addr < 0xFF && !trace) {
        ++reg->addr;
      }
      return true;
    case 0x5000: *val = (reg->irq_counter & 0x00FF); return true;
    case 0x5800: *val = ((reg->irq_counter & 0xFF00) >> 8); return true;
  }
  return false;
}

static bool map_namco_163_cpu_write(u16 addr, u8 val)
{
  if (addr < 0x4800 || (addr >= 0x6000 && addr <= 0x7FFF)) {
    return false;
  }
  map_namco_163* reg = (map_namco_163*)map_reg();
  switch (addr & 0xF800) {
    case 0x4800:
      if (!reg->namco_175 && !reg->namco_340) {
        reg->data[reg->addr & 0x7F] = val;
        if ((reg->addr & 0x80) && reg->addr < 0xFF) {
          ++reg->addr;
        }
      }
      break;
    case 0x5000:
      if (!reg->namco_175 && !reg->namco_340) {
        reg->irq_counter = (reg->irq_counter & 0xFF00) | val;
        map_irq(false);
      }
      break;
    case 0x5800:
      if (!reg->namco_175 && !reg->namco_340) {
        reg->irq_counter = (reg->irq_counter & 0x00FF) | (val << 8);
        map_irq(false);
      }
      break;
    case 0x8000:
    case 0x8800:
    case 0x9000:
    case 0x9800:
    case 0xA000:
    case 0xA800:
    case 0xB000:
    case 0xB800:
    case 0xC000:
      if (reg->namco_175) {
        // ram enable?
      }
    case 0xC800:
    case 0xD000:
    case 0xD800:
      reg->chr[(addr & 0x7800) >> 11] = val;
      map_namco_163_chr_update();
      break;
    case 0xE000:
      map_prg_rom_page_8k(val & 0x3F, 4);
      // todo: audio: val & 0x40, pin 22: val & 0x80
      if (reg->namco_340) {
        switch (val >> 6) {
          case 0: map_ciram_single_low(); break;
          case 1: map_ciram_vert_mirror(); break;
          case 2: map_ciram_single_high(); break;
          case 3: map_ciram_horiz_mirror(); break;
        }
      }
      break;
    case 0xE800:
      map_prg_rom_page_8k(val & 0x3F, 5);
      reg->ctrl = val & 0xC0;
      map_namco_163_chr_update();
      break;
    case 0xF000:
      map_prg_rom_page_8k(val & 0x3F, 6);
      // $3F replaces CHR bank output with audio state debug info
      // todo: audio? pin 44: val & 0x40, val & 0x80
      break;
    case 0xF800:
      if (!reg->namco_175 && !reg->namco_340) {
        reg->addr = val;
        // write protect not needed
      }
      break;
  }
  return true;
}

static void map_namco_163_cpu_cyc()
{
  map_namco_163* reg = (map_namco_163*)map_reg();
  if (reg->irq_counter & 0x8000) {
    if (reg->irq_counter == 0xFFFF) {
      reg->irq_counter = 0x7FFF;
      map_irq(true);
    } else {
      ++reg->irq_counter;
    }
  }
}

static void map_namco_163_load()
{
  map_set_cpu_cyc_cb(map_namco_163_cpu_cyc);
  map_set_cpu_read_cb(map_namco_163_cpu_read);
  map_set_cpu_write_cb(map_namco_163_cpu_write);
}

static void map_namco_175_340_load()
{
  map_namco_163* reg = (map_namco_163*)map_reg();
  mn_rom rom = mn_rom_get();
  switch (rom->submapper) {
    case 0:
      reg->namco_175 = (rom->prg_has_battery || rom->prg_ram_size);
      reg->namco_340 = !reg->namco_175;
      break;
    case 1: reg->namco_175 = true; break;
    case 2: reg->namco_340 = true; break;
  }
  map_set_cpu_write_cb(map_namco_163_cpu_write);
}

typedef struct
{
  u8 ctrl;
  u8 no_prg;
  u8 is_3446;
  u8 is_3433;
  u8 is_3453;
  u8 is_3425;
} map_namco_108;
static_assert(sizeof(map_namco_108) <= MAP_REG_SIZE);

static bool map_namco_108_cpu_write(u16 addr, u8 val)
{
  if (addr < 0x8000) {
    return false;
  }
  map_namco_108* reg = (map_namco_108*)map_reg();
  switch (addr & 0xE001) {
    case 0x8000: map_reg()[0] = val; break;
    case 0x8001:
      if (reg->is_3446) {
        switch (reg->ctrl & 7) {
          case 2: map_chr_page_2k(val & 0x3F, 0); break;
          case 3: map_chr_page_2k(val & 0x3F, 1); break;
          case 4: map_chr_page_2k(val & 0x3F, 2); break;
          case 5: map_chr_page_2k(val & 0x3F, 3); break;
          case 6: map_prg_rom_page_8k(val & 0x3F, 4); break;
          case 7: map_prg_rom_page_8k(val & 0x3F, 5); break;
        }
      } else {
        uint chr_sup = (reg->is_3433 || reg->is_3453) ? 0x40 : 0;
        switch (reg->ctrl & 7) {
          case 0:
            map_chr_page_2k((val & 0x3F) >> 1, 0);
            if (reg->is_3425) {
              map_ciram_page(val & 0x20 ? 1 : 0, 0x8);
              map_ciram_page(val & 0x20 ? 1 : 0, 0x9);
            }
            break;
          case 1:
            map_chr_page_2k((val & 0x3F) >> 1, 1);
            if (reg->is_3425) {
              map_ciram_page(val & 0x20 ? 1 : 0, 0xA);
              map_ciram_page(val & 0x20 ? 1 : 0, 0xB);
            }
            break;
          case 2: map_chr_page_1k(chr_sup | (val & 0x3F), 4); break;
          case 3: map_chr_page_1k(chr_sup | (val & 0x3F), 5); break;
          case 4: map_chr_page_1k(chr_sup | (val & 0x3F), 6); break;
          case 5: map_chr_page_1k(chr_sup | (val & 0x3F), 7); break;
          case 6:
            if (!reg->no_prg) {
              map_prg_rom_page_8k(val & 0x0F, 4);
            }
            break;
          case 7:
            if (!reg->no_prg) {
              map_prg_rom_page_8k(val & 0x0F, 5);
            }
            break;
        }
      }
      break;
  }
  if (reg->is_3453) {
    if (val & 0x40) {
      map_ciram_single_high();
    } else {
      map_ciram_single_low();
    }
  }
  return true;
}

static void map_namco_108_load()
{
  map_namco_108* reg = (map_namco_108*)map_reg();
  mn_rom rom = mn_rom_get();
  reg->no_prg = (rom->submapper == 1);
  map_set_cpu_write_cb(map_namco_108_cpu_write);
}

static void map_namco_3446_load()
{
  map_namco_108* reg = (map_namco_108*)map_reg();
  reg->is_3446 = true;
  map_set_cpu_write_cb(map_namco_108_cpu_write);
}

static void map_namco_3433_load()
{
  map_namco_108* reg = (map_namco_108*)map_reg();
  reg->is_3433 = true;
  map_set_cpu_write_cb(map_namco_108_cpu_write);
}

static void map_namco_3453_load()
{
  map_namco_108* reg = (map_namco_108*)map_reg();
  reg->is_3453 = true;
  map_set_cpu_write_cb(map_namco_108_cpu_write);
}

static void map_namco_3425_load()
{
  map_namco_108* reg = (map_namco_108*)map_reg();
  reg->is_3425 = true;
  map_set_cpu_write_cb(map_namco_108_cpu_write);
}

typedef struct
{
  u16 irq_latch;
  u16 irq_counter;
  u8 irq_enabled;
  u8 lz93d50;
  u8 prg_page;
  u8 has_outer_prg;
  u8 has_wram;
  u8 has_barcode;
  u8 has_eeprom128;
} map_bandai_fcg;
static_assert(sizeof(map_bandai_fcg) <= MAP_REG_SIZE);

static bool map_bandai_fcg_cpu_read(u16 addr, u8* val, bool)
{
  map_bandai_fcg* reg = (map_bandai_fcg*)map_reg();
  if (reg->lz93d50 && !reg->has_wram && addr >= 0x6000 && addr <= 0x7FFF) {
    u8 data = 0; // todo: eeprom (use prg-ram memory?)
    u8 barcode = 0;
    if (reg->has_barcode) {
      // todo: second barcode eeprom
    }
    *val = (*val & 0xEF) | (data & 0x10) | (barcode & 0x08);
    return true;
  }
  return false;
}

static bool map_bandai_fcg_cpu_write(u16 addr, u8 val)
{
  map_bandai_fcg* reg = (map_bandai_fcg*)map_reg();
  if (reg->lz93d50 ? (addr & 0x8000) : ((addr & 0xE000) == 0x6000)) {
    switch (addr & 0x000F) {
      case 0x00:
      case 0x01:
      case 0x02:
      case 0x03:
        if (reg->has_outer_prg) {
          uint outer_page = ((val & 1) << 4);
          reg->prg_page = outer_page | (reg->prg_page & 0x0F);
          map_prg_rom_page_16k(reg->prg_page, 2);
          map_prg_rom_page_16k(outer_page | 0x0F, 3);
          return true;
        }
        if (reg->has_barcode) {
          // todo: barcode
          return true;
        }
      case 0x04:
      case 0x05:
      case 0x06:
      case 0x07:
        if (!reg->has_outer_prg && !reg->has_barcode) {
          map_chr_page_1k(val, addr & 7);
        }
        return true;
      case 0x08:
        reg->prg_page = (reg->prg_page & 0xF0) | (val & 0x0F);
        map_prg_rom_page_16k(reg->prg_page, 2);
        return true;
      case 0x09:
        switch (val & 3) {
          case 0: map_ciram_vert_mirror(); break;
          case 1: map_ciram_horiz_mirror(); break;
          case 2: map_ciram_single_low(); break;
          case 3: map_ciram_single_high(); break;
        }
        return true;
      case 0x0A: {
        if (reg->lz93d50) {
          reg->irq_counter = reg->irq_latch;
        }
        reg->irq_enabled = (val & 0x01);
        map_irq(false);
        return true;
      }
      case 0x0B:
        reg->irq_latch = (reg->irq_latch & 0xFF00) | val;
        if (!reg->lz93d50) {
          reg->irq_counter = reg->irq_latch;
        }
        return true;
      case 0x0C:
        reg->irq_latch = (reg->irq_latch & 0x00FF) | (val << 8);
        if (!reg->lz93d50) {
          reg->irq_counter = reg->irq_latch;
        }
        return true;
      case 0x0D:
        if (reg->lz93d50) {
          if (reg->has_wram) {
            if (val & 0x20) {
              map_prg_ram_page_8k(0, 3, false);
            } else {
              map_prg_clear_page_8k(3);
            }
          }
          // else if (reg->has_barcode) {
          //  // todo: eeprom, barcode
          // } else {
          //  // todo: eeprom
          // }
        }
        return true;
    }
  }
  return false;
}

static void map_bandai_fcg_cpu_cyc()
{
  map_bandai_fcg* reg = (map_bandai_fcg*)map_reg();
  if (reg->irq_enabled) {
    if (reg->irq_counter == 0) {
      map_irq(true);
    } else {
      --reg->irq_counter;
    }
  }
}

static void map_bandai_fcg_load_cb()
{
  map_set_cpu_read_cb(map_bandai_fcg_cpu_read);
  map_set_cpu_write_cb(map_bandai_fcg_cpu_write);
  map_set_cpu_cyc_cb(map_bandai_fcg_cpu_cyc);
}

static void map_bandai_fcg_load()
{
  map_bandai_fcg* reg = (map_bandai_fcg*)map_reg();
  mn_rom rom = mn_rom_get();
  reg->lz93d50 = (rom->submapper == 5);
  map_bandai_fcg_load_cb();
}

static void map_bandai_fcg_wram_load()
{
  map_bandai_fcg* reg = (map_bandai_fcg*)map_reg();
  reg->lz93d50 = true;
  reg->has_outer_prg = true;
  reg->has_wram = true;
  map_bandai_fcg_load_cb();
}

static void map_bandai_fcg_barcode_load()
{
  map_bandai_fcg* reg = (map_bandai_fcg*)map_reg();
  reg->lz93d50 = true;
  reg->has_barcode = true;
  map_bandai_fcg_load_cb();
}

static void map_bandai_fcg_eeprom128_load()
{
  map_bandai_fcg* reg = (map_bandai_fcg*)map_reg();
  reg->lz93d50 = true;
  reg->has_eeprom128 = true;
  map_bandai_fcg_load_cb();
}

typedef struct
{
  u16 irq_addr;
  u8 irq_filter;
  u8 irq_counter;
  u8 irq_latch;
  u8 irq_reload;
  u8 irq_mode;
  u8 irq_prescaler;
  u8 irq_enabled;
  u8 irq_delay;
  u8 chr0;
  u8 chr1;
  u8 chr2;
  u8 chr3;
  u8 chr4;
  u8 chr5;
  u8 prg0;
  u8 prg1;
  u8 chr6;
  u8 chr7;
  u8 prg2;
  u8 ctrl;
  u8 alt_mirror;
} map_tengen_rambo1;
static_assert(sizeof(map_tengen_rambo1) <= MAP_REG_SIZE);

static void map_tengen_rambo1_update()
{
  map_tengen_rambo1* reg = (map_tengen_rambo1*)map_reg();
  uint chr0 = reg->chr0;
  uint chr1 = reg->chr1;
  uint chr2 = reg->chr2;
  uint chr3 = reg->chr3;
  uint chr4 = reg->chr4;
  uint chr5 = reg->chr5;
  uint chr6 = reg->chr6;
  uint chr7 = reg->chr7;
  if (reg->alt_mirror) {
    chr0 &= 0x7F;
    chr1 &= 0x7F;
    chr2 &= 0x7F;
    chr3 &= 0x7F;
    chr4 &= 0x7F;
    chr5 &= 0x7F;
    chr6 &= 0x7F;
    chr7 &= 0x7F;
    if (reg->ctrl & 0x80) {
      map_ciram_page(reg->chr4 >> 7, 0x8);
      map_ciram_page(reg->chr5 >> 7, 0x9);
      map_ciram_page(reg->chr6 >> 7, 0xA);
      map_ciram_page(reg->chr7 >> 7, 0xB);
    } else {
      map_ciram_page(reg->chr0 >> 7, 0x8);
      map_ciram_page(reg->chr0 >> 7, 0x9);
      map_ciram_page(reg->chr2 >> 7, 0xA);
      map_ciram_page(reg->chr2 >> 7, 0xB);
    }
  }
  if (reg->ctrl & 0x80) {
    if (reg->ctrl & 0x20) {
      map_chr_page_1k(chr0, 4);
      map_chr_page_1k(chr1, 5);
      map_chr_page_1k(chr2, 6);
      map_chr_page_1k(chr3, 7);
    } else {
      map_chr_page_2k(chr0 >> 1, 2);
      map_chr_page_2k(chr2 >> 1, 3);
    }
    map_chr_page_1k(chr4, 0);
    map_chr_page_1k(chr5, 1);
    map_chr_page_1k(chr6, 2);
    map_chr_page_1k(chr7, 3);
  } else {
    if (reg->ctrl & 0x20) {
      map_chr_page_1k(chr0, 0);
      map_chr_page_1k(chr1, 1);
      map_chr_page_1k(chr2, 2);
      map_chr_page_1k(chr3, 3);
    } else {
      map_chr_page_2k(chr0 >> 1, 0);
      map_chr_page_2k(chr2 >> 1, 1);
    }
    map_chr_page_1k(chr4, 4);
    map_chr_page_1k(chr5, 5);
    map_chr_page_1k(chr6, 6);
    map_chr_page_1k(chr7, 7);
  }
  if (reg->ctrl & 0x40) {
    map_prg_rom_page_8k(reg->prg2, 4);
    map_prg_rom_page_8k(reg->prg1, 5);
    map_prg_rom_page_8k(reg->prg0, 6);
    map_prg_rom_page_8k(-1, 7);
  } else {
    map_prg_rom_page_8k(reg->prg0, 4);
    map_prg_rom_page_8k(reg->prg1, 5);
    map_prg_rom_page_8k(reg->prg2, 6);
    map_prg_rom_page_8k(-1, 7);
  }
}

static bool map_tengen_rambo1_cpu_write(u16 addr, u8 val)
{
  if (addr < 0x8000) {
    return false;
  }
  map_tengen_rambo1* reg = (map_tengen_rambo1*)map_reg();
  switch (addr & 0xE001) {
    case 0x8000:
      reg->ctrl = val;
      map_tengen_rambo1_update();
      break;
    case 0x8001:
      switch (reg->ctrl & 0x0F) {
        case 0x0: reg->chr0 = val; break;
        case 0x1: reg->chr2 = val; break;
        case 0x2: reg->chr4 = val; break;
        case 0x3: reg->chr5 = val; break;
        case 0x4: reg->chr6 = val; break;
        case 0x5: reg->chr7 = val; break;
        case 0x6: reg->prg0 = val; break;
        case 0x7: reg->prg1 = val; break;
        case 0x8: reg->chr1 = val; break;
        case 0x9: reg->chr3 = val; break;
        case 0xF: reg->prg2 = val; break;
      }
      map_tengen_rambo1_update();
      break;
    case 0xA000:
      if (!mn_rom_get()->alt_mirror && !reg->alt_mirror) {
        if (val & 1) {
          map_ciram_horiz_mirror();
        } else {
          map_ciram_vert_mirror();
        }
      }
      break;
    case 0xA001: break;
    case 0xC000: reg->irq_latch = val; break;
    case 0xC001:
      reg->irq_mode = (val & 1);
      reg->irq_prescaler = 0;
      reg->irq_reload = true;
      break;
    case 0xE000:
      reg->irq_enabled = false;
      map_irq(false);
      break;
    case 0xE001: reg->irq_enabled = true; break;
  }
  return true;
}

static void map_tengen_rambo1_irq_clock()
{
  map_tengen_rambo1* reg = (map_tengen_rambo1*)map_reg();
  if (reg->irq_counter == 0 || reg->irq_reload) {
    reg->irq_counter = reg->irq_latch;
    if (reg->irq_reload && reg->irq_latch != 0 && reg->irq_latch != 0xFF) {
      ++reg->irq_counter; // hw bug?
    }
  } else {
    --reg->irq_counter;
  }
  if (reg->irq_counter == 0 && reg->irq_enabled) {
    reg->irq_delay = 2;
  }
  reg->irq_reload = false;
}

static void map_tengen_rambo1_ppu_addr(u16 addr)
{
  map_tengen_rambo1* reg = (map_tengen_rambo1*)map_reg();
  reg->irq_addr = addr;
}

static void map_tengen_rambo1_cpu_cyc()
{
  map_tengen_rambo1* reg = (map_tengen_rambo1*)map_reg();
  if (reg->irq_delay > 0) {
    if (--reg->irq_delay == 0) {
      map_irq(true);
    }
  }
  if (reg->irq_mode == 0) {
    if (reg->irq_addr & 0x1000) {
      if (reg->irq_filter == 0) {
        map_tengen_rambo1_irq_clock();
      }
      reg->irq_filter = 5;
    } else if (reg->irq_filter > 0) {
      --reg->irq_filter;
    }
  } else {
    if (++reg->irq_prescaler == 4) {
      reg->irq_prescaler = 0;
      map_tengen_rambo1_irq_clock();
    }
  }
}

static void map_tengen_rambo1_load()
{
  map_set_cpu_write_cb(map_tengen_rambo1_cpu_write);
  map_set_ppu_addr_cb(map_tengen_rambo1_ppu_addr);
  map_set_cpu_cyc_cb(map_tengen_rambo1_cpu_cyc);
  map_tengen_rambo1_update();
}

static void map_tengen_rambo1_800037_load()
{
  map_tengen_rambo1* reg = (map_tengen_rambo1*)map_reg();
  reg->alt_mirror = true;
  map_tengen_rambo1_load();
}


static bool map_taito_tc0190_cpu_write(u16 addr, u8 val)
{
  if (addr < 0x8000) {
    return false;
  }
  switch (addr & 0xA003) {
    case 0x8000:
      if (val & 0x40) {
        map_ciram_horiz_mirror();
      } else {
        map_ciram_vert_mirror();
      }
      map_prg_rom_page_8k(val & 0x3F, 4);
      break;
    case 0x8001: map_prg_rom_page_8k(val & 0x3F, 5); break;
    case 0x8002: map_chr_page_2k(val, 0); break;
    case 0x8003: map_chr_page_2k(val, 1); break;
    case 0xA000: map_chr_page_1k(val, 4); break;
    case 0xA001: map_chr_page_1k(val, 5); break;
    case 0xA002: map_chr_page_1k(val, 6); break;
    case 0xA003: map_chr_page_1k(val, 7); break;
  }
  return true;
}

typedef struct
{
  u16 irq_addr;
  u8 irq_filter;
  u8 irq_counter;
  u8 irq_latch;
  u8 irq_reload;
  u8 irq_enabled;
  u8 irq_delay;
} map_taito_tc0690;
static_assert(sizeof(map_taito_tc0690) <= MAP_REG_SIZE);

static bool map_taito_tc0690_cpu_write(u16 addr, u8 val)
{
  if (addr < 0x8000) {
    return false;
  }
  map_taito_tc0690* reg = (map_taito_tc0690*)map_reg();
  switch (addr & 0xE003) {
    case 0x8000: map_prg_rom_page_8k(val, 4); break;
    case 0x8001: map_prg_rom_page_8k(val, 5); break;
    case 0x8002: map_chr_page_2k(val, 0); break;
    case 0x8003: map_chr_page_2k(val, 1); break;
    case 0xA000: map_chr_page_1k(val, 4); break;
    case 0xA001: map_chr_page_1k(val, 5); break;
    case 0xA002: map_chr_page_1k(val, 6); break;
    case 0xA003: map_chr_page_1k(val, 7); break;
    case 0xC000: reg->irq_latch = (val ^ 0xFF); break;
    case 0xC001: reg->irq_reload = true; break;
    case 0xC002: reg->irq_enabled = true; break;
    case 0xC003:
      reg->irq_enabled = false;
      map_irq(false);
      break;
    case 0xE000:
      if (val & 0x40) {
        map_ciram_horiz_mirror();
      } else {
        map_ciram_vert_mirror();
      }
      break;
  }
  return false;
}

static void map_taito_tc0690_ppu_addr(u16 addr)
{
  map_tengen_rambo1* reg = (map_tengen_rambo1*)map_reg();
  reg->irq_addr = addr;
}

static void map_taito_tc0690_cpu_cyc()
{
  map_taito_tc0690* reg = (map_taito_tc0690*)map_reg();
  if (reg->irq_delay > 0) {
    if (--reg->irq_delay == 0) {
      map_irq(true);
    }
  }
  if (reg->irq_addr & 0x1000) {
    if (reg->irq_filter == 0) {
      // IRQ similar to RAMBO-1 (or MMC3, but filter on CPU)
      if (reg->irq_counter == 0 || reg->irq_reload) {
        reg->irq_counter = reg->irq_latch;
      } else {
        --reg->irq_counter;
      }
      if (reg->irq_counter == 0 && reg->irq_enabled) {
        // nesdev says 4, but 19-23 works fine (numbers also depends on irq_filter=5)
        reg->irq_delay = mn_rom_get()->submapper == 1 ? 5 : 21;
      }
      reg->irq_reload = false;
    }
    reg->irq_filter = 5;
  } else if (reg->irq_filter > 0) {
    --reg->irq_filter;
  }
}

static void map_taito_tc0690_load()
{
  map_set_cpu_cyc_cb(map_taito_tc0690_cpu_cyc);
  map_set_cpu_write_cb(map_taito_tc0690_cpu_write);
  map_set_ppu_addr_cb(map_taito_tc0690_ppu_addr);
}

typedef struct
{
  u8 ram_enabled;
  u8 alt_mirror;
} map_taito_x1005;
static_assert(sizeof(map_taito_x1005) <= MAP_REG_SIZE);

static bool map_taito_x1005_cpu_read(u16 addr, u8* val, bool)
{
  map_taito_x1005* reg = (map_taito_x1005*)map_reg();
  if (reg->ram_enabled && (addr & 0xFF00) == 0x7F00) {
    mn_rom rom = mn_rom_get();
    *val = rom->prg_ram[addr & 0x7F];
    return true;
  }
  return false;
}

static bool map_taito_x1005_cpu_write(u16 addr, u8 val)
{
  map_taito_x1005* reg = (map_taito_x1005*)map_reg();
  if (reg->ram_enabled && (addr & 0xFF00) == 0x7F00) {
    mn_rom rom = mn_rom_get();
    rom->prg_ram[addr & 0x7F] = val;
    return true;
  }
  switch (addr & 0xFF7F) {
    case 0x7E70:
      if (reg->alt_mirror) {
        uint ciram_page = (val >> 7);
        val &= 0x7F;
        map_ciram_page(ciram_page, 0x8);
        map_ciram_page(ciram_page, 0x9);
      }
      map_chr_page_2k(val >> 1, 0);
      return true;
    case 0x7E71:
      if (reg->alt_mirror) {
        uint ciram_page = (val >> 7);
        val &= 0x7F;
        map_ciram_page(ciram_page, 0xA);
        map_ciram_page(ciram_page, 0xB);
      }
      map_chr_page_2k(val >> 1, 1);
      return true;
    case 0x7E72: map_chr_page_1k(val, 4); return true;
    case 0x7E73: map_chr_page_1k(val, 5); return true;
    case 0x7E74: map_chr_page_1k(val, 6); return true;
    case 0x7E75: map_chr_page_1k(val, 7); return true;
    case 0x7E76:
      if (!reg->alt_mirror) {
        if (val & 1) {
          map_ciram_vert_mirror();
        } else {
          map_ciram_horiz_mirror();
        }
      }
      return true;
    case 0x7E78:
    case 0x7E79: reg->ram_enabled = (val == 0xA3); return true;
    case 0x7E7A:
    case 0x7E7B: map_prg_rom_page_8k(val, 4); return true;
    case 0x7E7C:
    case 0x7E7D: map_prg_rom_page_8k(val, 5); return true;
    case 0x7E7E:
    case 0x7E7F: map_prg_rom_page_8k(val, 6); return true;
  }
  return false;
}

static void map_taito_x1005_load()
{
  map_prg_clear_page_8k(3);
  map_set_cpu_read_cb(map_taito_x1005_cpu_read);
  map_set_cpu_write_cb(map_taito_x1005_cpu_write);
}

static void map_taito_x1005a_load()
{
  map_taito_x1005* reg = (map_taito_x1005*)map_reg();
  reg->alt_mirror = true;
  map_taito_x1005_load();
}

typedef struct
{
  u8 chr[6];
  u8 chr_inverse;
  u8 ram[3];
} map_taito_x1017;
static_assert(sizeof(map_taito_x1017) <= MAP_REG_SIZE);

static void map_taito_x1017_chr_update()
{
  map_taito_x1017* reg = (map_taito_x1017*)map_reg();
  if (reg->chr_inverse) {
    map_chr_page_2k(reg->chr[0] >> 1, 2);
    map_chr_page_2k(reg->chr[1] >> 1, 3);
    map_chr_page_1k(reg->chr[2], 0);
    map_chr_page_1k(reg->chr[3], 1);
    map_chr_page_1k(reg->chr[4], 2);
    map_chr_page_1k(reg->chr[5], 3);
  } else {
    map_chr_page_2k(reg->chr[0] >> 1, 0);
    map_chr_page_2k(reg->chr[1] >> 1, 1);
    map_chr_page_1k(reg->chr[2], 4);
    map_chr_page_1k(reg->chr[3], 5);
    map_chr_page_1k(reg->chr[4], 6);
    map_chr_page_1k(reg->chr[5], 7);
  }
}

static uint map_taito_x1017_reverse_bits(u8 val)
{
  uint b0 = (val & 0x01) << 7;
  uint b1 = (val & 0x02) << 5;
  uint b2 = (val & 0x04) << 3;
  uint b3 = (val & 0x08) << 1;
  uint b4 = (val & 0x10) >> 1;
  uint b5 = (val & 0x20) >> 3;
  uint b6 = (val & 0x40) >> 5;
  uint b7 = (val & 0x80) >> 7;
  return b0 | b1 | b2 | b3 | b4 | b5 | b6 | b7;
}

static bool map_taito_x1017_cpu_read(u16 addr, u8* val, bool)
{
  map_taito_x1017* reg = (map_taito_x1017*)map_reg();
  if ((addr >= 0x6000 && addr <= 0x67FF && !reg->ram[0]) || (addr >= 0x6800 && addr <= 0x6FFF && !reg->ram[1])
      || (addr >= 0x7000 && addr <= 0x73FF && !reg->ram[2]) || (addr >= 0x7400 && addr <= 0x7FFF))
  {
    *val = 0; // must return 0 instead of openbus when ram disabled
    return true;
  }
  return false;
}

static bool map_taito_x1017_cpu_write(u16 addr, u8 val)
{
  map_taito_x1017* reg = (map_taito_x1017*)map_reg();
  switch (addr & 0xFF7F) {
    case 0x7E70:
    case 0x7E71:
    case 0x7E72:
    case 0x7E73:
    case 0x7E74:
    case 0x7E75:
      reg->chr[addr & 7] = val;
      map_taito_x1017_chr_update();
      return true;
    case 0x7E76:
      if (val & 1) {
        map_ciram_vert_mirror();
      } else {
        map_ciram_horiz_mirror();
      }
      reg->chr_inverse = (val & 2);
      map_taito_x1017_chr_update();
      return true;
    case 0x7E77:
      reg->ram[0] = (val == 0xCA);
      if (reg->ram[0]) {
        map_prg_ram_page_2k(0, 12, false);
      } else {
        map_prg_clear_page_2k(12);
      }
      return true;
    case 0x7E78:
      reg->ram[1] = (val == 0x69);
      if (reg->ram[1]) {
        map_prg_ram_page_2k(1, 13, false);
      } else {
        map_prg_clear_page_2k(13);
      }
      return true;
    case 0x7E79:
      reg->ram[2] = (val == 0x84);
      if (reg->ram[2]) {
        map_prg_ram_page_1k(4, 28, false);
      } else {
        map_prg_clear_page_1k(28);
      }
      return true;
    case 0x7E7A: map_prg_rom_page_8k(map_taito_x1017_reverse_bits(val) >> 2, 4); return true;
    case 0x7E7B: map_prg_rom_page_8k(map_taito_x1017_reverse_bits(val) >> 2, 5); return true;
    case 0x7E7C: map_prg_rom_page_8k(map_taito_x1017_reverse_bits(val) >> 2, 6); return true;
    case 0x7E7D:
    case 0x7E7E:
    case 0x7E7F:
      // irq not used in real games
      break;
  }
  return false;
}

static void map_taito_x1017_load()
{
  map_prg_clear_page_8k(3);
  map_set_cpu_read_cb(map_taito_x1017_cpu_read);
  map_set_cpu_write_cb(map_taito_x1017_cpu_write);
}

typedef struct
{
  u8 ctrl;
  u8 prg0;
  u8 prg1;
  u8 no_9000;
} map_irem_g101;
static_assert(sizeof(map_irem_g101) <= MAP_REG_SIZE);

static void map_irem_g101_update()
{
  map_irem_g101* reg = (map_irem_g101*)map_reg();
  if (reg->ctrl & 2) {
    map_prg_rom_page_8k(-2, 4);
    map_prg_rom_page_8k(reg->prg1, 5);
    map_prg_rom_page_8k(reg->prg0, 6);
    map_prg_rom_page_8k(-1, 7);
  } else {
    map_prg_rom_page_8k(reg->prg0, 4);
    map_prg_rom_page_8k(reg->prg1, 5);
    map_prg_rom_page_8k(-2, 6);
    map_prg_rom_page_8k(-1, 7);
  }
}

static bool map_irem_g101_cpu_write(u16 addr, u8 val)
{
  if (addr < 0x8000) {
    return false;
  }
  map_irem_g101* reg = (map_irem_g101*)map_reg();
  switch (addr & 0xF000) {
    case 0x8000:
      reg->prg0 = (val & 0x1F);
      map_irem_g101_update();
      break;
    case 0x9000:
      if (!reg->no_9000) {
        if (val & 1) {
          map_ciram_horiz_mirror();
        } else {
          map_ciram_vert_mirror();
        }
        reg->ctrl = val;
        map_irem_g101_update();
      }
      break;
    case 0xA000:
      reg->prg1 = (val & 0x1F);
      map_irem_g101_update();
      break;
    case 0xB000: map_chr_page_1k(val, addr & 7); break;
  }
  return true;
}

static void map_irem_g101_load()
{
  map_irem_g101* reg = (map_irem_g101*)map_reg();
  mn_rom rom = mn_rom_get();
  if (rom->submapper == 1) {
    reg->no_9000 = true;
    map_ciram_single_low();
  }
  map_set_cpu_write_cb(map_irem_g101_cpu_write);
}

typedef struct
{
  u16 irq_latch;
  u16 irq_counter;
  u8 irq_enabled;
  u8 ctrl;
  u8 prg0;
  u8 prg1;
} map_irem_h3001;
static_assert(sizeof(map_irem_h3001) <= MAP_REG_SIZE);

static void map_irem_h3001_update()
{
  map_irem_h3001* reg = (map_irem_h3001*)map_reg();
  if (reg->ctrl & 2) {
    map_prg_rom_page_8k(-2, 4);
    map_prg_rom_page_8k(reg->prg1, 5);
    map_prg_rom_page_8k(reg->prg0, 6);
    map_prg_rom_page_8k(-1, 7);
  } else {
    map_prg_rom_page_8k(reg->prg0, 4);
    map_prg_rom_page_8k(reg->prg1, 5);
    map_prg_rom_page_8k(-2, 6);
    map_prg_rom_page_8k(-1, 7);
  }
}

static bool map_irem_h3001_cpu_write(u16 addr, u8 val)
{
  if (addr < 0x8000) {
    return false;
  }
  map_irem_h3001* reg = (map_irem_h3001*)map_reg();
  switch (addr & 0xF000) {
    case 0x8000:
      reg->prg0 = (val & 0x1F);
      map_irem_h3001_update();
      break;
    case 0x9000:
      switch (addr & 7) {
        case 0:
          reg->ctrl = val;
          map_irem_h3001_update();
          break;
        case 1:
          switch (val >> 6) {
            case 0: map_ciram_vert_mirror(); break;
            case 2: map_ciram_horiz_mirror(); break;
            case 1:
            case 3: map_ciram_single_low(); break;
          }
          break;
        case 3:
          reg->irq_enabled = (val >> 7);
          map_irq(false);
          break;
        case 4:
          reg->irq_counter = reg->irq_latch;
          map_irq(false);
          break;
        case 5: reg->irq_latch = (reg->irq_latch & 0x00FF) | (val << 8); break;
        case 6: reg->irq_latch = (reg->irq_latch & 0xFF00) | val; break;
      }
      break;
    case 0xA000:
      reg->prg1 = (val & 0x1F);
      map_irem_h3001_update();
      break;
    case 0xB000: map_chr_page_1k(val, addr & 7); break;
  }
  return true;
}

static void map_irem_h3001_cpu_cyc()
{
  map_irem_h3001* reg = (map_irem_h3001*)map_reg();
  if (reg->irq_enabled && reg->irq_counter) {
    if (--reg->irq_counter == 0) {
      map_irq(true);
    }
  }
}

static void map_irem_h3001_load()
{
  map_set_cpu_write_cb(map_irem_h3001_cpu_write);
  map_set_cpu_cyc_cb(map_irem_h3001_cpu_cyc);
}

static bool map_homebrew_nsf_subset_cpu_write(u16 addr, u8 val)
{
  if ((addr & 0xF000) == 0x5000) {
    map_prg_rom_page_4k(val, 8 + (addr & 7));
    return true;
  }
  return false;
}

bool map_other_load()
{
  mn_rom rom = mn_rom_get();
  switch (rom->mapper) {
    case 16: map_bandai_fcg_load(); break;
    case 18: map_jaleco_ss_load(); break;
    case 19: map_namco_163_load(); break;
    case 21:
    case 22:
    case 23:
    case 25:
    case 27: map_vrc2_vrc4_load(); break;
    case 24: map_vrc6a_load(); break;
    case 26: map_vrc6b_load(); break;
    case 28: map_set_cpu_write_cb(map_action53_cpu_write); break;
    case 31: map_set_cpu_write_cb(map_homebrew_nsf_subset_cpu_write); break;
    case 32: map_irem_g101_load(); break;
    case 33: map_set_cpu_write_cb(map_taito_tc0190_cpu_write); break;
    case 48: map_taito_tc0690_load(); break;
    case 64: map_tengen_rambo1_load(); break;
    case 65: map_irem_h3001_load(); break;
    case 67: map_sunsoft3_load(); break;
    case 68: map_sunsoft4_load(); break;
    case 69: map_sunsoft_fme7_load(); break;
    case 73: map_vrc3_load(); break;
    case 75: map_vrc1_load(); break;
    case 76: map_namco_3446_load(); break;
    case 80: map_taito_x1005_load(); break;
    case 82: map_taito_x1017_load(); break;
    case 85: map_vrc7_load(); break;
    case 88: map_namco_3433_load(); break;
    case 95: map_namco_3425_load(); break;
    case 153: map_bandai_fcg_wram_load(); break;
    case 154: map_namco_3453_load(); break;
    case 157: map_bandai_fcg_barcode_load(); break;
    case 158: map_tengen_rambo1_800037_load(); break;
    case 159: map_bandai_fcg_eeprom128_load(); break;
    case 206: map_namco_108_load(); break;
    case 207: map_taito_x1005a_load(); break;
    case 210: map_namco_175_340_load(); break;
    case 552: map_taito_x1017_load(); break;
    default: return false;
  }
  return true;
}
