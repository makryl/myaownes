#include "map.h"
#include "cpu.h"
#include "dev.h"
#include "common.h"
#include <string.h>

enum
{
  MAP_CPU_PAGE_SHIFT = 12, // 4kb
  MAP_CPU_PAGE_SIZE = 1 << MAP_CPU_PAGE_SHIFT,
  MAP_CPU_PAGE_MASK = MAP_CPU_PAGE_SIZE - 1,
  MAP_PPU_PAGE_SHIFT = 10, // 1kb
  MAP_PPU_PAGE_SIZE = 1 << MAP_PPU_PAGE_SHIFT,
  MAP_PPU_PAGE_MASK = MAP_PPU_PAGE_SIZE - 1,
};

MN_CACHE_LINE static struct
{
  void (*cpu_cyc)();
  bool (*cpu_read)(u16, u8*, bool);
  bool (*cpu_write)(u16, u8);
  void (*ppu_addr)(u16);
  bool (*ppu_read)(u16, u8*);
  bool (*ppu_write)(u16, u8);

  const u8* cpu_read_page[16];
  u8* cpu_write_page[16];
  const u8* ppu_read_page[16];
  u8* ppu_write_page[16];

  Rom rom;
} mapper;

MN_CACHE_LINE static struct
{
  bool apu_irq;
  bool irq;
  MN_CACHE_LINE u8 vram[0x1000];
  MN_CACHE_LINE u8 eram[0x2000]; // used for mapper registers. <0x4020 never accessed from CPU, even if mapped.
} map;

Rom mn_rom_get() { return mapper.rom; }
uint map_size() { return sizeof(map); }
void* map_data() { return &map; }
bool map_ready() { return mapper.rom && !mapper.rom->mapper_error; }
static void map_cpu_irq() { cpu_irq(map.apu_irq || map.irq); }

void map_apu_irq(bool enabled)
{
  map.apu_irq = enabled;
  map_cpu_irq();
}


void map_cpu_cyc()
{
  if (mapper.cpu_cyc) {
    mapper.cpu_cyc();
  }
}

void map_cpu_read(u16 addr, u8* val, bool trace)
{
  if (!mapper.cpu_read || !mapper.cpu_read(addr, val, trace)) {
    const u8* page = mapper.cpu_read_page[addr >> MAP_CPU_PAGE_SHIFT];
    if (page) {
      *val = page[addr & MAP_CPU_PAGE_MASK];
    }
  }
}

void map_cpu_write(u16 addr, u8 val)
{
  if (!mapper.cpu_write || !mapper.cpu_write(addr, val)) {
    u8* page = mapper.cpu_write_page[addr >> MAP_CPU_PAGE_SHIFT];
    if (page) {
      page[addr & MAP_CPU_PAGE_MASK] = val;
    }
  }
}

void map_ppu_addr(u16 addr)
{
  if (mapper.ppu_addr) {
    mapper.ppu_addr(addr);
  }
}

void map_ppu_read(u16 addr, u8* val)
{
  if (!mapper.ppu_read || !mapper.ppu_read(addr, val)) {
    const u8* page = mapper.ppu_read_page[addr >> MAP_PPU_PAGE_SHIFT];
    if (page) {
      *val = page[addr & MAP_PPU_PAGE_MASK];
    }
  }
}

void map_ppu_write(u16 addr, u8 val)
{
  if (!mapper.ppu_write || !mapper.ppu_write(addr, val)) {
    u8* page = mapper.ppu_write_page[addr >> MAP_PPU_PAGE_SHIFT];
    if (page) {
      page[addr & MAP_PPU_PAGE_MASK] = val;
    }
  }
}

static const u8* map_cpu_read_pages(const u8* src, uint src_page, uint dst_page, uint pages)
{
  const u8* next = nullptr;
  for (uint i = 0; i < pages; ++i) {
    next = src ? src + MAP_CPU_PAGE_SIZE * (src_page + i) : 0;
    mapper.cpu_read_page[dst_page + i] = next;
  }
  return next;
}

static u8* map_cpu_write_pages(u8* src, uint src_page, uint dst_page, uint pages)
{
  u8* next = nullptr;
  for (uint i = 0; i < pages; ++i) {
    next = src ? src + MAP_CPU_PAGE_SIZE * (src_page + i) : 0;
    mapper.cpu_write_page[dst_page + i] = next;
  }
  return next;
}

