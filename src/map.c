#include "map.h"
#include "rom.h"
#include <string.h>

static const size_t MAP_PRG_PAGE = 0x1000; // 4kb
static const size_t MAP_CHR_PAGE = 0x400; // 1kb
static const size_t MAP_NT_PAGE = 0x400; // 1kb

static struct
{
  u8 vram[0x1000];
  u8 wram[0x2000];
  u8 sram[0x2000];

  const u8* prg_page[8];
  const u8* chr_page[8];
  u8* nt_page[4];
  u8 reg[4];

  void (*load)();
  u8 (*cpu_read)(u16);
  void (*cpu_write)(u16, u8);
  u8 (*ppu_read)(u16);
  void (*ppu_write)(u16, u8);

  Rom rom;
} mapper;

static const u8* map_prg(const u8* src, int dst_from, int pages)
{
  const u8* next = 0;
  for (int i = 0; i < pages; ++i) {
    next = src + MAP_PRG_PAGE * i;
    mapper.prg_page[dst_from + i] = next;
  }
  return next;
}

static const u8* map_chr(const u8* src, int dst_from, int pages)
{
  const u8* next = 0;
  for (int i = 0; i < pages; ++i) {
    next = src + MAP_CHR_PAGE * i;
    mapper.chr_page[dst_from + i] = next;
  }
  return next;
}

static void map_nt_low()
{
  mapper.nt_page[0] = mapper.nt_page[1] = mapper.nt_page[2] = mapper.nt_page[3] = mapper.vram;
}

static void map_nt_high()
{
  mapper.nt_page[0] = mapper.nt_page[1] = mapper.nt_page[2] = mapper.nt_page[3] = mapper.vram + MAP_NT_PAGE;
}

static void map_nt_horiz()
{
  mapper.nt_page[0] = mapper.vram;
  mapper.nt_page[1] = mapper.vram;
  mapper.nt_page[2] = mapper.vram + MAP_NT_PAGE;
  mapper.nt_page[3] = mapper.vram + MAP_NT_PAGE;
}

static void map_nt_vert()
{
  mapper.nt_page[0] = mapper.vram;
  mapper.nt_page[1] = mapper.vram + MAP_NT_PAGE;
  mapper.nt_page[2] = mapper.vram;
  mapper.nt_page[3] = mapper.vram + MAP_NT_PAGE;
}

static void map_nt_four()
{
  mapper.nt_page[0] = mapper.vram + MAP_NT_PAGE * 0;
  mapper.nt_page[1] = mapper.vram + MAP_NT_PAGE * 1;
  mapper.nt_page[2] = mapper.vram + MAP_NT_PAGE * 2;
  mapper.nt_page[3] = mapper.vram + MAP_NT_PAGE * 3;
}

static void map_load_NROM()
{
  const u8* prg_next = map_prg(mapper.rom->prg, 0, 4);
  map_prg(mapper.rom->prg_pages == 1 ? mapper.rom->prg : prg_next, 4, 4);
  map_chr(mapper.rom->chr ? mapper.rom->chr : mapper.wram, 0, 8);

  if (mapper.rom->has_vram) {
    map_nt_four();
  } else if (mapper.rom->vert_mirror) {
    map_nt_vert();
  } else {
    map_nt_horiz();
  }
}

static u8 map_cpu_read_NROM(u16 addr)
{
  if (addr < 0x6000) {
    return 0; // eram todo: openbus
  } else if (addr < 0x8000) {
    if (mapper.rom->has_sram) {
      return mapper.sram[addr & 0x1FFF];
    } else {
      return 0; // todo: openbus
    }
  } else {
    if (mapper.rom->prg_pages == 1) {
      return mapper.rom->prg[addr & 0x3FFF];
    } else {
      return mapper.rom->prg[addr & 0x7FFF];
    }
  }
}

static void map_cpu_write_NROM(u16 addr, u8 val)
{
  if (addr < 0x6000) {
    // eram
  } else if (addr < 0x8000) {
    if (mapper.rom->has_sram) {
      mapper.sram[addr & 0x1FFF] = val;
    }
  }
}

static u8 map_ppu_read_NROM(u16 addr)
{
  if (addr < 0x2000) {
    return mapper.chr_page[addr >> 10][addr & 0x3FF];
  } else {
    addr &= 0xFFF;
    return mapper.nt_page[addr >> 10][addr & 0x3FF];
  }
}

static void map_ppu_write_NROM(u16 addr, u8 val)
{
  if (addr < 0x2000) {
    mapper.wram[addr] = val;
  } else {
    addr &= 0xFFF;
    mapper.nt_page[addr >> 10][addr & 0x3FF] = val;
  }
}

typedef struct
{
  u8 shift_reg;
  u8 write_count;
  u8 mirror_mode;
} Mmc1;

static Mmc1* map_mmc1_get()
{
  static Mmc1 mmc1;
  return &mmc1;
}

static void map_mmc1_apply()
{
  switch (map_mmc1_get()->mirror_mode) {
    default:
    case 0: map_nt_low(); break;
    case 1: map_nt_high(); break;
    case 2: map_nt_vert(); break;
    case 3: map_nt_horiz(); break;
  }
}

