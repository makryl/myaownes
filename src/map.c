#include "map.h"
#include "rom.h"
#include <string.h>

static const size_t MAP_CPU_PAGE_SHIFT = 12; // 4kb
static const size_t MAP_CPU_PAGE_SIZE = 1 << MAP_CPU_PAGE_SHIFT;
static const size_t MAP_CPU_PAGE_MASK = MAP_CPU_PAGE_SIZE - 1;
static const size_t MAP_PPU_PAGE_SHIFT = 10; // 1kb
static const size_t MAP_PPU_PAGE_SIZE = 1 << MAP_PPU_PAGE_SHIFT;
static const size_t MAP_PPU_PAGE_MASK = MAP_PPU_PAGE_SIZE - 1;

static struct
{
  u8 vram[0x1000];
  u8 wram[0x2000];
  u8 eram[0x2000];
  u8 sram[0x2000];

  const u8* cpu_read_page[16];
  u8* cpu_write_page[16];
  const u8* ppu_read_page[16];
  u8* ppu_write_page[16];

  u8 openbus;

  u8 reg[8];

  bool (*cpu_read)(u16, u8*);
  bool (*cpu_write)(u16, u8);
  bool (*ppu_read)(u16, u8*);
  bool (*ppu_write)(u16, u8);

  Rom rom;
} mapper;

u8 map_openbus() { return mapper.openbus; }

static const u8* map_cpu_read_pages(const u8* src, u8 src_page, u8 dst_page, u8 pages)
{
  const u8* next = 0;
  for (int i = 0; i < pages; ++i) {
    next = src + MAP_CPU_PAGE_SIZE * (src_page + i);
    mapper.cpu_read_page[dst_page + i] = next;
  }
  return next;
}

static u8* map_cpu_write_pages(u8* src, u8 src_page, u8 dst_page, u8 pages)
{
  u8* next = 0;
  for (int i = 0; i < pages; ++i) {
    next = src + MAP_CPU_PAGE_SIZE * (src_page + i);
    mapper.cpu_write_page[dst_page + i] = next;
  }
  return next;
}

static const u8* map_ppu_read_pages(const u8* src, u8 src_page, u8 dst_page, u8 pages)
{
  const u8* next = 0;
  for (int i = 0; i < pages; ++i) {
    next = src + MAP_PPU_PAGE_SIZE * (src_page + i);
    mapper.ppu_read_page[dst_page + i] = next;
  }
  return next;
}

static u8* map_ppu_write_pages(u8* src, u8 src_page, u8 dst_page, u8 pages)
{
  u8* next = 0;
  for (int i = 0; i < pages; ++i) {
    next = src + MAP_PPU_PAGE_SIZE * (src_page + i);
    mapper.ppu_write_page[dst_page + i] = next;
  }
  return next;
}

static void map_cpu_read_page_4k(const u8* src, u8 sp, u8 dp) { map_cpu_read_pages(src, sp, dp, 1); }
static void map_cpu_read_page_8k(const u8* src, u8 sp, u8 dp) { map_cpu_read_pages(src, sp * 2, dp * 2, 2); }
static void map_cpu_read_page_16k(const u8* src, u8 sp, u8 dp) { map_cpu_read_pages(src, sp * 4, dp * 4, 4); }
static void map_cpu_read_page_32k(const u8* src, u8 sp, u8 dp) { map_cpu_read_pages(src, sp * 8, dp * 8, 8); }
static void map_cpu_write_page_4k(u8* src, u8 sp, u8 dp) { map_cpu_write_pages(src, sp, dp, 1); }
static void map_cpu_write_page_8k(u8* src, u8 sp, u8 dp) { map_cpu_write_pages(src, sp * 2, dp * 2, 2); }
static void map_cpu_write_page_16k(u8* src, u8 sp, u8 dp) { map_cpu_write_pages(src, sp * 4, dp * 4, 4); }
static void map_cpu_write_page_32k(u8* src, u8 sp, u8 dp) { map_cpu_write_pages(src, sp * 8, dp * 8, 8); }

static void map_ppu_read_page_1k(const u8* src, u8 sp, u8 dp) { map_ppu_read_pages(src, sp, dp, 1); }
static void map_ppu_read_page_2k(const u8* src, u8 sp, u8 dp) { map_ppu_read_pages(src, sp * 2, dp * 2, 2); }
static void map_ppu_read_page_4k(const u8* src, u8 sp, u8 dp) { map_ppu_read_pages(src, sp * 4, dp * 4, 4); }
static void map_ppu_read_page_8k(const u8* src, u8 sp, u8 dp) { map_ppu_read_pages(src, sp * 8, dp * 8, 8); }
static void map_ppu_write_page_1k(u8* src, u8 sp, u8 dp) { map_ppu_write_pages(src, sp, dp, 1); }
static void map_ppu_write_page_2k(u8* src, u8 sp, u8 dp) { map_ppu_write_pages(src, sp * 2, dp * 2, 2); }
static void map_ppu_write_page_4k(u8* src, u8 sp, u8 dp) { map_ppu_write_pages(src, sp * 4, dp * 4, 4); }
static void map_ppu_write_page_8k(u8* src, u8 sp, u8 dp) { map_ppu_write_pages(src, sp * 8, dp * 8, 8); }