static const u8* map_ppu_read_pages(const u8* src, uint src_page, uint dst_page, uint pages)
{
  const u8* next = nullptr;
  for (uint i = 0; i < pages; ++i) {
    next = src ? src + MAP_PPU_PAGE_SIZE * (src_page + i) : 0;
    mapper.ppu_read_page[dst_page + i] = next;
  }
  return next;
}

static u8* map_ppu_write_pages(u8* src, uint src_page, uint dst_page, uint pages)
{
  u8* next = nullptr;
  for (uint i = 0; i < pages; ++i) {
    next = src ? src + MAP_PPU_PAGE_SIZE * (src_page + i) : 0;
    mapper.ppu_write_page[dst_page + i] = next;
  }
  return next;
}

static uint map_page_mask(uint n)
{
  --n;
  n |= n >> 1;
  n |= n >> 2;
  n |= n >> 4;
  n |= n >> 8;
  n |= n >> 16;
  return n;
}

static uint map_page_clamp(uint page, uint size, u8 shift)
{
  uint pages = (size >> shift);
  page &= map_page_mask(pages);
  if (page >= pages) {
    page %= pages;
  }
  return page;
}

static void map_prg_rom_page_4k(uint sp, uint dp)
{
  sp = map_page_clamp(sp, mapper.rom->prg_rom_size, MAP_CPU_PAGE_SHIFT + 0);
  map_cpu_read_pages(mapper.rom->prg_rom, sp, dp, 1);
  map_cpu_write_pages(0, sp, dp, 1);
}

static void map_prg_rom_page_8k(uint sp, uint dp)
{
  sp = map_page_clamp(sp, mapper.rom->prg_rom_size, MAP_CPU_PAGE_SHIFT + 1);
  map_cpu_read_pages(mapper.rom->prg_rom, sp * 2, dp * 2, 2);
  map_cpu_write_pages(0, sp * 2, dp * 2, 2);
}

static void map_prg_rom_page_16k(uint sp, uint dp)
{
  sp = map_page_clamp(sp, mapper.rom->prg_rom_size, MAP_CPU_PAGE_SHIFT + 2);
  map_cpu_read_pages(mapper.rom->prg_rom, sp * 4, dp * 4, 4);
  map_cpu_write_pages(0, sp * 4, dp * 4, 4);
}

static void map_prg_rom_page_32k(uint sp, uint dp)
{
  sp = map_page_clamp(sp, mapper.rom->prg_rom_size, MAP_CPU_PAGE_SHIFT + 3);
  map_cpu_read_pages(mapper.rom->prg_rom, sp * 8, dp * 8, 8);
  map_cpu_write_pages(0, sp * 8, dp * 8, 8);
}

static void map_prg_ram_page_4k(uint sp, uint dp)
{
  sp = map_page_clamp(sp, mapper.rom->prg_rom_size, MAP_CPU_PAGE_SHIFT + 0);
  map_cpu_read_pages(mapper.rom->prg_ram, sp, dp, 1);
  map_cpu_write_pages(mapper.rom->prg_ram, sp, dp, 1);
}

static void map_prg_ram_page_8k(uint sp, uint dp)
{
  sp = map_page_clamp(sp, mapper.rom->prg_rom_size, MAP_CPU_PAGE_SHIFT + 1);
  map_cpu_read_pages(mapper.rom->prg_ram, sp * 2, dp * 2, 2);
  map_cpu_write_pages(mapper.rom->prg_ram, sp * 2, dp * 2, 2);
}

static void map_prg_ram_page_16k(uint sp, uint dp)
{
  sp = map_page_clamp(sp, mapper.rom->prg_rom_size, MAP_CPU_PAGE_SHIFT + 2);
  map_cpu_read_pages(mapper.rom->prg_ram, sp * 4, dp * 4, 4);
  map_cpu_write_pages(mapper.rom->prg_ram, sp * 4, dp * 4, 4);
}

static void map_prg_ram_page_32k(uint sp, uint dp)
{
  sp = map_page_clamp(sp, mapper.rom->prg_rom_size, MAP_CPU_PAGE_SHIFT + 3);
  map_cpu_read_pages(mapper.rom->prg_ram, sp * 8, dp * 8, 8);
  map_cpu_write_pages(mapper.rom->prg_ram, sp * 8, dp * 8, 8);
}

