#include "map.h"
#include "cpu.h"
#include "dev.h"
#include "common.h"
#include "map_internal.h"
#include "map_discrete.h"
#include "map_mmc.h"
#include "map_other.h"

enum
{
  MAP_PAGE_SHIFT = 10, // 1kb
  MAP_PAGE_SIZE = 1 << MAP_PAGE_SHIFT,
  MAP_PAGE_MASK = MAP_PAGE_SIZE - 1,
};

MN_CACHE_LINE static struct
{
  map_cpu_cyc_cb cpu_cyc;
  map_cpu_read_cb cpu_read;
  map_cpu_write_cb cpu_write;
  map_cpu_internal_cb cpu_internal;
  map_ppu_addr_cb ppu_addr;
  map_ppu_read_cb ppu_read;
  map_ppu_write_cb ppu_write;

  mn_rom rom;

  MN_CACHE_LINE const u8* cpu_read_page[64];
  MN_CACHE_LINE u8* cpu_write_page[64];
  MN_CACHE_LINE const u8* ppu_read_page[16];
  MN_CACHE_LINE u8* ppu_write_page[16];
} map_dyn;

MN_CACHE_LINE static struct
{
  bool apu_irq: 1;
  bool irq: 1;
  MN_CACHE_LINE u8 ciram[0x1000];
  MN_CACHE_LINE u8 reg[MAP_REG_SIZE];
} map;

void map_set_cpu_cyc_cb(map_cpu_cyc_cb cb) { map_dyn.cpu_cyc = cb; }
void map_set_cpu_read_cb(map_cpu_read_cb cb) { map_dyn.cpu_read = cb; }
void map_set_cpu_write_cb(map_cpu_write_cb cb) { map_dyn.cpu_write = cb; }
void map_set_cpu_internal_cb(map_cpu_internal_cb cb) { map_dyn.cpu_internal = cb; }
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


uint map_cpu_read_raw(uint addr, uint val)
{
  const u8* page = map_dyn.cpu_read_page[addr >> MAP_PAGE_SHIFT];
  if (page) {
    val = page[addr & MAP_PAGE_MASK];
  }
  return val;
}

void map_cpu_write_raw(uint addr, uint val)
{
  u8* page = map_dyn.cpu_write_page[addr >> MAP_PAGE_SHIFT];
  if (page) {
    page[addr & MAP_PAGE_MASK] = val;
  }
}

uint map_ppu_read_raw(uint addr, uint val)
{
  const u8* page = map_dyn.ppu_read_page[addr >> MAP_PAGE_SHIFT];
  if (page) {
    val = page[addr & MAP_PAGE_MASK];
  }
  return val;
}

void map_ppu_write_raw(uint addr, uint val)
{
  u8* page = map_dyn.ppu_write_page[addr >> MAP_PAGE_SHIFT];
  if (page) {
    page[addr & MAP_PAGE_MASK] = val;
  }
}

uint map_cpu_read(uint addr, uint val, bool trace)
{
  if (!map_dyn.cpu_read || !map_dyn.cpu_read(addr, &val, trace)) {
    val = map_cpu_read_raw(addr, val);
  }
  return val;
}

void map_cpu_write(uint addr, uint val)
{
  if (!map_dyn.cpu_write || !map_dyn.cpu_write(addr, val)) {
    map_cpu_write_raw(addr, val);
  }
}

void map_cpu_internal(uint addr, uint val)
{
  if (map_dyn.cpu_internal) {
    map_dyn.cpu_internal(addr, val);
  }
}

void map_ppu_addr(uint addr)
{
  if (map_dyn.ppu_addr) {
    map_dyn.ppu_addr(addr);
  }
}

uint map_ppu_read(uint addr, uint val)
{
  if (!map_dyn.ppu_read || !map_dyn.ppu_read(addr, &val)) {
    val = map_ppu_read_raw(addr, val);
  }
  return val;
}

void map_ppu_write(uint addr, uint val)
{
  if (!map_dyn.ppu_write || !map_dyn.ppu_write(addr, val)) {
    map_ppu_write_raw(addr, val);
  }
}

void map_clear_pages()
{
  memset((void*)map_dyn.cpu_read_page, 0, sizeof(map_dyn.cpu_read_page));
  memset((void*)map_dyn.cpu_write_page, 0, sizeof(map_dyn.cpu_write_page));
  memset((void*)map_dyn.ppu_read_page, 0, sizeof(map_dyn.ppu_read_page));
  memset((void*)map_dyn.ppu_write_page, 0, sizeof(map_dyn.ppu_write_page));
}

