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

  u8 reg0;
  u8 reg1;
  u8 reg2;
  u8 reg3;
  u8 reg4;
  u8 reg5;
  u8 reg6;
  u8 reg7;

  void (*load)();
  bool (*cpu_read)(u16, u8*);
  bool (*cpu_write)(u16, u8);
  bool (*ppu_read)(u16, u8*);
  bool (*ppu_write)(u16, u8);

  Rom rom;
} mapper;

u8 map_openbus() { return mapper.openbus; }

static const u8* map_cpu_read_pages(const u8* src, int dst_from, int pages)
{
  const u8* next = 0;
  for (int i = 0; i < pages; ++i) {
    next = src + MAP_CPU_PAGE_SIZE * i;
    mapper.cpu_read_page[dst_from + i] = next;
  }
  return next;
}

static u8* map_cpu_write_pages(u8* src, int dst_from, int pages)
{
  u8* next = 0;
  for (int i = 0; i < pages; ++i) {
    next = src + MAP_CPU_PAGE_SIZE * i;
    mapper.cpu_write_page[dst_from + i] = next;
  }
  return next;
}

static const u8* map_ppu_read_pages(const u8* src, int dst_from, int pages)
{
  const u8* next = 0;
  for (int i = 0; i < pages; ++i) {
    next = src + MAP_PPU_PAGE_SIZE * i;
    mapper.ppu_read_page[dst_from + i] = next;
  }
  return next;
}

static u8* map_ppu_write_pages(u8* src, int dst_from, int pages)
{
  u8* next = 0;
  for (int i = 0; i < pages; ++i) {
    next = src + MAP_PPU_PAGE_SIZE * i;
    mapper.ppu_write_page[dst_from + i] = next;
  }
  return next;
}

static void map_ppu_nt_page(int page, u8* src)
{
  mapper.ppu_read_page[page] = src;
  mapper.ppu_write_page[page] = src;
  mapper.ppu_read_page[page + 4] = src;
  mapper.ppu_write_page[page + 4] = src;
}

static void map_ppu_nt_low()
{
  map_ppu_nt_page(0x8, mapper.vram);
  map_ppu_nt_page(0x9, mapper.vram);
  map_ppu_nt_page(0xA, mapper.vram);
  map_ppu_nt_page(0xB, mapper.vram);
}

static void map_ppu_nt_high()
{
  map_ppu_nt_page(0x8, mapper.vram + MAP_PPU_PAGE_SIZE);
  map_ppu_nt_page(0x9, mapper.vram + MAP_PPU_PAGE_SIZE);
  map_ppu_nt_page(0xA, mapper.vram + MAP_PPU_PAGE_SIZE);
  map_ppu_nt_page(0xB, mapper.vram + MAP_PPU_PAGE_SIZE);
}

static void map_ppu_nt_horiz()
{
  map_ppu_nt_page(0x8, mapper.vram);
  map_ppu_nt_page(0x9, mapper.vram);
  map_ppu_nt_page(0xA, mapper.vram + MAP_PPU_PAGE_SIZE);
  map_ppu_nt_page(0xB, mapper.vram + MAP_PPU_PAGE_SIZE);
}

static void map_ppu_nt_vert()
{
  map_ppu_nt_page(0x8, mapper.vram);
  map_ppu_nt_page(0x9, mapper.vram + MAP_PPU_PAGE_SIZE);
  map_ppu_nt_page(0xA, mapper.vram);
  map_ppu_nt_page(0xB, mapper.vram + MAP_PPU_PAGE_SIZE);
}

static void map_ppu_nt_four()
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
    u8* page = mapper.ppu_write_page[addr >> MAP_CPU_PAGE_SHIFT];
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
  const u8* prg_next = map_cpu_read_pages(mapper.rom->prg, 8, 4);
  map_cpu_read_pages(mapper.rom->prg_pages == 1 ? mapper.rom->prg : prg_next, 12, 4);

  map_cpu_read_pages(mapper.eram, 4, 2);
  map_cpu_write_pages(mapper.eram, 4, 2);

  map_cpu_read_pages(mapper.sram, 6, 2);
  map_cpu_write_pages(mapper.sram, 6, 2);

  map_ppu_read_pages(mapper.rom->chr ? mapper.rom->chr : mapper.wram, 0, 8);
  map_ppu_write_pages(mapper.wram, 0, 8);

  if (mapper.rom->alt_mirror) {
    map_ppu_nt_four();
  } else if (mapper.rom->vert_mirror) {
    map_ppu_nt_vert();
  } else {
    map_ppu_nt_horiz();
  }
}

static void map_mmc1_update()
{
  switch (mapper.reg0 & 0x03) {
    default:
    case 0: map_ppu_nt_low(); break;
    case 1: map_ppu_nt_high(); break;
    case 2: map_ppu_nt_vert(); break;
    case 3: map_ppu_nt_horiz(); break;
  }
}

static bool map_mmc1_cpu_write(u16 addr, u8 val)
{
  if (val & 0x80) {
    mapper.reg4 = 0x10;
    mapper.reg5 = 0;
    mapper.reg0 |= 0x0C;
    return false;
  }

  bool bit = (val & 1);
  mapper.reg4 = (mapper.reg4 >> 1) | (bit << 4);
  mapper.reg5++;

  if (mapper.reg5 == 5) {
    if (addr <= 0x9FFF) {
      mapper.reg0 = mapper.reg4;
    } else if (addr <= 0xBFFF) {
      mapper.reg1 = mapper.reg4;
    } else if (addr <= 0xDFFF) {
      mapper.reg2 = mapper.reg4;
    } else {
      mapper.reg3 = mapper.reg4;
    }

    mapper.reg4 = 0;
    mapper.reg5 = 0;

    map_mmc1_update();
  }

  return false;
}

static void map_mmc1_load()
{
  map_nrom_load();
  mapper.cpu_write = map_mmc1_cpu_write;
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
