#pragma once

#include "map_internal.h"

typedef struct
{
  u8 ctrl;
  u8 chr0;
  u8 chr1;
  u8 prg;
  u8 shift;
  u8 delay;
  u8 no_prg;
  u8 mmc1a;
} map_mmc1;
static_assert(sizeof(map_mmc1) <= MAP_REG_SIZE);

static void map_mmc1_update()
{
  mn_rom rom = mn_rom_get();
  map_mmc1* reg = (map_mmc1*)map_reg();
  u8 ctrl_nt = reg->ctrl & 3;
  u8 ctrl_prg = (reg->ctrl >> 2) & 3;
  u8 ctrl_chr = reg->ctrl >> 4;
  u8 chr_page0 = reg->chr0;
  u8 chr_page1 = ctrl_chr ? reg->chr1 : reg->chr0;
  u8 prg_page = reg->prg & 0x0F;
  u8 prg_ram_page = 0;
  u8 prg_sup0 = 0;
  u8 prg_sup1 = 0;
  u8 prg_last = -1 & 0x0F;
  bool prg_ram_disabled = false;

  bool snrom = (rom->prg_rom_size <= 256 * 1024 && rom->prg_ram_size == 8 * 1024
                && (rom->chr_rom_size == 8 * 1024 || rom->chr_ram_size == 8 * 1024));

  bool szrom = (rom->prg_ram_size == 16 * 1024 && (rom->chr_rom_size >= 16 * 1024 || rom->chr_ram_size >= 16 * 1024));

  if (reg->mmc1a) {
    if (reg->prg & 0x10) {
      prg_sup0 |= (prg_page & 0x08);
      prg_sup1 |= (prg_page & 0x08);
      prg_page &= 7;
      prg_last &= 7;
    }
  } else if (snrom) {
    prg_ram_disabled = (chr_page0 & 0x10);
  } else {
    prg_ram_disabled = (reg->prg & 0x10);
  }

  if (szrom) {
    prg_ram_page = (chr_page0 >> 4) & 1;
  } else if (rom->prg_ram_size == 16 * 1024) {
    prg_ram_page = (chr_page0 >> 3) & 1;
  } else if (rom->prg_ram_size == 32 * 1024) {
    prg_ram_page = (chr_page0 >> 2) & 3;
  }

  if (rom->prg_rom_size == 512 * 1024) {
    prg_sup0 |= chr_page0 & 0x10;
    prg_sup1 |= chr_page1 & 0x10;
  }

  if (!rom->alt_mirror) {
    switch (ctrl_nt) {
      case 0: map_ciram_single_low(); break;
      case 1: map_ciram_single_high(); break;
      case 2: map_ciram_vert_mirror(); break;
      case 3: map_ciram_horiz_mirror(); break;
    }
  }

  if (prg_ram_disabled) {
    map_prg_clear_page_8k(3);
  } else {
    map_prg_ram_page_8k(prg_ram_page, 3, false);
  }

  if (ctrl_chr) {
    map_chr_page_4k(chr_page0, 0);
    map_chr_page_4k(chr_page1, 1);
  } else {
    map_chr_page_8k(chr_page0 >> 1, 0);
  }

  if (!reg->no_prg) {
    switch (ctrl_prg) {
      case 0:
      case 1: map_prg_rom_page_32k((prg_sup0 | prg_page) >> 1, 1); break;
      case 2:
        map_prg_rom_page_16k(prg_sup0, 2);
        map_prg_rom_page_16k((prg_sup1 | prg_page), 3);
        break;
      case 3:
        map_prg_rom_page_16k(prg_sup0 | prg_page, 2);
        map_prg_rom_page_16k(prg_sup1 | prg_last, 3);
        break;
    }
  }
}