static void map_chr_page_1k(uint sp, uint dp)
{
  uint rom_pages = (mapper.rom->chr_rom_size >> (MAP_PPU_PAGE_SHIFT + 0));
  if (sp < rom_pages || mapper.rom->chr_ram_size == 0) {
    sp = map_page_clamp(sp, mapper.rom->chr_rom_size, MAP_PPU_PAGE_SHIFT + 0);
    map_ppu_read_pages(mapper.rom->chr_rom, sp, dp, 1);
    map_ppu_write_pages(0, sp, dp, 1);
  } else {
    sp -= rom_pages;
    sp = map_page_clamp(sp, mapper.rom->chr_ram_size, MAP_PPU_PAGE_SHIFT + 0);
    map_ppu_read_pages(mapper.rom->chr_ram, sp, dp, 1);
    map_ppu_write_pages(mapper.rom->chr_ram, sp, dp, 1);
  }
}

static void map_chr_page_2k(uint sp, uint dp)
{
  uint rom_pages = (mapper.rom->chr_rom_size >> (MAP_PPU_PAGE_SHIFT + 1));
  if (sp < rom_pages || mapper.rom->chr_ram_size == 0) {
    sp = map_page_clamp(sp, mapper.rom->chr_rom_size, MAP_PPU_PAGE_SHIFT + 1);
    map_ppu_read_pages(mapper.rom->chr_rom, sp * 2, dp * 2, 2);
    map_ppu_write_pages(0, sp * 2, dp * 2, 2);
  } else {
    sp -= rom_pages;
    sp = map_page_clamp(sp, mapper.rom->chr_ram_size, MAP_PPU_PAGE_SHIFT + 1);
    map_ppu_read_pages(mapper.rom->chr_ram, sp * 2, dp * 2, 2);
    map_ppu_write_pages(mapper.rom->chr_ram, sp * 2, dp * 2, 2);
  }
}

static void map_chr_page_4k(uint sp, uint dp)
{
  uint rom_pages = (mapper.rom->chr_rom_size >> (MAP_PPU_PAGE_SHIFT + 2));
  if (sp < rom_pages || mapper.rom->chr_ram_size == 0) {
    sp = map_page_clamp(sp, mapper.rom->chr_rom_size, MAP_PPU_PAGE_SHIFT + 2);
    map_ppu_read_pages(mapper.rom->chr_rom, sp * 4, dp * 4, 4);
    map_ppu_write_pages(0, sp * 4, dp * 4, 4);
  } else {
    sp -= rom_pages;
    sp = map_page_clamp(sp, mapper.rom->chr_ram_size, MAP_PPU_PAGE_SHIFT + 2);
    map_ppu_read_pages(mapper.rom->chr_ram, sp * 4, dp * 4, 4);
    map_ppu_write_pages(mapper.rom->chr_ram, sp * 4, dp * 4, 4);
  }
}

static void map_chr_page_8k(uint sp, uint dp)
{
  uint rom_pages = (mapper.rom->chr_rom_size >> (MAP_PPU_PAGE_SHIFT + 3));
  if (sp < rom_pages || mapper.rom->chr_ram_size == 0) {
    sp = map_page_clamp(sp, mapper.rom->chr_rom_size, MAP_PPU_PAGE_SHIFT + 3);
    map_ppu_read_pages(mapper.rom->chr_rom, sp * 8, dp * 8, 8);
    map_ppu_write_pages(0, sp * 8, dp * 8, 8);
  } else {
    sp -= rom_pages;
    sp = map_page_clamp(sp, mapper.rom->chr_ram_size, MAP_PPU_PAGE_SHIFT + 3);
    map_ppu_read_pages(mapper.rom->chr_ram, sp * 8, dp * 8, 8);
    map_ppu_write_pages(mapper.rom->chr_ram, sp * 8, dp * 8, 8);
  }
}

static void map_ppu_nt_page(uint page, u8* src)
{
  mapper.ppu_read_page[page] = src;
  mapper.ppu_write_page[page] = src;
  mapper.ppu_read_page[page + 4] = src;
  mapper.ppu_write_page[page + 4] = src;
}

