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

  if (snrom) {
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
    prg_sup0 = chr_page0 & 0x10;
    prg_sup1 = chr_page1 & 0x10;
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

static bool map_mmc1_cpu_write(u16 addr, u8 val)
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

static void map_mmc1a_load(bool init) { map_mmc1_load(init); }

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
    map_chr_page_2k(chr0 >> 1, 2);
    map_chr_page_2k(chr1 >> 1, 3);
    map_chr_page_1k(chr2, 0);
    map_chr_page_1k(chr3, 1);
    map_chr_page_1k(chr4, 2);
    map_chr_page_1k(chr5, 3);
  } else {
    map_chr_page_2k(chr0 >> 1, 0);
    map_chr_page_2k(chr1 >> 1, 1);
    map_chr_page_1k(chr2, 4);
    map_chr_page_1k(chr3, 5);
    map_chr_page_1k(chr4, 6);
    map_chr_page_1k(chr5, 7);
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

static bool map_mmc3_cpu_write(u16 addr, u8 val)
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
  return false;
}

static void map_mmc3_ppu_addr(u16 addr)
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

static void map_mmc3a_load()
{
  map_mmc3* reg = (map_mmc3*)map_reg();
  reg->old_irq = true;
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

static bool map_mmc2_cpu_write(u16 addr, u8 val)
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

static bool map_mmc2_ppu_read(u16 addr, u8* val)
{
  map_ppu_read_raw(addr, val);
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

static bool map_mmc4_cpu_write(u16 addr, u8 val)
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

static bool map_mmc4_ppu_read(u16 addr, u8* val)
{
  map_ppu_read_raw(addr, val);
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

/*
enum
{
  MAP_MMC5_APU_END = 0x1015,

  MAP_MMC5_PRG_MODE = 0x1100,
  MAP_MMC5_CHR_MODE = 0x1101,
  MAP_MMC5_PRG_RAM_PROTECT1 = 0x1102,
  MAP_MMC5_PRG_RAM_PROTECT2 = 0x1103,
  MAP_MMC5_EXT_MODE = 0x1104,
  MAP_MMC5_NT_MODE = 0x1105,
  MAP_MMC5_FILL_TILE = 0x1106,
  MAP_MMC5_FILL_COLOR = 0x1107,
  MAP_MMC5_PRG_BASE = 0x1113,
  MAP_MMC5_CHR_BASE = 0x1120,
  MAP_MMC5_CHR_HIGH = 0x1130,

  MAP_MMC5_VERT_SPLIT = 0x1200,
  MAP_MMC5_VERT_SPLIT_SCROLL = 0x1201,
  MAP_MMC5_VERT_SPLIT_BANK = 0x1202,

  MAP_MMC5_IRQ_LATCH = 0x1203,
  MAP_MMC5_IRQ_ENABLED = 0x1204, // val & 0x80

  MAP_MMC5_MUL_A = 0x1205,
  MAP_MMC5_MUL_B = 0x1206,

  MAP_MMC5_ERAM = 0x1C00,

  // custom
  MAP_MMC5_IRQ_STATUS = 0x1300,
  MAP_MMC5_IRQ_A12 = 0x1301,
  MAP_MMC5_IRQ_COUNTER = 0x1302,
};

static void map_mmc5_prg_8k(u8 reg, u8 dp)
{
  if (reg & 0x80) {
    map_prg_page_8k(reg, dp);
  } else {
    map_sram_page_8k(reg, dp);
  }
}

static void map_mmc5_prg_16k(u8 reg, u8 dp)
{
  if (reg & 0x80) {
    map_prg_page_16k(reg >> 1, dp);
  } else {
    map_sram_page_16k(reg >> 1, dp);
  }
}

static void map_mmc5_update_prg()
{
  map_mmc5_prg_8k(map_dyn.eram[MAP_MMC5_PRG_BASE + 0], 3);

  switch (map_dyn.eram[MAP_MMC5_PRG_MODE]) {
    case 0: map_prg_page_32k(map_dyn.eram[MAP_MMC5_PRG_BASE + 4] >> 2, 2); break;
    case 1:
      map_mmc5_prg_16k(map_dyn.eram[MAP_MMC5_PRG_BASE + 2], 2);
      map_prg_page_16k(map_dyn.eram[MAP_MMC5_PRG_BASE + 4] >> 1, 3);
      break;
    case 2:
      map_mmc5_prg_16k(map_dyn.eram[MAP_MMC5_PRG_BASE + 2], 2);
      map_mmc5_prg_8k(map_dyn.eram[MAP_MMC5_PRG_BASE + 3], 6);
      map_prg_page_8k(map_dyn.eram[MAP_MMC5_PRG_BASE + 4], 7);
      break;
    case 3:
      map_mmc5_prg_8k(map_dyn.eram[MAP_MMC5_PRG_BASE + 1], 4);
      map_mmc5_prg_8k(map_dyn.eram[MAP_MMC5_PRG_BASE + 2], 5);
      map_mmc5_prg_8k(map_dyn.eram[MAP_MMC5_PRG_BASE + 3], 6);
      map_prg_page_8k(map_dyn.eram[MAP_MMC5_PRG_BASE + 4], 7);
      break;
  }
}

static void map_mmc5_update_chr()
{
  u16 chr_hi = map_dyn.eram[MAP_MMC5_CHR_HIGH] << 8;
  switch (map_dyn.eram[MAP_MMC5_CHR_MODE]) {
    case 0: map_chr_page_8k(map_dyn.eram[MAP_MMC5_CHR_BASE + 7] | chr_hi, 0); break;
    case 1:
      map_chr_page_4k(map_dyn.eram[MAP_MMC5_CHR_BASE + 3] | chr_hi, 0);
      map_chr_page_4k(map_dyn.eram[MAP_MMC5_CHR_BASE + 7] | chr_hi, 1);
      break;
    case 2:
      map_chr_page_2k(map_dyn.eram[MAP_MMC5_CHR_BASE + 1] | chr_hi, 0);
      map_chr_page_2k(map_dyn.eram[MAP_MMC5_CHR_BASE + 3] | chr_hi, 1);
      map_chr_page_2k(map_dyn.eram[MAP_MMC5_CHR_BASE + 5] | chr_hi, 2);
      map_chr_page_2k(map_dyn.eram[MAP_MMC5_CHR_BASE + 7] | chr_hi, 3);
      break;
    case 3:
      for (int i = 0; i < 8; i++) {
        map_chr_page_1k(map_dyn.eram[MAP_MMC5_CHR_BASE + i] | chr_hi, i);
      }
      break;
  }
}

static bool map_mmc5_cpu_write(u16 addr, u8 val)
{
  if (addr < 0x5000 || addr >= 0x6000) {
    return false;
  }
  addr &= 0x1FFF;
  if (addr < MAP_MMC5_APU_END) {
    // todo: apu
    return true;
  }
  if (addr == MAP_MMC5_PRG_MODE || (addr >= MAP_MMC5_PRG_BASE && addr <= MAP_MMC5_PRG_BASE + 7)) {
    map_dyn.eram[addr] = val;
    map_mmc5_update_prg();
    return true;
  }
  if (addr == MAP_MMC5_CHR_MODE || addr == MAP_MMC5_CHR_HIGH
      || (addr >= MAP_MMC5_CHR_BASE && addr <= MAP_MMC5_CHR_BASE + 7))
  {
    map_dyn.eram[addr] = val;
    map_mmc5_update_chr();
    return true;
  }
  if (addr == MAP_MMC5_IRQ_LATCH || addr == MAP_MMC5_IRQ_ENABLED) {
    map_dyn.eram[addr] = val;
    return true;
  }
  if (addr == MAP_MMC5_NT_MODE) {
    u8 nt0 = addr & 3;
    u8 nt1 = (addr >> 2) & 3;
    u8 nt2 = (addr >> 4) & 3;
    u8 nt3 = addr >> 6;


  map_ciram_page(0x8, map_dyn.ciram + MAP_PPU_PAGE_SIZE * 0);
  map_ciram_page(0x9, map_dyn.ciram + MAP_PPU_PAGE_SIZE * 1);
  map_ciram_page(0xA, map_dyn.ciram + MAP_PPU_PAGE_SIZE * 2);
  map_ciram_page(0xB, map_dyn.ciram + MAP_PPU_PAGE_SIZE * 3);
  }
  // if (addr >= MAP_MMC5_ERAM && map_dyn.eram[MAP_MMC5_EXT_MODE] > 2) { // eram write protect
  //   return true;
  // }
  return false;
}

static bool map_mmc5_cpu_read(u16 addr, u8* val)
{
  if (addr < 0x5000 || addr >= 0x6000) {
    return false;
  }
  addr &= 0x1FFF;
  if (addr == MAP_MMC5_IRQ_ENABLED) {
    *val = map_dyn.eram[MAP_MMC5_IRQ_STATUS];
    map_dyn.eram[MAP_MMC5_IRQ_STATUS] &= ~0x80;
    return true;
  }
  if (addr == MAP_MMC5_MUL_A || addr == MAP_MMC5_MUL_B) {
    u16 result = (u16)map_dyn.eram[MAP_MMC5_MUL_A] * (u16)map_dyn.eram[MAP_MMC5_MUL_B];
    *val = (addr == MAP_MMC5_MUL_A) ? (result & 0xFF) : (result >> 8);
    return true;
  }
  // if (addr >= MAP_MMC5_ERAM && map_dyn.eram[MAP_MMC5_EXT_MODE] < 2) { // eram read protect
  //   return true;
  // }
  return false;
}

static bool map_mmc5_ppu_read(u16 addr, u8*)
{
  if (addr < 0x2000) {
    u8 a12 = (addr >> 12) & 1;
    if (a12 && !map_dyn.eram[MAP_MMC5_IRQ_A12]) {
      // todo
    }
    map_dyn.eram[MAP_MMC5_IRQ_A12] = a12;
  }
  return false;
}

void map_mmc5_load()
{
  map_dyn.cpu_write = map_mmc5_cpu_write;
  map_dyn.cpu_read = map_mmc5_cpu_read;
  map_dyn.ppu_read = map_mmc5_ppu_read;

  map_dyn.eram[MAP_MMC5_PRG_MODE] = 3;
  map_dyn.eram[MAP_MMC5_CHR_MODE] = 3;
  map_dyn.eram[MAP_MMC5_PRG_RAM_PROTECT1] = 3;
  map_dyn.eram[MAP_MMC5_PRG_RAM_PROTECT2] = 3;
  map_dyn.eram[MAP_MMC5_EXT_MODE] = 3;

  map_mmc5_update_prg();
  map_mmc5_update_chr();
}
*/

bool map_mmc_load(bool init)
{
  mn_rom rom = mn_rom_get();
  switch (rom->mapper) {
    case 1: map_mmc1_load(init); break;
    case 4: map_mmc3_load(); break;
    // case 5: map_mmc5_load(); break;
    case 9: map_mmc2_load(init); break;
    case 10: map_mmc4_load(init); break;
    case 12: map_mmc3a_load(); break;
    case 118: map_mmc3_txsrom_load(); break;
    case 119: map_mmc3_tqrom_load(); break;
    case 155: map_mmc1a_load(init); break;
    default: return false;
  }
  return true;
}
