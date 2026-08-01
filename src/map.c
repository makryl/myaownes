#include "map.h"
#include "cpu.h"
#include "dev.h"
#include "common.h"
#include "map_internal.h"
#include "map_discrete.h"
#include "map_mmc.h"
#include "map_other.h"
#include <string.h>

enum : u64
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

  mn_rom rom;
} map_dyn;

MN_CACHE_LINE static struct
{
  bool apu_irq;
  bool irq;
  MN_CACHE_LINE u8 vram[0x1000];
  MN_CACHE_LINE u8 reg[MAP_REG_SIZE];
} map;

void map_set_cpu_cyc_cb(map_cpu_cyc_cb cb) { map_dyn.cpu_cyc = cb; }
void map_set_cpu_read_cb(map_cpu_read_cb cb) { map_dyn.cpu_read = cb; }
void map_set_cpu_write_cb(map_cpu_write_cb cb) { map_dyn.cpu_write = cb; }
void map_set_ppu_addr_cb(map_ppu_addr_cb cb) { map_dyn.ppu_addr = cb; }
void map_set_ppu_read_cb(map_ppu_read_cb cb) { map_dyn.ppu_read = cb; }
void map_set_ppu_write_cb(map_ppu_write_cb cb) { map_dyn.ppu_write = cb; }

u8* map_reg() { return map.reg; }
mn_rom mn_rom_get() { return map_dyn.rom; }
uint map_size() { return sizeof(map); }
void* map_data() { return &map; }
bool map_ready() { return map_dyn.rom && !map_dyn.rom->mapper_error; }
void map_cpu_irq() { cpu_irq(map.apu_irq || map.irq); }

void map_irq(bool enabled)
{
  map.irq = enabled;
  map_cpu_irq();
}

void map_apu_irq(bool enabled)
{
  map.apu_irq = enabled;
  map_cpu_irq();
}

void map_cpu_cyc()
{
  if (map_dyn.cpu_cyc) {
    map_dyn.cpu_cyc();
  }
}

void map_cpu_read(u16 addr, u8* val, bool trace)
{
  if (!map_dyn.cpu_read || !map_dyn.cpu_read(addr, val, trace)) {
    const u8* page = map_dyn.cpu_read_page[addr >> MAP_CPU_PAGE_SHIFT];
    if (page) {
      *val = page[addr & MAP_CPU_PAGE_MASK];
    }
  }
}

void map_cpu_write(u16 addr, u8 val)
{
  if (!map_dyn.cpu_write || !map_dyn.cpu_write(addr, val)) {
    u8* page = map_dyn.cpu_write_page[addr >> MAP_CPU_PAGE_SHIFT];
    if (page) {
      page[addr & MAP_CPU_PAGE_MASK] = val;
    }
  }
}

void map_ppu_addr(u16 addr)
{
  if (map_dyn.ppu_addr) {
    map_dyn.ppu_addr(addr);
  }
}

void map_ppu_read(u16 addr, u8* val)
{
  if (!map_dyn.ppu_read || !map_dyn.ppu_read(addr, val)) {
    const u8* page = map_dyn.ppu_read_page[addr >> MAP_PPU_PAGE_SHIFT];
    if (page) {
      *val = page[addr & MAP_PPU_PAGE_MASK];
    }
  }
}

void map_ppu_write(u16 addr, u8 val)
{
  if (!map_dyn.ppu_write || !map_dyn.ppu_write(addr, val)) {
    u8* page = map_dyn.ppu_write_page[addr >> MAP_PPU_PAGE_SHIFT];
    if (page) {
      page[addr & MAP_PPU_PAGE_MASK] = val;
    }
  }
}

void map_clear_pages()
{
  memset(map_dyn.cpu_read_page, 0, sizeof(map_dyn.cpu_read_page));
  memset(map_dyn.cpu_write_page, 0, sizeof(map_dyn.cpu_write_page));
  memset(map_dyn.ppu_read_page, 0, sizeof(map_dyn.ppu_read_page));
  memset(map_dyn.ppu_write_page, 0, sizeof(map_dyn.ppu_write_page));
}

