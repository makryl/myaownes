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
        case 0: map_ppu_nt_vert_mirror(); break;
        case 1: map_ppu_nt_horiz_mirror(); break;
        case 2: map_ppu_nt_single_low(); break;
        case 3: map_ppu_nt_single_high(); break;
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

  // simplified W.PN MMDD -> W.10 MM00
  map_chr_page_1k(reg->chr[0], 0);
  map_chr_page_1k(reg->chr[1], 1);
  map_chr_page_1k(reg->chr[2], 2);
  map_chr_page_1k(reg->chr[3], 3);
  map_chr_page_1k(reg->chr[4], 4);
  map_chr_page_1k(reg->chr[5], 5);
  map_chr_page_1k(reg->chr[6], 6);
  map_chr_page_1k(reg->chr[7], 7);

  switch ((reg->ctrl >> 2) & 3) {
    case 0: map_ppu_nt_vert_mirror(); break;
    case 1: map_ppu_nt_horiz_mirror(); break;
    case 2: map_ppu_nt_single_low(); break;
    case 3: map_ppu_nt_single_high(); break;
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
    case 0xC000:
    case 0xC001:
    case 0xC002:
    case 0xC003: reg->prg1 = (val & 0x1F); break;
    case 0xB003: reg->ctrl = val; break;
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
    case 0x9000:
    case 0xA000:
    case 0xB000:
    case 0xB001:
    case 0xB002:
      // todo: audio
      break;
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
          map_ppu_nt_horiz_mirror();
        } else {
          map_ppu_nt_vert_mirror();
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
        case 0: map_ppu_nt_vert_mirror(); break;
        case 1: map_ppu_nt_horiz_mirror(); break;
        case 2: map_ppu_nt_single_low(); break;
        case 3: map_ppu_nt_single_high(); break;
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
  u8 chr[12];
  u8 data[0x80]; // todo: save this to ".sav"?
} map_namco163;
static_assert(sizeof(map_namco163) <= MAP_REG_SIZE);

static void map_namco163_chr_update()
{
  map_namco163* reg = (map_namco163*)map_reg();
  for (uint i = 0; i < 12; ++i) {
    bool use_nt = false;
    if (reg->chr[i] >= 0xE0) {
      if (i < 4) {
        use_nt = !(reg->ctrl & 0x40);
      } else if (i < 8) {
        use_nt = !(reg->ctrl & 0x80);
      } else {
        use_nt = true;
      }
    }
    if (use_nt) {
      map_ppu_nt_page(reg->chr[i] & 1, i);
    } else {
      map_chr_page_1k(reg->chr[i], i);
    }
  }
}

static bool map_namco163_cpu_read(u16 addr, u8* val, bool trace)
{
  map_namco163* reg = (map_namco163*)map_reg();
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

static bool map_namco163_cpu_write(u16 addr, u8 val)
{
  if (addr < 0x4800 || (addr >= 0x6000 && addr <= 0x7FFF)) {
    return false;
  }
  map_namco163* reg = (map_namco163*)map_reg();
  switch (addr & 0xF800) {
    case 0x4800:
      reg->data[reg->addr & 0x7F] = val;
      if ((reg->addr & 0x80) && reg->addr < 0xFF) {
        ++reg->addr;
      }
      break;
    case 0x5000:
      reg->irq_counter = (reg->irq_counter & 0xFF00) | val;
      map_irq(false);
      break;
    case 0x5800:
      reg->irq_counter = (reg->irq_counter & 0x00FF) | (val << 8);
      map_irq(false);
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
    case 0xC800:
    case 0xD000:
    case 0xD800:
      reg->chr[(addr & 0x7800) >> 11] = val;
      map_namco163_chr_update();
      break;
    case 0xE000:
      map_prg_rom_page_8k(val & 0x3F, 4);
      // todo: audio: val & 0x40, pin 22: val & 0x80
      break;
    case 0xE800:
      map_prg_rom_page_8k(val & 0x3F, 5);
      reg->ctrl = val & 0xC0;
      map_namco163_chr_update();
      break;
    case 0xF000:
      map_prg_rom_page_8k(val & 0x3F, 6);
      // $3F replaces CHR bank output with audio state debug info
      // todo: audio? pin 44: val & 0x40, val & 0x80
      break;
    case 0xF800:
      reg->addr = val;
      // write protect not needed
      break;
  }
  return true;
}

static void map_namco163_cpu_cyc()
{
  map_namco163* reg = (map_namco163*)map_reg();
  if (reg->irq_counter & 0x8000) {
    if (reg->irq_counter == 0xFFFF) {
      reg->irq_counter = 0x7FFF;
      map_irq(true);
    } else {
      ++reg->irq_counter;
    }
  }
}

static void map_namco163_load()
{
  map_set_cpu_cyc_cb(map_namco163_cpu_cyc);
  map_set_cpu_read_cb(map_namco163_cpu_read);
  map_set_cpu_write_cb(map_namco163_cpu_write);
}

bool map_other_load()
{
  mn_rom rom = mn_rom_get();
  switch (rom->mapper) {
    case 18: map_jaleco_ss_load(); break;
    case 19: map_namco163_load(); break;
    case 28: map_set_cpu_write_cb(map_action53_cpu_write); break;
    case 67: map_sunsoft3_load(); break;
    // case 68: map_sunsoft4_load(); break;
    case 69: map_sunsoft_fme7_load(); break;
    case 21:
    case 22:
    case 23:
    case 25:
    case 27: map_vrc2_vrc4_load(); break;
    case 24: map_vrc6a_load(); break;
    case 26: map_vrc6b_load(); break;
    case 73: map_vrc3_load(); break;
    case 75: map_vrc1_load(); break;
    case 85: map_vrc7_load(); break;
    default: return false;
  }
  return true;
}