static void map_ppu_nt_page(int page, u8* src)
{
  mapper.ppu_read_page[page] = src;
  mapper.ppu_write_page[page] = src;
  mapper.ppu_read_page[page + 4] = src;
  mapper.ppu_write_page[page + 4] = src;
}

static void map_ppu_nt_single_low()
{
  map_ppu_nt_page(0x8, mapper.vram);
  map_ppu_nt_page(0x9, mapper.vram);
  map_ppu_nt_page(0xA, mapper.vram);
  map_ppu_nt_page(0xB, mapper.vram);
}

static void map_ppu_nt_single_high()
{
  map_ppu_nt_page(0x8, mapper.vram + MAP_PPU_PAGE_SIZE);
  map_ppu_nt_page(0x9, mapper.vram + MAP_PPU_PAGE_SIZE);
  map_ppu_nt_page(0xA, mapper.vram + MAP_PPU_PAGE_SIZE);
  map_ppu_nt_page(0xB, mapper.vram + MAP_PPU_PAGE_SIZE);
}

static void map_ppu_nt_vert_mirror()
{
  map_ppu_nt_page(0x8, mapper.vram);
  map_ppu_nt_page(0x9, mapper.vram + MAP_PPU_PAGE_SIZE);
  map_ppu_nt_page(0xA, mapper.vram);
  map_ppu_nt_page(0xB, mapper.vram + MAP_PPU_PAGE_SIZE);
}

static void map_ppu_nt_horiz_mirror()
{
  map_ppu_nt_page(0x8, mapper.vram);
  map_ppu_nt_page(0x9, mapper.vram);
  map_ppu_nt_page(0xA, mapper.vram + MAP_PPU_PAGE_SIZE);
  map_ppu_nt_page(0xB, mapper.vram + MAP_PPU_PAGE_SIZE);
}

static void map_ppu_nt_four_screen()
{
  map_ppu_nt_page(0x8, mapper.vram + MAP_PPU_PAGE_SIZE * 0);
  map_ppu_nt_page(0x9, mapper.vram + MAP_PPU_PAGE_SIZE * 1);
  map_ppu_nt_page(0xA, mapper.vram + MAP_PPU_PAGE_SIZE * 2);
  map_ppu_nt_page(0xB, mapper.vram + MAP_PPU_PAGE_SIZE * 3);
}

u8 map_cpu_read(u16 addr)
{
  if (!mapper.cpu_read || !mapper.cpu_read(addr, &mapper.openbus)) {
    const u8* page = mapper.cpu_read_page[addr >> MAP_CPU_PAGE_SHIFT];
    if (page) {
      mapper.openbus = page[addr & MAP_CPU_PAGE_MASK];
    }
  }
  return mapper.openbus;
}

void map_cpu_write(u16 addr, u8 val)
{
  mapper.openbus = val;
  if (!mapper.cpu_write || !mapper.cpu_write(addr, val)) {
    u8* page = mapper.cpu_write_page[addr >> MAP_CPU_PAGE_SHIFT];
    if (page) {
      page[addr & MAP_CPU_PAGE_MASK] = val;
    }
  }
}

u8 map_ppu_read(u16 addr)
{
  if (!mapper.ppu_read || !mapper.ppu_read(addr, &mapper.openbus)) {
    const u8* page = mapper.ppu_read_page[addr >> MAP_PPU_PAGE_SHIFT];
    if (page) {
      mapper.openbus = page[addr & MAP_PPU_PAGE_MASK];
    }
  }
  return mapper.openbus;
}

void map_ppu_write(u16 addr, u8 val)
{
  mapper.openbus = val;
  if (!mapper.ppu_write || !mapper.ppu_write(addr, val)) {
    u8* page = mapper.ppu_write_page[addr >> MAP_PPU_PAGE_SHIFT];
    if (page) {
      page[addr & MAP_PPU_PAGE_MASK] = val;
    }
  }
}