static const u8* map_cpu_read_pages(const u8* src, uint src_page, uint dst_page, uint pages)
{
  const u8* next = nullptr;
  for (uint i = 0; i < pages; ++i) {
    next = src ? src + (size_t)MAP_CPU_PAGE_SIZE * (src_page + i) : 0;
    map_dyn.cpu_read_page[dst_page + i] = next;
  }
  return next;
}

static u8* map_cpu_write_pages(u8* src, uint src_page, uint dst_page, uint pages)
{
  u8* next = nullptr;
  for (uint i = 0; i < pages; ++i) {
    next = src ? src + (size_t)MAP_CPU_PAGE_SIZE * (src_page + i) : 0;
    map_dyn.cpu_write_page[dst_page + i] = next;
  }
  return next;
}

static const u8* map_ppu_read_pages(const u8* src, uint src_page, uint dst_page, uint pages)
{
  const u8* next = nullptr;
  for (uint i = 0; i < pages; ++i) {
    next = src ? src + (size_t)MAP_PPU_PAGE_SIZE * (src_page + i) : 0;
    map_dyn.ppu_read_page[dst_page + i] = next;
  }
  return next;
}

static u8* map_ppu_write_pages(u8* src, uint src_page, uint dst_page, uint pages)
{
  u8* next = nullptr;
  for (uint i = 0; i < pages; ++i) {
    next = src ? src + (size_t)MAP_PPU_PAGE_SIZE * (src_page + i) : 0;
    map_dyn.ppu_write_page[dst_page + i] = next;
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

void map_prg_rom_page(uint sp, uint dp, uint shift)
{
  uint pages = (1 << shift);
  sp = map_page_clamp(sp, map_dyn.rom->prg_rom_size, MAP_CPU_PAGE_SHIFT + shift);
  map_cpu_read_pages(map_dyn.rom->prg_rom, sp * pages, dp * pages, pages);
  map_cpu_write_pages(nullptr, sp * pages, dp * pages, pages);
}

void map_prg_ram_page(uint sp, uint dp, uint shift)
{
  if (map_dyn.rom->prg_ram_size) {
    uint pages = (1 << shift);
    sp = map_page_clamp(sp, map_dyn.rom->prg_ram_size, MAP_CPU_PAGE_SHIFT + shift);
    map_cpu_read_pages(map_dyn.rom->prg_ram, sp * pages, dp * pages, pages);
    map_cpu_write_pages(map_dyn.rom->prg_ram, sp * pages, dp * pages, pages);
  }
}

void map_chr_page(uint sp, uint dp, uint shift)
{
  uint map_pages = (1 << shift);
  uint rom_pages = (map_dyn.rom->chr_rom_size >> (MAP_PPU_PAGE_SHIFT + shift));
  if (sp < rom_pages || map_dyn.rom->chr_ram_size == 0) {
    sp = map_page_clamp(sp, map_dyn.rom->chr_rom_size, MAP_PPU_PAGE_SHIFT + shift);
    map_ppu_read_pages(map_dyn.rom->chr_rom, sp * map_pages, dp * map_pages, map_pages);
    map_ppu_write_pages(0, sp * map_pages, dp * map_pages, map_pages);
  } else {
    sp -= rom_pages;
    sp = map_page_clamp(sp, map_dyn.rom->chr_ram_size, MAP_PPU_PAGE_SHIFT + shift);
    map_ppu_read_pages(map_dyn.rom->chr_ram, sp * map_pages, dp * map_pages, map_pages);
    map_ppu_write_pages(map_dyn.rom->chr_ram, sp * map_pages, dp * map_pages, map_pages);
  }
}

void map_prg_rom_page_4k(uint sp, uint dp) { map_prg_rom_page(sp, dp, 0); };
void map_prg_rom_page_8k(uint sp, uint dp) { map_prg_rom_page(sp, dp, 1); };
void map_prg_rom_page_16k(uint sp, uint dp) { map_prg_rom_page(sp, dp, 2); };
void map_prg_rom_page_32k(uint sp, uint dp) { map_prg_rom_page(sp, dp, 3); };

void map_prg_ram_page_4k(uint sp, uint dp) { map_prg_ram_page(sp, dp, 0); };
void map_prg_ram_page_8k(uint sp, uint dp) { map_prg_ram_page(sp, dp, 1); };
void map_prg_ram_page_16k(uint sp, uint dp) { map_prg_ram_page(sp, dp, 2); };
void map_prg_ram_page_32k(uint sp, uint dp) { map_prg_ram_page(sp, dp, 3); };

void map_chr_page_1k(uint sp, uint dp) { map_chr_page(sp, dp, 0); }
void map_chr_page_2k(uint sp, uint dp) { map_chr_page(sp, dp, 1); }
void map_chr_page_4k(uint sp, uint dp) { map_chr_page(sp, dp, 2); }
void map_chr_page_8k(uint sp, uint dp) { map_chr_page(sp, dp, 3); }

void map_ppu_nt_page(uint sp, uint dp)
{
  u8* src = map.vram + sp * MAP_PPU_PAGE_SIZE;
  map_dyn.ppu_read_page[dp] = src;
  map_dyn.ppu_write_page[dp] = src;
  map_dyn.ppu_read_page[dp + 4] = src;
  map_dyn.ppu_write_page[dp + 4] = src;
}

void map_ppu_nt_single_low()
{
  map_ppu_nt_page(0, 0x8);
  map_ppu_nt_page(0, 0x9);
  map_ppu_nt_page(0, 0xA);
  map_ppu_nt_page(0, 0xB);
}

void map_ppu_nt_single_high()
{
  map_ppu_nt_page(1, 0x8);
  map_ppu_nt_page(1, 0x9);
  map_ppu_nt_page(1, 0xA);
  map_ppu_nt_page(1, 0xB);
}

void map_ppu_nt_vert_mirror()
{
  map_ppu_nt_page(0, 0x8);
  map_ppu_nt_page(1, 0x9);
  map_ppu_nt_page(0, 0xA);
  map_ppu_nt_page(1, 0xB);
}

void map_ppu_nt_horiz_mirror()
{
  map_ppu_nt_page(0, 0x8);
  map_ppu_nt_page(0, 0x9);
  map_ppu_nt_page(1, 0xA);
  map_ppu_nt_page(1, 0xB);
}

void map_ppu_nt_four_screen()
{
  map_ppu_nt_page(0, 0x8);
  map_ppu_nt_page(1, 0x9);
  map_ppu_nt_page(2, 0xA);
  map_ppu_nt_page(3, 0xB);
}

static void map_nrom_load()
{
  map_prg_rom_page_16k(0, 2);
  map_prg_rom_page_16k(-1, 3);

  map_prg_ram_page_4k(0, 6); // ram may have < 8k
  map_prg_ram_page_4k(1, 7);

  for (uint i = 0; i < 8; ++i) { // rom may have < 8k
    map_chr_page_1k(i, i);
  }

  if (map_dyn.rom->alt_mirror) {
    map_ppu_nt_four_screen();
  } else if (map_dyn.rom->vert_mirror) {
    map_ppu_nt_vert_mirror();
  } else {
    map_ppu_nt_horiz_mirror();
  }
}

bool map_rom_load(mn_rom rom, bool init)
{
  map_dyn.rom = rom;
  rom->mapper_error = false;
  dev_region_update();
  map_nrom_load();

  if (rom->mapper == 0) {
    return true;
  }

  if (map_discrete_load()) {
    return true;
  }

  if (map_mmc_load(init)) {
    return true;
  }

  if (map_other_load()) {
    return true;
  }

  rom->mapper_error = true;
  return false;
}

bool mn_rom_set(mn_rom rom)
{
  memset(&map_dyn, 0, sizeof(map_dyn));
  memset(&map, 0, sizeof(map));
  if (rom) {
    if (!map_rom_load(rom, true)) {
      return false;
    }
    dev_power();
  }
  return true;
}