static bool map_mmc1_cpu_write(uint addr, uint val)
{
  map_mmc1* reg = (map_mmc1*)map_reg();
  if (addr < 0x8000) {
    return false;
  }
  if (val & 0x80) {
    reg->ctrl |= 0x0C;
    reg->shift = 0x10;
    map_mmc1_update();
  } else {
    if (reg->delay > 0) {
      return true;
    }
    reg->delay = 2;
    bool last = reg->shift & 1;
    reg->shift >>= 1;
    reg->shift |= (val & 1) << 4;
    if (last) {
      uint idx = (addr >> 13) & 3;
      map_reg()[idx] = reg->shift & 0x1F;
      reg->shift = 0x10;
      map_mmc1_update();
    }
  }
  return true;
}

static void map_mmc1_cpu_cyc()
{
  map_mmc1* reg = (map_mmc1*)map_reg();
  if (reg->delay > 0) {
    --reg->delay;
  }
}

static void map_mmc1_load(bool init)
{
  map_mmc1* reg = (map_mmc1*)map_reg();
  mn_rom rom = mn_rom_get();
  reg->no_prg = (rom->submapper == 5);
  map_set_cpu_write_cb(map_mmc1_cpu_write);
  map_set_cpu_cyc_cb(map_mmc1_cpu_cyc);
  if (init) {
    reg->ctrl = 0x0F;
    reg->chr0 = 0x00;
    reg->chr1 = 0x01;
    reg->prg = 0x00;
    reg->shift = 0x10;
  }
  map_mmc1_update();
}

static void map_mmc1a_load(bool init)
{
  map_mmc1* reg = (map_mmc1*)map_reg();
  reg->mmc1a = true;
  map_mmc1_load(init);
}

typedef struct
{
  u8 chr0;
  u8 chr1;
  u8 chr2;
  u8 chr3;
  u8 chr4;
  u8 chr5;
  u8 prg0;
  u8 prg1;
  u8 ctrl;
  u8 filter;
  u8 counter;
  u8 latch;
  u8 reload;
  u8 enabled;
  u8 old_irq;
  u8 alt_mirror;
  u8 use_chr_ram;
  u8 has_outer_chr;
  u8 outer_chr0;
  u8 outer_chr1;
} map_mmc3;
static_assert(sizeof(map_mmc3) <= MAP_REG_SIZE);