static void map_nrom_load()
{
  map_cpu_read_page_16k(mapper.rom->prg, 0, 2);
  map_cpu_read_page_16k(mapper.rom->prg, mapper.rom->prg_pages == 1 ? 0 : 1, 3);

  map_cpu_read_page_8k(mapper.eram, 0, 2);
  map_cpu_write_page_8k(mapper.eram, 0, 2);

  map_cpu_read_page_8k(mapper.sram, 0, 3);
  map_cpu_write_page_8k(mapper.sram, 0, 3);

  map_ppu_read_page_8k(mapper.rom->chr ? mapper.rom->chr : mapper.wram, 0, 0);
  map_ppu_write_page_8k(mapper.wram, 0, 0);

  if (mapper.rom->alt_mirror) {
    map_ppu_nt_four_screen();
  } else if (mapper.rom->vert_mirror) {
    map_ppu_nt_vert_mirror();
  } else {
    map_ppu_nt_horiz_mirror();
  }
}

static void map_mmc1_update()
{
  u8 ctrl_nt = mapper.reg[0] & 3;
  u8 ctrl_prg = (mapper.reg[0] >> 2) & 3;
  u8 ctrl_chr = mapper.reg[0] >> 4;
  u8 chr_page0 = mapper.reg[1];
  u8 chr_page1 = mapper.reg[2];
  u8 prg_sup0 = mapper.reg[1] & 0x10;
  u8 prg_sup1 = mapper.reg[2] & 0x10;
  u8 prg_page = mapper.reg[3] & 0x0F;
  u8 prg_mask = mapper.rom->prg_pages - 1; // check pow-of-2 size?

  if (!mapper.rom->alt_mirror) {
    switch (ctrl_nt) {
      case 0: map_ppu_nt_single_low(); break;
      case 1: map_ppu_nt_single_high(); break;
      case 2: map_ppu_nt_vert_mirror(); break;
      case 3: map_ppu_nt_horiz_mirror(); break;
    }
  }

  if (mapper.rom->chr) {
    if (ctrl_chr) {
      map_ppu_read_page_4k(mapper.rom->chr, chr_page0, 0);
      map_ppu_read_page_4k(mapper.rom->chr, chr_page1, 1);
    } else {
      map_ppu_read_page_8k(mapper.rom->chr, chr_page0, 0);
    }
  }

  switch (ctrl_prg) {
    case 0:
    case 1: {
      map_cpu_read_page_32k(mapper.rom->prg, ((prg_sup0 | prg_page) & prg_mask) >> 1, 1);
      break;
    }
    case 2:
      map_cpu_read_page_16k(mapper.rom->prg, prg_sup0 & prg_mask, 2);
      map_cpu_read_page_16k(mapper.rom->prg, (prg_sup1 | prg_page) & prg_mask, 3);
      break;
    case 3:
      map_cpu_read_page_16k(mapper.rom->prg, (prg_sup0 | prg_page) & prg_mask, 2);
      map_cpu_read_page_16k(mapper.rom->prg, (prg_sup1 | (prg_mask & 0xF)) & prg_mask, 3);
      break;
  }
}

static bool map_mmc1_cpu_write(u16 addr, u8 val)
{
  // todo: mmc1 is slower than cpu, ignore sequential write on next cpu cycle
  if (val & 0x80) {
    mapper.reg[0] |= 0x0C;
    mapper.reg[4] = 0x10;
    map_mmc1_update();
  } else {
    bool last = mapper.reg[4] & 1;
    mapper.reg[4] >>= 1;
    mapper.reg[4] |= (val & 1) << 4;
    if (last) {
      u8 id = (addr >> 13) & 3;
      mapper.reg[id] = mapper.reg[4] & 0x1F;
      mapper.reg[4] = 0x10;
      map_mmc1_update();
    }
  }
  return false;
}

static void map_mmc1_load()
{
  map_nrom_load();
  mapper.cpu_write = map_mmc1_cpu_write;
  mapper.reg[0] = 0x0F;
  mapper.reg[1] = 0x00;
  mapper.reg[2] = 0x01;
  mapper.reg[3] = 0x00;
  mapper.reg[4] = 0x10;
  map_mmc1_update();
}

static void map_unrom_load() {}
static void map_cnrom_load() {}
static void map_mmc3_load() {}
static void map_mmc5_load() {}
static void map_axrom_load() {}

void mn_rom_load(Rom rom)
{
  memset(&mapper, 0, sizeof(mapper));
  mapper.rom = rom;
  switch (rom->mapper) {
    default:
    case 0: map_nrom_load(); break;
    case 1: map_mmc1_load(); break;
    case 2: map_unrom_load(); break;
    case 3: map_cnrom_load(); break;
    case 4: map_mmc3_load(); break;
    case 5: map_mmc5_load(); break;
    case 7: map_axrom_load(); break;
  }
}