static const u8* map_cpu_read_pages(const u8* src, uint src_page, uint dst_page, uint pages)
{
  const u8* next = nullptr;
  for (uint i = 0; i < pages; ++i) {
    next = src ? src + (size_t)MAP_PAGE_SIZE * (src_page + i) : 0;
    map_dyn.cpu_read_page[dst_page + i] = next;
  }
  return next;
}

static u8* map_cpu_write_pages(u8* src, uint src_page, uint dst_page, uint pages)
{
  u8* next = nullptr;
  for (uint i = 0; i < pages; ++i) {
    next = src ? src + (size_t)MAP_PAGE_SIZE * (src_page + i) : 0;
    map_dyn.cpu_write_page[dst_page + i] = next;
  }
  return next;
}

static const u8* map_ppu_read_pages(const u8* src, uint src_page, uint dst_page, uint pages)
{
  const u8* next = nullptr;
  for (uint i = 0; i < pages; ++i) {
    next = src ? src + (size_t)MAP_PAGE_SIZE * (src_page + i) : 0;
    map_dyn.ppu_read_page[dst_page + i] = next;
  }
  return next;
}

static u8* map_ppu_write_pages(u8* src, uint src_page, uint dst_page, uint pages)
{
  u8* next = nullptr;
  for (uint i = 0; i < pages; ++i) {
    next = src ? src + (size_t)MAP_PAGE_SIZE * (src_page + i) : 0;
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

static uint map_page_clamp(uint page, uint size, uint shift)
{
  uint pages = (size >> shift);
  if (pages == 0) {
    __builtin_debugtrap();
    return 0;
  }
  page &= map_page_mask(pages);
  if (page >= pages) {
    page %= pages;
  }
  return page;
}

void map_prg_clear_page(uint dp, uint shift)
{
  uint pages = (1 << shift);
  map_cpu_read_pages(nullptr, 0, dp * pages, pages);
  map_cpu_write_pages(nullptr, 0, dp * pages, pages);
}

void map_prg_rom_page(uint sp, uint dp, uint shift)
{
  uint pages = (1 << shift);
  sp = map_page_clamp(sp, map_dyn.rom->prg_rom_size, MAP_PAGE_SHIFT + shift);
  map_cpu_read_pages(map_dyn.rom->prg_rom, sp * pages, dp * pages, pages);
  map_cpu_write_pages(nullptr, sp * pages, dp * pages, pages);
}

void map_prg_ram_page(uint sp, uint dp, uint shift, bool readonly)
{
  if (map_dyn.rom->prg_ram_size < MAP_PAGE_SIZE) {
    // probably eeprom
  } else {
    uint pages = (1 << shift);
    sp = map_page_clamp(sp, map_dyn.rom->prg_ram_size, MAP_PAGE_SHIFT + shift);
    map_cpu_read_pages(map_dyn.rom->prg_ram, sp * pages, dp * pages, pages);
    map_cpu_write_pages(readonly ? nullptr : map_dyn.rom->prg_ram, sp * pages, dp * pages, pages);
  }
}

void map_chr_page(uint sp, uint dp, uint shift)
{
  uint map_pages = (1 << shift);
  uint rom_pages = (map_dyn.rom->chr_rom_size >> (MAP_PAGE_SHIFT + shift));
  if (sp < rom_pages || map_dyn.rom->chr_ram_size == 0) {
    sp = map_page_clamp(sp, map_dyn.rom->chr_rom_size, MAP_PAGE_SHIFT + shift);
    map_ppu_read_pages(map_dyn.rom->chr_rom, sp * map_pages, dp * map_pages, map_pages);
    map_ppu_write_pages(0, sp * map_pages, dp * map_pages, map_pages);
  } else {
    sp -= rom_pages;
    sp = map_page_clamp(sp, map_dyn.rom->chr_ram_size, MAP_PAGE_SHIFT + shift);
    map_ppu_read_pages(map_dyn.rom->chr_ram, sp * map_pages, dp * map_pages, map_pages);
    map_ppu_write_pages(map_dyn.rom->chr_ram, sp * map_pages, dp * map_pages, map_pages);
  }
}

void map_prg_clear_page_1k(uint dp) { map_prg_clear_page(dp, 0); }
void map_prg_clear_page_2k(uint dp) { map_prg_clear_page(dp, 1); }
void map_prg_clear_page_4k(uint dp) { map_prg_clear_page(dp, 2); }
void map_prg_clear_page_8k(uint dp) { map_prg_clear_page(dp, 3); }
void map_prg_clear_page_16k(uint dp) { map_prg_clear_page(dp, 4); }
void map_prg_clear_page_32k(uint dp) { map_prg_clear_page(dp, 5); }

void map_prg_rom_page_1k(uint sp, uint dp) { map_prg_rom_page(sp, dp, 0); };
void map_prg_rom_page_2k(uint sp, uint dp) { map_prg_rom_page(sp, dp, 1); };
void map_prg_rom_page_4k(uint sp, uint dp) { map_prg_rom_page(sp, dp, 2); };
void map_prg_rom_page_8k(uint sp, uint dp) { map_prg_rom_page(sp, dp, 3); };
void map_prg_rom_page_16k(uint sp, uint dp) { map_prg_rom_page(sp, dp, 4); };
void map_prg_rom_page_32k(uint sp, uint dp) { map_prg_rom_page(sp, dp, 5); };

void map_prg_ram_page_1k(uint sp, uint dp, bool readonly) { map_prg_ram_page(sp, dp, 0, readonly); };
void map_prg_ram_page_2k(uint sp, uint dp, bool readonly) { map_prg_ram_page(sp, dp, 1, readonly); };
void map_prg_ram_page_4k(uint sp, uint dp, bool readonly) { map_prg_ram_page(sp, dp, 2, readonly); };
void map_prg_ram_page_8k(uint sp, uint dp, bool readonly) { map_prg_ram_page(sp, dp, 3, readonly); };
void map_prg_ram_page_16k(uint sp, uint dp, bool readonly) { map_prg_ram_page(sp, dp, 4, readonly); };
void map_prg_ram_page_32k(uint sp, uint dp, bool readonly) { map_prg_ram_page(sp, dp, 5, readonly); };

void map_chr_page_1k(uint sp, uint dp) { map_chr_page(sp, dp, 0); }
void map_chr_page_2k(uint sp, uint dp) { map_chr_page(sp, dp, 1); }
void map_chr_page_4k(uint sp, uint dp) { map_chr_page(sp, dp, 2); }
void map_chr_page_8k(uint sp, uint dp) { map_chr_page(sp, dp, 3); }

void map_nt_page(uint sp, uint dp, bool use_chr)
{
  if (use_chr) {
    map_chr_page_1k(sp, dp);
    map_chr_page_1k(sp, dp + 4);
  } else {
    map_ciram_page(sp, dp);
  }
}

void map_cpu_ciram_page(uint sp, uint dp, bool readonly)
{
  map_cpu_read_pages(map.ciram, sp, dp, 1);
  map_cpu_write_pages(readonly ? nullptr : map.ciram, sp, dp, 1);
}

void map_ciram_page(uint sp, uint dp)
{
  u8* src = map.ciram + sp * (size_t)MAP_PAGE_SIZE;
  map_dyn.ppu_read_page[dp] = src;
  map_dyn.ppu_write_page[dp] = src;
  map_dyn.ppu_read_page[dp + 4] = src;
  map_dyn.ppu_write_page[dp + 4] = src;
}

void map_ciram_single_low()
{
  map_ciram_page(0, 0x8);
  map_ciram_page(0, 0x9);
  map_ciram_page(0, 0xA);
  map_ciram_page(0, 0xB);
}

void map_ciram_single_high()
{
  map_ciram_page(1, 0x8);
  map_ciram_page(1, 0x9);
  map_ciram_page(1, 0xA);
  map_ciram_page(1, 0xB);
}

void map_ciram_vert_mirror()
{
  map_ciram_page(0, 0x8);
  map_ciram_page(1, 0x9);
  map_ciram_page(0, 0xA);
  map_ciram_page(1, 0xB);
}

void map_ciram_horiz_mirror()
{
  map_ciram_page(0, 0x8);
  map_ciram_page(0, 0x9);
  map_ciram_page(1, 0xA);
  map_ciram_page(1, 0xB);
}

void map_ciram_four_screen()
{
  map_ciram_page(0, 0x8);
  map_ciram_page(1, 0x9);
  map_ciram_page(2, 0xA);
  map_ciram_page(3, 0xB);
}

static void map_nrom_load()
{
  map_prg_rom_page_16k(0, 2);
  map_prg_rom_page_16k(-1, 3);

  for (uint i = 0; i < 8; ++i) { // ram may have < 8k
    map_prg_ram_page_1k(i, 24 + i, false);
  }

  for (uint i = 0; i < 8; ++i) { // rom may have < 8k
    map_chr_page_1k(i, i);
  }

  if (map_dyn.rom->alt_mirror) {
    map_ciram_four_screen();
  } else if (map_dyn.rom->vert_mirror) {
    map_ciram_vert_mirror();
  } else {
    map_ciram_horiz_mirror();
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