static void map_mmc3_update()
{
  map_mmc3* reg = (map_mmc3*)map_reg();
  uint chr0 = reg->chr0;
  uint chr1 = reg->chr1;
  uint chr2 = reg->chr2;
  uint chr3 = reg->chr3;
  uint chr4 = reg->chr4;
  uint chr5 = reg->chr5;
  uint outer_chr0 = (reg->outer_chr0 << 8);
  uint outer_chr1 = (reg->outer_chr1 << 8);
  if (reg->alt_mirror) {
    chr0 &= 0x7F;
    chr1 &= 0x7F;
    chr2 &= 0x7F;
    chr3 &= 0x7F;
    chr4 &= 0x7F;
    chr5 &= 0x7F;
    if (reg->ctrl & 0x80) {
      map_ciram_page(reg->chr2 >> 7, 0x8);
      map_ciram_page(reg->chr3 >> 7, 0x9);
      map_ciram_page(reg->chr4 >> 7, 0xA);
      map_ciram_page(reg->chr5 >> 7, 0xB);
    } else {
      map_ciram_page(reg->chr0 >> 7, 0x8);
      map_ciram_page(reg->chr0 >> 7, 0x9);
      map_ciram_page(reg->chr1 >> 7, 0xA);
      map_ciram_page(reg->chr1 >> 7, 0xB);
    }
  }
  if (reg->use_chr_ram) {
    chr0 &= 0x3F;
    chr1 &= 0x3F;
    chr2 &= 0x3F;
    chr3 &= 0x3F;
    chr4 &= 0x3F;
    chr5 &= 0x3F;
    uint ram_offset = (mn_rom_get()->chr_rom_size >> 10);
    chr0 += (reg->chr0 & 0x40) ? ram_offset : 0;
    chr1 += (reg->chr1 & 0x40) ? ram_offset : 0;
    chr2 += (reg->chr2 & 0x40) ? ram_offset : 0;
    chr3 += (reg->chr3 & 0x40) ? ram_offset : 0;
    chr4 += (reg->chr4 & 0x40) ? ram_offset : 0;
    chr5 += (reg->chr5 & 0x40) ? ram_offset : 0;
  }
  if (reg->ctrl & 0x80) {
    map_chr_page_2k((outer_chr1 | chr0) >> 1, 2);
    map_chr_page_2k((outer_chr1 | chr1) >> 1, 3);
    map_chr_page_1k(reg->outer_chr0 | chr2, 0);
    map_chr_page_1k(reg->outer_chr0 | chr3, 1);
    map_chr_page_1k(reg->outer_chr0 | chr4, 2);
    map_chr_page_1k(reg->outer_chr0 | chr5, 3);
  } else {
    map_chr_page_2k((outer_chr0 | chr0) >> 1, 0);
    map_chr_page_2k((outer_chr0 | chr1) >> 1, 1);
    map_chr_page_1k(outer_chr1 | chr2, 4);
    map_chr_page_1k(outer_chr1 | chr3, 5);
    map_chr_page_1k(outer_chr1 | chr4, 6);
    map_chr_page_1k(outer_chr1 | chr5, 7);
  }
  if (reg->ctrl & 0x40) {
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

static bool map_mmc3_cpu_write(uint addr, uint val)
{
  map_mmc3* reg = (map_mmc3*)map_reg();
  switch (addr & 0xE001) {
    case 0x8000:
      reg->ctrl = val;
      map_mmc3_update();
      return true;
    case 0x8001:
      map_reg()[reg->ctrl & 7] = val;
      map_mmc3_update();
      return true;
    case 0xA000:
      if (!mn_rom_get()->alt_mirror && !reg->alt_mirror) {
        if (val & 1) {
          map_ciram_horiz_mirror();
        } else {
          map_ciram_vert_mirror();
        }
      }
      return true;
    case 0xA001:
      // sram protect not needed?
      // if (val & 0x80) {
      //   map_prg_ram_page_8k(0, 3, val & 0x40);
      // } else {
      //   map_prg_clear_page_8k(3);
      // }
      return true;
    case 0xC000: reg->latch = val; return true;
    case 0xC001: reg->reload = 1; return true;
    case 0xE000:
      reg->enabled = 0;
      map_irq(false);
      return true;
    case 0xE001: reg->enabled = 1; return true;
  }
  if (reg->has_outer_chr && (addr & 0xE100)) {
    reg->outer_chr0 = (val & 1);
    reg->outer_chr1 = ((val & 0x10) >> 4);
    map_mmc3_update();
  }
  return false;
}

static void map_mmc3_ppu_addr(uint addr)
{
  map_mmc3* reg = (map_mmc3*)map_reg();
  if (addr & 0x1000) {
    if (reg->filter == 0) {
      if (reg->counter == 0 || reg->reload) {
        reg->counter = reg->latch;
      } else {
        --reg->counter;
      }
      if (reg->counter == 0 && reg->enabled && (reg->reload || !reg->old_irq || reg->latch)) {
        map_irq(true);
      }
      reg->reload = 0;
    }
    reg->filter = 5;
  } else if (reg->filter == 5) {
    --reg->filter;
  }
}

static void map_mmc3_cpu_cyc()
{
  map_mmc3* reg = (map_mmc3*)map_reg();
  if (reg->filter > 0 && reg->filter < 5) {
    --reg->filter;
  }
}

static void map_mmc3_set_cb()
{
  map_set_cpu_cyc_cb(map_mmc3_cpu_cyc);
  map_set_cpu_write_cb(map_mmc3_cpu_write);
  map_set_ppu_addr_cb(map_mmc3_ppu_addr);
}

static void map_mmc3_load()
{
  map_mmc3* reg = (map_mmc3*)map_reg();
  mn_rom rom = mn_rom_get();
  reg->old_irq = (rom->submapper == 4);
  map_mmc3_set_cb();
  map_mmc3_update();
}

static void map_mmc3a_huang1_load()
{
  map_mmc3* reg = (map_mmc3*)map_reg();
  reg->old_irq = true;
  reg->has_outer_chr = true;
  map_mmc3_set_cb();
  map_mmc3_update();
}

static void map_mmc3_txsrom_load()
{
  map_mmc3* reg = (map_mmc3*)map_reg();
  reg->alt_mirror = true;
  map_mmc3_set_cb();
  map_mmc3_update();
}

static void map_mmc3_tqrom_load()
{
  map_mmc3* reg = (map_mmc3*)map_reg();
  reg->use_chr_ram = true;
  map_mmc3_set_cb();
  map_mmc3_update();
}

typedef struct
{
  u8 chr0fd;
  u8 chr0fe;
  u8 chr1fd;
  u8 chr1fe;
  u8 latch0;
  u8 latch1;
} map_mmc2;
static_assert(sizeof(map_mmc2) <= MAP_REG_SIZE);

static bool map_mmc2_cpu_write(uint addr, uint val)
{
  map_mmc2* reg = (map_mmc2*)map_reg();
  switch (addr & 0xF000) {
    case 0xA000: map_prg_rom_page_8k(val & 0x0F, 4); return true;
    case 0xB000:
      reg->chr0fd = val & 0x1F;
      if (reg->latch0 == 0xFD) {
        map_chr_page_4k(reg->chr0fd, 0);
      }
      return true;
    case 0xC000:
      reg->chr0fe = val & 0x1F;
      if (reg->latch0 == 0xFE) {
        map_chr_page_4k(reg->chr0fe, 0);
      }
      return true;
    case 0xD000:
      reg->chr1fd = val & 0x1F;
      if (reg->latch1 == 0xFD) {
        map_chr_page_4k(reg->chr1fd, 1);
      }
      return true;
    case 0xE000:
      reg->chr1fe = val & 0x1F;
      if (reg->latch1 == 0xFE) {
        map_chr_page_4k(reg->chr1fe, 1);
      }
      return true;
    case 0xF000:
      if (val & 1) {
        map_ciram_horiz_mirror();
      } else {
        map_ciram_vert_mirror();
      }
      return true;
  }
  return false;
}

static bool map_mmc2_ppu_read(uint addr, uint* val)
{
  *val = map_ppu_read_raw(addr, *val);
  map_mmc2* reg = (map_mmc2*)map_reg();
  if (addr == 0x0FD8) {
    reg->latch0 = 0xFD;
    map_chr_page_4k(reg->chr0fd, 0);
  } else if (addr == 0x0FE8) {
    reg->latch0 = 0xFE;
    map_chr_page_4k(reg->chr0fe, 0);
  } else if ((addr & 0xFFF8) == 0x1FD8) {
    reg->latch1 = 0xFD;
    map_chr_page_4k(reg->chr1fd, 1);
  } else if ((addr & 0xFFF8) == 0x1FE8) {
    reg->latch1 = 0xFE;
    map_chr_page_4k(reg->chr1fe, 1);
  }
  return true;
}

static void map_mmc2_load(bool init)
{
  map_set_cpu_write_cb(map_mmc2_cpu_write);
  map_set_ppu_read_cb(map_mmc2_ppu_read);
  map_prg_rom_page_8k(0, 4);
  map_prg_rom_page_8k(-3, 5);
  map_prg_rom_page_8k(-2, 6);
  map_prg_rom_page_8k(-1, 7);
  if (init) {
    map_mmc2* reg = (map_mmc2*)map_reg();
    reg->latch0 = 0xFD;
    reg->latch1 = 0xFD;
  }
}

typedef struct
{
  u8 chr0fd;
  u8 chr0fe;
  u8 chr1fd;
  u8 chr1fe;
  u8 latch0;
  u8 latch1;
} map_mmc4;
static_assert(sizeof(map_mmc4) <= MAP_REG_SIZE);

static bool map_mmc4_cpu_write(uint addr, uint val)
{
  map_mmc4* reg = (map_mmc4*)map_reg();
  switch (addr & 0xF000) {
    case 0xA000: map_prg_rom_page_16k(val & 0x0F, 2); return true;
    case 0xB000:
      reg->chr0fd = val & 0x1F;
      if (reg->latch0 == 0xFD) {
        map_chr_page_4k(reg->chr0fd, 0);
      }
      return true;
    case 0xC000:
      reg->chr0fe = val & 0x1F;
      if (reg->latch0 == 0xFE) {
        map_chr_page_4k(reg->chr0fe, 0);
      }
      return true;
    case 0xD000:
      reg->chr1fd = val & 0x1F;
      if (reg->latch1 == 0xFD) {
        map_chr_page_4k(reg->chr1fd, 1);
      }
      return true;
    case 0xE000:
      reg->chr1fe = val & 0x1F;
      if (reg->latch1 == 0xFE) {
        map_chr_page_4k(reg->chr1fe, 1);
      }
      return true;
    case 0xF000:
      if (val & 1) {
        map_ciram_horiz_mirror();
      } else {
        map_ciram_vert_mirror();
      }
      return true;
  }
  return false;
}

static bool map_mmc4_ppu_read(uint addr, uint* val)
{
  *val = map_ppu_read_raw(addr, *val);
  map_mmc4* reg = (map_mmc4*)map_reg();
  if ((addr & 0xFFF8) == 0x0FD8) {
    reg->latch0 = 0xFD;
    map_chr_page_4k(reg->chr0fd, 0);
  } else if ((addr & 0xFFF8) == 0x0FE8) {
    reg->latch0 = 0xFE;
    map_chr_page_4k(reg->chr0fe, 0);
  } else if ((addr & 0xFFF8) == 0x1FD8) {
    reg->latch1 = 0xFD;
    map_chr_page_4k(reg->chr1fd, 1);
  } else if ((addr & 0xFFF8) == 0x1FE8) {
    reg->latch1 = 0xFE;
    map_chr_page_4k(reg->chr1fe, 1);
  }
  return true;
}

static void map_mmc4_load(bool init)
{
  map_set_cpu_write_cb(map_mmc4_cpu_write);
  map_set_ppu_read_cb(map_mmc4_ppu_read);
  if (init) {
    map_mmc2* reg = (map_mmc2*)map_reg();
    reg->latch0 = 0xFD;
    reg->latch1 = 0xFD;
  }
}

typedef struct
{
  uint irq_last_addr;
  u8 prg_mode;
  u8 chr_mode;
  u8 prg_ram_protect1;
  u8 prg_ram_protect2;
  u8 ext_mode;
  u8 nt_mode;
  u8 fill_tile;
  u8 fill_attr;
  u8 prg_page[5];
  u8 chr_page[12];
  u8 chr_outer;
  u8 vert_split_mode;
  u8 vert_split_scroll;
  u8 vert_split_bank;
  u8 irq_latch;
  u8 irq_enabled;
  u8 irq_status;
  u8 irq_counter;
  u8 irq_inframe_filter_on;
  u8 irq_inframe_filter_off;
  u8 mul_a;
  u8 mul_b;
  u8 latch_2001;
} map_mmc5;
static_assert(sizeof(map_mmc5) <= MAP_REG_SIZE);

static void map_mmc5_prg_8k(u8 reg, u8 dp, bool ram_readonly)
{
  if (reg & 0x80) {
    map_prg_rom_page_8k(reg, dp);
  } else {
    map_prg_ram_page_8k(reg, dp, ram_readonly);
  }
}

static void map_mmc5_prg_16k(u8 reg, u8 dp, bool ram_readonly)
{
  if (reg & 0x80) {
    map_prg_rom_page_16k(reg >> 1, dp);
  } else {
    // map_prg_ram_page_16k(reg >> 1, dp, ram_readonly);
    map_prg_ram_page_8k(reg & ~1, dp << 1, ram_readonly);
    map_prg_ram_page_8k(reg | 1, (dp << 1) | 1, ram_readonly);
  }
}

static void map_mmc5_update_prg()
{
  map_mmc5* reg = (map_mmc5*)map_reg();
  bool ram_readonly = !(reg->prg_ram_protect1 == 2 && reg->prg_ram_protect2 == 1);
  map_prg_ram_page_8k(reg->prg_page[0], 3, ram_readonly);
  switch (reg->prg_mode) {
    case 0:
      // map_prg_rom_page_32k(reg->prg_page[4] >> 2, 1);
      map_prg_rom_page_16k((reg->prg_page[4] >> 1) & ~1, 2);
      map_prg_rom_page_16k((reg->prg_page[4] >> 1) | 1, 3);
      break;
    case 1:
      map_mmc5_prg_16k(reg->prg_page[2], 2, ram_readonly);
      map_prg_rom_page_16k(reg->prg_page[4] >> 1, 3);
      break;
    case 2:
      map_mmc5_prg_16k(reg->prg_page[2], 2, ram_readonly);
      map_mmc5_prg_8k(reg->prg_page[3], 6, ram_readonly);
      map_prg_rom_page_8k(reg->prg_page[4], 7);
      break;
    case 3:
      map_mmc5_prg_8k(reg->prg_page[1], 4, ram_readonly);
      map_mmc5_prg_8k(reg->prg_page[2], 5, ram_readonly);
      map_mmc5_prg_8k(reg->prg_page[3], 6, ram_readonly);
      map_prg_rom_page_8k(reg->prg_page[4], 7);
      break;
  }
}

static void map_mmc5_update_chr()
{
  map_mmc5* reg = (map_mmc5*)map_reg();
  uint chr_outer = reg->chr_outer << 8;
  switch (reg->chr_mode) {
    case 0: map_chr_page_8k(reg->chr_page[7] | chr_outer, 0); break;
    case 1:
      map_chr_page_4k(reg->chr_page[3] | chr_outer, 0);
      map_chr_page_4k(reg->chr_page[7] | chr_outer, 1);
      break;
    case 2:
      map_chr_page_2k(reg->chr_page[1] | chr_outer, 0);
      map_chr_page_2k(reg->chr_page[3] | chr_outer, 1);
      map_chr_page_2k(reg->chr_page[5] | chr_outer, 2);
      map_chr_page_2k(reg->chr_page[7] | chr_outer, 3);
      break;
    case 3:
      map_chr_page_1k(reg->chr_page[0] | chr_outer, 0);
      map_chr_page_1k(reg->chr_page[1] | chr_outer, 1);
      map_chr_page_1k(reg->chr_page[2] | chr_outer, 2);
      map_chr_page_1k(reg->chr_page[3] | chr_outer, 3);
      map_chr_page_1k(reg->chr_page[8] | chr_outer, 4); // todo
      map_chr_page_1k(reg->chr_page[9] | chr_outer, 5);
      map_chr_page_1k(reg->chr_page[10] | chr_outer, 6);
      map_chr_page_1k(reg->chr_page[11] | chr_outer, 7);
      break;
  }
}

static void map_mmc5_nt_page(uint mode, uint dp)
{
  switch (mode) {
    case 0: map_ciram_page(0, dp); break;
    case 1: map_ciram_page(1, dp); break;
    case 2:
      // todo: exram, check ext_mode
      map_ciram_page(2, dp);
      break;
    case 3:
      // todo: fill with fill_tile and fill_color
      map_ciram_page(3, dp);
      break;
  }
}

static void map_mmc5_update_nt()
{
  map_mmc5* reg = (map_mmc5*)map_reg();
  map_cpu_ciram_page(2, 23, reg->ext_mode == 3); // ExRAM -> 0x5C00-0x5FFF
  map_mmc5_nt_page((reg->nt_mode >> 0) & 3, 0x8);
  map_mmc5_nt_page((reg->nt_mode >> 2) & 3, 0x9);
  map_mmc5_nt_page((reg->nt_mode >> 4) & 3, 0xA);
  map_mmc5_nt_page((reg->nt_mode >> 6) & 3, 0xB);
}

static void map_mmc5_irq_cancel()
{
  map_mmc5* reg = (map_mmc5*)map_reg();
  reg->irq_status = 0;
  reg->irq_counter = 0;
  map_irq(false);
}

static void map_mmc5_reset()
{
  map_mmc5* reg = (map_mmc5*)map_reg();
  reg->prg_mode = 3;
  reg->chr_mode = 3;
  reg->prg_ram_protect1 = 1;
  reg->prg_ram_protect2 = 2;
  reg->ext_mode = 3;
  reg->prg_page[4] = 0xFF;
  reg->chr_outer = 0;
  reg->vert_split_mode = 0;
  reg->vert_split_scroll = 0;
  reg->irq_enabled = false;
  map_irq(false);
  map_mmc5_update_prg();
  map_mmc5_update_chr();
  map_mmc5_update_nt();
}

static bool map_mmc5_cpu_write(uint addr, uint val)
{
  if (addr < 0x5000 || addr >= 0x6000) {
    return false;
  }
  if (addr >= 0x5C00 && addr <= 0x5FFF) {
    return false;
  }
  map_mmc5* reg = (map_mmc5*)map_reg();
  switch (addr) {
    case 0x5000:
    case 0x5001:
    case 0x5002:
    case 0x5003:
    case 0x5004:
    case 0x5005:
    case 0x5006:
    case 0x5007:
    case 0x5008:
    case 0x5009:
    case 0x500A:
    case 0x500B:
    case 0x500C:
    case 0x500D:
    case 0x500E:
    case 0x500F:
    case 0x5010:
    case 0x5011:
    case 0x5012:
    case 0x5013:
    case 0x5014:
    case 0x5015:
      // todo: audio
      break;
    case 0x5100:
      reg->prg_mode = (val & 3);
      map_mmc5_update_prg();
      break;
    case 0x5101:
      reg->chr_mode = (val & 3);
      map_mmc5_update_chr();
      break;
    case 0x5102:
      reg->prg_ram_protect1 = (val & 3);
      map_mmc5_update_prg();
      break;
    case 0x5103:
      reg->prg_ram_protect2 = (val & 3);
      map_mmc5_update_prg();
      break;
    case 0x5104:
      reg->ext_mode = (val & 3);
      map_mmc5_update_nt();
      break;
    case 0x5105:
      reg->nt_mode = val;
      map_mmc5_update_nt();
      break;
    case 0x5106:
      reg->fill_tile = val;
      map_mmc5_update_nt();
      break;
    case 0x5107:
      reg->fill_attr = (val & 3);
      map_mmc5_update_nt();
      break;
    case 0x5113:
    case 0x5114:
    case 0x5115:
    case 0x5116:
    case 0x5117:
      reg->prg_page[addr - 0x5113] = val;
      map_mmc5_update_prg();
      break;
    case 0x5120:
    case 0x5121:
    case 0x5122:
    case 0x5123:
    case 0x5124:
    case 0x5125:
    case 0x5126:
    case 0x5127:
    case 0x5128:
    case 0x5129:
    case 0x512A:
    case 0x512B:
      reg->chr_page[addr - 0x5120] = val;
      map_mmc5_update_chr();
      break;
    case 0x5130:
      reg->chr_outer = (val & 3);
      map_mmc5_update_chr();
      map_mmc5_update_nt();
      break;
    case 0x5200: reg->vert_split_mode = val; break;
    case 0x5201: reg->vert_split_scroll = val; break;
    case 0x5202: reg->vert_split_bank = val; break;
    case 0x5203: reg->irq_latch = val; break;
    case 0x5204:
      reg->irq_enabled = (val & 0x80);
      map_irq(reg->irq_enabled && (reg->irq_status & 0x80));
      break;
    case 0x5205: reg->mul_a = val; break;
    case 0x5206: reg->mul_b = val; break;
    case 0x5207:
    case 0x5208:
    case 0x5209:
    case 0x520A:
      // todo: MMC5A: 0x5207 0x5208 0x5209 0x520A 0x5800-0x5BFF
      break;
  }
  return true;
}

static bool map_mmc5_cpu_read(uint addr, uint* val, bool trace)
{
  map_mmc5* reg = (map_mmc5*)map_reg();
  if (addr >= 0x5C00 && addr <= 0x5FFF) {
    return false;
  }
  switch (addr) {
    case 0xFFFA:
    case 0xFFFB: map_mmc5_irq_cancel(); return false; // NMI
    case 0xFFFC:
    case 0xFFFD: map_mmc5_reset(); return false; // Reset
    case 0x5204: {
      bool m2_vs_cpu_a0 = false; // todo:?
      *val = reg->irq_status | m2_vs_cpu_a0;
      if (!trace) {
        reg->irq_status &= ~0x80;
        map_irq(false);
      }
      return true;
    }
    case 0x5205: {
      uint mul = reg->mul_a * reg->mul_b;
      *val = (mul & 0xFF);
      return true;
    }
    case 0x5206: {
      uint mul = reg->mul_a * reg->mul_b;
      *val = ((mul >> 8) & 0xFF);
      return true;
    }
  }
  return false;
}

static bool map_mmc5_ppu_read(uint addr, uint*)
{
  map_mmc5* reg = (map_mmc5*)map_reg();
  reg->irq_inframe_filter_off = 0;
  if (reg->irq_inframe_filter_on >= 2) {
    if (reg->irq_status & 0x40) {
      if (++reg->irq_counter == reg->irq_latch) {
        reg->irq_status |= 0x80;
        if (reg->irq_enabled) {
          map_irq(true);
        }
      }
    } else {
      reg->irq_status |= 0x40;
      reg->irq_counter = 0;
    }
  }
  if (addr >= 0x2000 && addr <= 0x2FFF && addr == reg->irq_last_addr) {
    ++reg->irq_inframe_filter_on;
  } else {
    reg->irq_inframe_filter_on = 0;
  }
  reg->irq_last_addr = addr;
  return false;
}

static void map_mmc5_cpu_cyc()
{
  map_mmc5* reg = (map_mmc5*)map_reg();
  if (++reg->irq_inframe_filter_off >= 3) {
    reg->irq_status &= ~0x40;
  }
}

static void map_mmc5_cpu_internal(uint addr, uint val)
{
  map_mmc5* reg = (map_mmc5*)map_reg();
  switch (addr) {
    case 0x2000:
      if (val & 0x20) {
        // todo: 8x16 sprite size?
      }
      break;
    case 0x2001:
      if (!(reg->latch_2001 & 0x18) && (val & 0x18)) {
        // map_mmc5_irq_cancel(); ?
      }
      if (!(val & 0x18)) {
        // todo: disable extensions?
      }
      reg->latch_2001 = val;
      break;
    case 0x2002:
      if (val & 0x80) {
        reg->irq_counter = 0;
      }
      break;
    // case 0x2005: break; // todo: val & 7
    // case 0x2006: break; // todo:??
    case 0x4014: map_mmc5_irq_cancel(); break;
  }
}

void map_mmc5_load(bool init)
{
  map_set_cpu_cyc_cb(map_mmc5_cpu_cyc);
  map_set_cpu_read_cb(map_mmc5_cpu_read);
  map_set_cpu_write_cb(map_mmc5_cpu_write);
  map_set_cpu_internal_cb(map_mmc5_cpu_internal);
  map_set_ppu_read_cb(map_mmc5_ppu_read);
  if (init) {
    map_mmc5_reset();
  } else {
    map_mmc5_update_prg();
    map_mmc5_update_chr();
    map_mmc5_update_nt();
  }
}

bool map_mmc_load(bool init)
{
  mn_rom rom = mn_rom_get();
  switch (rom->mapper) {
    case 1: map_mmc1_load(init); break;
    case 4: map_mmc3_load(); break;
    case 5: map_mmc5_load(init); break;
    case 9: map_mmc2_load(init); break;
    case 10: map_mmc4_load(init); break;
    case 12: map_mmc3a_huang1_load(); break;
    case 118: map_mmc3_txsrom_load(); break;
    case 119: map_mmc3_tqrom_load(); break;
    case 155: map_mmc1a_load(init); break;
    default: return false;
  }
  return true;
}