static void map_load_MMC1()
{
  map_load_NROM();
  Mmc1* mmc1 = map_mmc1_get();
  mmc1->shift_reg = 0;
  mmc1->write_count = 0;
  mmc1->mirror_mode = 0;
  map_mmc1_apply();
}

static u8 map_cpu_read_MMC1(u16 addr) { return map_cpu_read_NROM(addr); }
static u8 map_ppu_read_MMC1(u16 addr) { return map_ppu_read_NROM(addr); }
static void map_ppu_write_MMC1(u16 addr, u8 val) { map_ppu_write_NROM(addr, val); }

static void map_cpu_write_MMC1(u16 addr, u8 val)
{
  Mmc1* mmc1 = map_mmc1_get();

  if (val & 0x80) {
    mmc1->shift_reg = 0x10;
    mmc1->write_count = 0;
    mapper.reg[0] |= 0x0C;
    return;
  }

  bool bit = (val & 1);
  mmc1->shift_reg = (mmc1->shift_reg >> 1) | (bit << 4);
  mmc1->write_count++;

  if (mmc1->write_count == 5) {
    if (addr <= 0x9FFF) {
      mapper.reg[0] = mmc1->shift_reg;
      mmc1->mirror_mode = mmc1->shift_reg & 0x03;
    } else if (addr <= 0xBFFF) {
      mapper.reg[1] = mmc1->shift_reg;
    } else if (addr <= 0xDFFF) {
      mapper.reg[2] = mmc1->shift_reg;
    } else {
      mapper.reg[3] = mmc1->shift_reg;
    }

    mmc1->shift_reg = 0;
    mmc1->write_count = 0;

    map_mmc1_apply();
  }
}

static void map_load_UNROM() {}
static u8 map_cpu_read_UNROM(u16 addr) { return map_cpu_read_NROM(addr); }
static void map_cpu_write_UNROM(u16 addr, u8 val) { map_cpu_write_NROM(addr, val); }
static u8 map_ppu_read_UNROM(u16 addr) { return map_cpu_read_NROM(addr); }
static void map_ppu_write_UNROM(u16 addr, u8 val) { map_cpu_write_NROM(addr, val); }

static void map_load_CNROM() {}
static u8 map_cpu_read_CNROM(u16 addr) { return map_cpu_read_NROM(addr); }
static void map_cpu_write_CNROM(u16 addr, u8 val) { map_cpu_write_NROM(addr, val); }
static u8 map_ppu_read_CNROM(u16 addr) { return map_cpu_read_NROM(addr); }
static void map_ppu_write_CNROM(u16 addr, u8 val) { map_cpu_write_NROM(addr, val); }

static void map_load_MMC3() {}
static u8 map_cpu_read_MMC3(u16 addr) { return map_cpu_read_NROM(addr); }
static void map_cpu_write_MMC3(u16 addr, u8 val) { map_cpu_write_NROM(addr, val); }
static u8 map_ppu_read_MMC3(u16 addr) { return map_cpu_read_NROM(addr); }
static void map_ppu_write_MMC3(u16 addr, u8 val) { map_cpu_write_NROM(addr, val); }

static void map_load_MMC5() {}
static u8 map_cpu_read_MMC5(u16 addr) { return map_cpu_read_NROM(addr); }
static void map_cpu_write_MMC5(u16 addr, u8 val) { map_cpu_write_NROM(addr, val); }
static u8 map_ppu_read_MMC5(u16 addr) { return map_cpu_read_NROM(addr); }
static void map_ppu_write_MMC5(u16 addr, u8 val) { map_cpu_write_NROM(addr, val); }

static void map_load_AOROM() {}
static u8 map_cpu_read_AOROM(u16 addr) { return map_cpu_read_NROM(addr); }
static void map_cpu_write_AOROM(u16 addr, u8 val) { map_cpu_write_NROM(addr, val); }
static u8 map_ppu_read_AOROM(u16 addr) { return map_cpu_read_NROM(addr); }
static void map_ppu_write_AOROM(u16 addr, u8 val) { map_cpu_write_NROM(addr, val); }

#define M(id, name)                          \
  case id:                                   \
    mapper.load = map_load_##name;           \
    mapper.cpu_read = map_cpu_read_##name;   \
    mapper.cpu_write = map_cpu_write_##name; \
    mapper.ppu_read = map_ppu_read_##name;   \
    mapper.ppu_write = map_ppu_write_##name; \
    break;

void mn_rom_load(Rom rom)
{
  memset(&mapper, 0, sizeof(mapper));
  switch (rom->mapper) {
    default:
      M(0, NROM);
      M(1, MMC1);
      M(2, UNROM);
      M(3, CNROM);
      M(4, MMC3);
      M(5, MMC5);
      M(7, AOROM);
  }
  mapper.rom = rom;
  mapper.load();
}

#undef M

u8 map_cpu_read(u16 addr) { return mapper.cpu_read(addr); }
void map_cpu_write(u16 addr, u8 val) { mapper.cpu_write(addr, val); }
u8 map_ppu_read(u16 addr) { return mapper.ppu_read(addr); }
void map_ppu_write(u16 addr, u8 val) { mapper.ppu_write(addr, val); }