static void map_ppu_nt_single_low()
{
  map_ppu_nt_page(0x8, map.vram);
  map_ppu_nt_page(0x9, map.vram);
  map_ppu_nt_page(0xA, map.vram);
  map_ppu_nt_page(0xB, map.vram);
}

static void map_ppu_nt_single_high()
{
  map_ppu_nt_page(0x8, map.vram + MAP_PPU_PAGE_SIZE);
  map_ppu_nt_page(0x9, map.vram + MAP_PPU_PAGE_SIZE);
  map_ppu_nt_page(0xA, map.vram + MAP_PPU_PAGE_SIZE);
  map_ppu_nt_page(0xB, map.vram + MAP_PPU_PAGE_SIZE);
}

static void map_ppu_nt_vert_mirror()
{
  map_ppu_nt_page(0x8, map.vram);
  map_ppu_nt_page(0x9, map.vram + MAP_PPU_PAGE_SIZE);
  map_ppu_nt_page(0xA, map.vram);
  map_ppu_nt_page(0xB, map.vram + MAP_PPU_PAGE_SIZE);
}

static void map_ppu_nt_horiz_mirror()
{
  map_ppu_nt_page(0x8, map.vram);
  map_ppu_nt_page(0x9, map.vram);
  map_ppu_nt_page(0xA, map.vram + MAP_PPU_PAGE_SIZE);
  map_ppu_nt_page(0xB, map.vram + MAP_PPU_PAGE_SIZE);
}

static void map_ppu_nt_four_screen()
{
  map_ppu_nt_page(0x8, map.vram + MAP_PPU_PAGE_SIZE * 0);
  map_ppu_nt_page(0x9, map.vram + MAP_PPU_PAGE_SIZE * 1);
  map_ppu_nt_page(0xA, map.vram + MAP_PPU_PAGE_SIZE * 2);
  map_ppu_nt_page(0xB, map.vram + MAP_PPU_PAGE_SIZE * 3);
}

static void map_nrom_load()
{
  map_prg_rom_page_16k(0, 2);
  map_prg_rom_page_16k(1, 3);

  map_prg_ram_page_8k(0, 3);

  // map_cpu_read_pages(mapper.eram, 0, 4, 2);
  // map_cpu_write_pages(mapper.eram, 0, 4, 2);

  for (uint i = 0; i < 8; ++i) { // rom may have < 8k
    map_chr_page_1k(i, i);
  }

  if (mapper.rom->alt_mirror) {
    map_ppu_nt_four_screen();
  } else if (mapper.rom->vert_mirror) {
    map_ppu_nt_vert_mirror();
  } else {
    map_ppu_nt_horiz_mirror();
  }
}

typedef struct
{
  u8 ctrl;
  u8 chr0;
  u8 chr1;
  u8 prg;
  u8 shift;
  u8 delay;
} MapMMC1;

static void map_mmc1_update()
{
  MapMMC1* reg = (MapMMC1*)map.eram;
  u8 ctrl_nt = reg->ctrl & 3;
  u8 ctrl_prg = (reg->ctrl >> 2) & 3;
  u8 ctrl_chr = reg->ctrl >> 4;
  u8 chr_page0 = reg->chr0;
  u8 chr_page1 = ctrl_chr ? reg->chr1 : reg->chr0;
  u8 prg_sup0 = chr_page0 & 0x10;
  u8 prg_sup1 = chr_page1 & 0x10;
  u8 prg_page = reg->prg & 0x0F;
  u8 prg_last = -1 & 0x0F;

  if (!mapper.rom->alt_mirror) {
    switch (ctrl_nt) {
      case 0: map_ppu_nt_single_low(); break;
      case 1: map_ppu_nt_single_high(); break;
      case 2: map_ppu_nt_vert_mirror(); break;
      case 3: map_ppu_nt_horiz_mirror(); break;
    }
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
  MapMMC1* reg = (MapMMC1*)map.eram;
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
      map.eram[idx] = reg->shift & 0x1F;
      reg->shift = 0x10;
      map_mmc1_update();
    }
  }
  return true;
}

static void map_mmc1_cpu_cyc()
{
  MapMMC1* reg = (MapMMC1*)map.eram;
  if (reg->delay > 0) {
    --reg->delay;
  }
}

static void map_mmc1_load(bool init)
{
  MapMMC1* reg = (MapMMC1*)map.eram;
  mapper.cpu_write = map_mmc1_cpu_write;
  mapper.cpu_cyc = map_mmc1_cpu_cyc;
  if (init) {
    reg->ctrl = 0x0F;
    reg->chr0 = 0x00;
    reg->chr1 = 0x01;
    reg->prg = 0x00;
    reg->shift = 0x10;
  }
  map_mmc1_update();
}

static bool map_uxrom_cpu_write(u16 addr, u8 val)
{
  if (addr >= 0x8000) {
    map_prg_rom_page_16k(val, 2);
    return true;
  }
  return false;
}

static void map_uxrom_load()
{
  mapper.cpu_write = map_uxrom_cpu_write;
  map_prg_rom_page_16k(-1, 3);
}

static bool map_cxrom_cpu_write(u16 addr, u8 val)
{
  if (addr >= 0x8000) {
    map_chr_page_8k(val, 0);
    return true;
  }
  return false;
}

static void map_cxrom_load() { mapper.cpu_write = map_cxrom_cpu_write; }

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
  u8 alt_irq;
} MapMMC3;

static void map_mmc3_update()
{
  MapMMC3* reg = (MapMMC3*)map.eram;
  if (reg->ctrl & 0x80) {
    map_chr_page_1k(reg->chr2, 0);
    map_chr_page_1k(reg->chr3, 1);
    map_chr_page_1k(reg->chr4, 2);
    map_chr_page_1k(reg->chr5, 3);
    map_chr_page_2k(reg->chr0 >> 1, 2);
    map_chr_page_2k(reg->chr1 >> 1, 3);
  } else {
    map_chr_page_2k(reg->chr0 >> 1, 0);
    map_chr_page_2k(reg->chr1 >> 1, 1);
    map_chr_page_1k(reg->chr2, 4);
    map_chr_page_1k(reg->chr3, 5);
    map_chr_page_1k(reg->chr4, 6);
    map_chr_page_1k(reg->chr5, 7);
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
  MapMMC3* reg = (MapMMC3*)map.eram;
  switch (addr & 0xE001) {
    case 0x8000:
      reg->ctrl = val;
      map_mmc3_update();
      return true;
    case 0x8001:
      map.eram[reg->ctrl & 7] = val;
      map_mmc3_update();
      return true;
    case 0xA000:
      if (!mapper.rom->alt_mirror) {
        if (val & 1) {
          map_ppu_nt_horiz_mirror();
        } else {
          map_ppu_nt_vert_mirror();
        }
      }
      return true;
    case 0xA001: /* sram protect not needed*/ return true;
    case 0xC000: reg->latch = val; return true;
    case 0xC001: reg->reload = 1; return true;
    case 0xE000:
      reg->enabled = 0;
      map.irq = false;
      map_cpu_irq();
      return true;
    case 0xE001: reg->enabled = 1; return true;
  }
  return false;
}

static void map_mmc3_ppu_addr(u16 addr)
{
  MapMMC3* reg = (MapMMC3*)map.eram;
  if (addr & 0x1000) {
    if (reg->filter == 0) {
      if (reg->counter == 0 || reg->reload) {
        reg->counter = reg->latch;
      } else {
        --reg->counter;
      }
      if (reg->counter == 0 && reg->enabled && (reg->reload || !reg->alt_irq || reg->latch)) {
        map.irq = true;
        map_cpu_irq();
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
  MapMMC3* reg = (MapMMC3*)map.eram;
  if (reg->filter > 0 && reg->filter < 5) {
    --reg->filter;
  }
}

static void map_mmc3_load()
{
  mapper.cpu_cyc = map_mmc3_cpu_cyc;
  mapper.cpu_write = map_mmc3_cpu_write;
  mapper.ppu_addr = map_mmc3_ppu_addr;
  map_mmc3_update();
}

static void map_mmc3a_load()
{
  map_mmc3_load();
  MapMMC3* reg = (MapMMC3*)map.eram;
  reg->alt_irq = true;
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
  map_mmc5_prg_8k(mapper.eram[MAP_MMC5_PRG_BASE + 0], 3);

  switch (mapper.eram[MAP_MMC5_PRG_MODE]) {
    case 0: map_prg_page_32k(mapper.eram[MAP_MMC5_PRG_BASE + 4] >> 2, 2); break;
    case 1:
      map_mmc5_prg_16k(mapper.eram[MAP_MMC5_PRG_BASE + 2], 2);
      map_prg_page_16k(mapper.eram[MAP_MMC5_PRG_BASE + 4] >> 1, 3);
      break;
    case 2:
      map_mmc5_prg_16k(mapper.eram[MAP_MMC5_PRG_BASE + 2], 2);
      map_mmc5_prg_8k(mapper.eram[MAP_MMC5_PRG_BASE + 3], 6);
      map_prg_page_8k(mapper.eram[MAP_MMC5_PRG_BASE + 4], 7);
      break;
    case 3:
      map_mmc5_prg_8k(mapper.eram[MAP_MMC5_PRG_BASE + 1], 4);
      map_mmc5_prg_8k(mapper.eram[MAP_MMC5_PRG_BASE + 2], 5);
      map_mmc5_prg_8k(mapper.eram[MAP_MMC5_PRG_BASE + 3], 6);
      map_prg_page_8k(mapper.eram[MAP_MMC5_PRG_BASE + 4], 7);
      break;
  }
}

static void map_mmc5_update_chr()
{
  u16 chr_hi = mapper.eram[MAP_MMC5_CHR_HIGH] << 8;
  switch (mapper.eram[MAP_MMC5_CHR_MODE]) {
    case 0: map_chr_page_8k(mapper.eram[MAP_MMC5_CHR_BASE + 7] | chr_hi, 0); break;
    case 1:
      map_chr_page_4k(mapper.eram[MAP_MMC5_CHR_BASE + 3] | chr_hi, 0);
      map_chr_page_4k(mapper.eram[MAP_MMC5_CHR_BASE + 7] | chr_hi, 1);
      break;
    case 2:
      map_chr_page_2k(mapper.eram[MAP_MMC5_CHR_BASE + 1] | chr_hi, 0);
      map_chr_page_2k(mapper.eram[MAP_MMC5_CHR_BASE + 3] | chr_hi, 1);
      map_chr_page_2k(mapper.eram[MAP_MMC5_CHR_BASE + 5] | chr_hi, 2);
      map_chr_page_2k(mapper.eram[MAP_MMC5_CHR_BASE + 7] | chr_hi, 3);
      break;
    case 3:
      for (int i = 0; i < 8; i++) {
        map_chr_page_1k(mapper.eram[MAP_MMC5_CHR_BASE + i] | chr_hi, i);
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
    mapper.eram[addr] = val;
    map_mmc5_update_prg();
    return true;
  }
  if (addr == MAP_MMC5_CHR_MODE || addr == MAP_MMC5_CHR_HIGH
      || (addr >= MAP_MMC5_CHR_BASE && addr <= MAP_MMC5_CHR_BASE + 7))
  {
    mapper.eram[addr] = val;
    map_mmc5_update_chr();
    return true;
  }
  if (addr == MAP_MMC5_IRQ_LATCH || addr == MAP_MMC5_IRQ_ENABLED) {
    mapper.eram[addr] = val;
    return true;
  }
  if (addr == MAP_MMC5_NT_MODE) {
    u8 nt0 = addr & 3;
    u8 nt1 = (addr >> 2) & 3;
    u8 nt2 = (addr >> 4) & 3;
    u8 nt3 = addr >> 6;


  map_ppu_nt_page(0x8, mapper.vram + MAP_PPU_PAGE_SIZE * 0);
  map_ppu_nt_page(0x9, mapper.vram + MAP_PPU_PAGE_SIZE * 1);
  map_ppu_nt_page(0xA, mapper.vram + MAP_PPU_PAGE_SIZE * 2);
  map_ppu_nt_page(0xB, mapper.vram + MAP_PPU_PAGE_SIZE * 3);
  }
  // if (addr >= MAP_MMC5_ERAM && mapper.eram[MAP_MMC5_EXT_MODE] > 2) { // eram write protect
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
    *val = mapper.eram[MAP_MMC5_IRQ_STATUS];
    mapper.eram[MAP_MMC5_IRQ_STATUS] &= ~0x80;
    return true;
  }
  if (addr == MAP_MMC5_MUL_A || addr == MAP_MMC5_MUL_B) {
    u16 result = (u16)mapper.eram[MAP_MMC5_MUL_A] * (u16)mapper.eram[MAP_MMC5_MUL_B];
    *val = (addr == MAP_MMC5_MUL_A) ? (result & 0xFF) : (result >> 8);
    return true;
  }
  // if (addr >= MAP_MMC5_ERAM && mapper.eram[MAP_MMC5_EXT_MODE] < 2) { // eram read protect
  //   return true;
  // }
  return false;
}

static bool map_mmc5_ppu_read(u16 addr, u8*)
{
  if (addr < 0x2000) {
    u8 a12 = (addr >> 12) & 1;
    if (a12 && !mapper.eram[MAP_MMC5_IRQ_A12]) {
      // Будущий счетчик строк
    }
    mapper.eram[MAP_MMC5_IRQ_A12] = a12;
  }
  return false;
}

void map_mmc5_load()
{
  mapper.cpu_write = map_mmc5_cpu_write;
  mapper.cpu_read = map_mmc5_cpu_read;
  mapper.ppu_read = map_mmc5_ppu_read;

  mapper.eram[MAP_MMC5_PRG_MODE] = 3;
  mapper.eram[MAP_MMC5_CHR_MODE] = 3;
  mapper.eram[MAP_MMC5_PRG_RAM_PROTECT1] = 3;
  mapper.eram[MAP_MMC5_PRG_RAM_PROTECT2] = 3;
  mapper.eram[MAP_MMC5_EXT_MODE] = 3;

  map_mmc5_update_prg();
  map_mmc5_update_chr();
}
*/

static bool map_axrom_cpu_write(u16 addr, u8 val)
{
  if (addr >= 0x8000) {
    map_prg_rom_page_32k(val & 0x0F, 1);
    if (val & 0x10) {
      map_ppu_nt_single_high();
    } else {
      map_ppu_nt_single_low();
    }
    return true;
  }
  return false;
}

static void map_axrom_load()
{
  mapper.cpu_write = map_axrom_cpu_write;
  map_ppu_nt_single_low();
}

static bool map_colordreams_cpu_write(u16 addr, u8 val)
{
  if (addr >= 0x8000) {
    map_prg_rom_page_32k(val & 0x0F, 1);
    map_chr_page_8k(val >> 4, 0);
    return true;
  }
  return false;
}

static void map_colordreams_load() { mapper.cpu_write = map_colordreams_cpu_write; }

static bool map_bnrom_cpu_write(u16 addr, u8 val)
{
  if (addr < 0x7FFD) {
    return false;
  }
  switch (addr) {
    default:
    case 0x7FFD: map_prg_rom_page_32k(val, 1); break;
    case 0x7FFE: map_chr_page_4k(val, 0); break;
    case 0x7FFF: map_chr_page_4k(val, 1); break;
  }
  return true;
}

static void map_bnrom_load() { mapper.cpu_write = map_bnrom_cpu_write; }

static bool map_gxrom_cpu_write(u16 addr, u8 val)
{
  if (addr >= 0x8000) {
    map_prg_rom_page_32k(val >> 4, 1);
    map_chr_page_8k(val & 0xF, 0);
    return true;
  }
  return false;
}

static void map_gxrom_load() { mapper.cpu_write = map_gxrom_cpu_write; }

bool map_rom_load(Rom rom, bool init)
{
  mapper.rom = rom;
  rom->mapper_error = false;
  map_nrom_load();
  switch (rom->mapper) {
    case 0: break;
    case 1: map_mmc1_load(init); break;
    case 2: map_uxrom_load(); break;
    case 3: map_cxrom_load(); break;
    case 4: map_mmc3_load(); break;
    // case 5: map_mmc5_load(); break;
    case 7: map_axrom_load(); break;
    case 11: map_colordreams_load(); break;
    case 12: map_mmc3a_load(); break;
    case 34: map_bnrom_load(); break;
    case 66: map_gxrom_load(); break;
    default: rom->mapper_error = true; return false;
  }
  return true;
}

bool mn_rom_set(Rom rom)
{
  memset(&mapper, 0, sizeof(mapper));
  memset(&map, 0, sizeof(map));
  if (rom) {
    if (!map_rom_load(rom, true)) {
      return false;
    }
    dev_power();
  }
  return true;
}
