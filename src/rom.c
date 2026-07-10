#include "rom.h"
#include "common.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

struct NESHeader
{
  u8 magic[4];
  u8 prg_pages;
  u8 chr_pages;
  u8 mapper_flags;
  u8 mapper_sig2;
  u8 sub_mapper;
  u8 prg_chr_ext;
  u8 prg_ram_size;
  u8 chr_ram_size;
  u8 tv_system;
  u8 other[3];
};

enum : u8
{
  NES_FLAG_VERT_MIRROR = (1 << 0),
  NES_FLAG_BATTERY = (1 << 1),
  NES_FLAG_TRAINER = (1 << 2),
  NES_FLAG_ALT_MIRROR = (1 << 3),
};

enum : u8
{
  NES_TV_NTSC = 0,
  NES_TV_PAL,
  NES_TV_MULTI,
  NES_TV_DENDY,
};

static const size_t PRG_PAGE_SIZE = 0x4000;
static const size_t CHR_PAGE_SIZE = 0x2000;

Rom mn_rom_file(const char* path)
{
  FILE* f = fopen(path, "rb");
  if (!f) {
    errorf("can not open %s: %s\n", path, strerror(errno));
    return 0;
  }

  struct NESHeader h;
  if (!fread(&h, sizeof(h), 1, f)) {
    errorf("can not read header from %s: %s\n", path, strerror(errno));
    fclose(f);
    return 0;
  }
  if (h.magic[0] != 'N' || h.magic[1] != 'E' || h.magic[2] != 'S' || h.magic[3] != 0x1A) {
    errorf("not NES file %s\n", path);
    fclose(f);
    return 0;
  }

  if (h.prg_pages == 0) {
    errorf("no rom_pages in %s\n", path);
    fclose(f);
    return 0;
  }

  bool is_v2 = (h.mapper_sig2 & 0x0C) == 0x08;

  size_t prg_rom_size = 0;
  size_t chr_rom_size = 0;

  if (is_v2) {
    u8 prg_ext = h.prg_chr_ext & 0x0F;
    u8 chr_ext = h.prg_chr_ext >> 4;
    if (prg_ext == 0x0F) {
      prg_rom_size = (1ull << (h.prg_pages >> 2)) * ((h.prg_pages & 0x03) * 2 + 1);
    } else {
      prg_rom_size = (h.prg_pages | (prg_ext << 8)) * PRG_PAGE_SIZE;
    }
    if (chr_ext == 0x0F) {
      chr_rom_size = (1ull << (h.chr_pages >> 2)) * ((h.chr_pages & 0x03) * 2 + 1);
    } else {
      chr_rom_size = (h.chr_pages | (chr_ext << 4)) * CHR_PAGE_SIZE;
    }
  } else {
    prg_rom_size = h.prg_pages * PRG_PAGE_SIZE;
    chr_rom_size = h.chr_pages * CHR_PAGE_SIZE;
  }

  size_t total_rom_size = prg_rom_size + chr_rom_size;
  void* mem = malloc(sizeof(struct Rom) + total_rom_size);
  if (!mem) {
    errorf("no memory (%d) for %s", (int)total_rom_size, path);
    fclose(f);
    return 0;
  }

  if (h.mapper_flags & NES_FLAG_TRAINER) { // todo
    fseek(f, 512, SEEK_CUR);
  }

  if (fread((u8*)mem + sizeof(struct Rom), 1, total_rom_size, f) != total_rom_size) {
    errorf("can not read data from %s: %s\n", path, strerror(errno));
    fclose(f);
    free(mem);
    return 0;
  }

  fclose(f);


  Rom rom = mem;
  rom->prg_rom = (u8*)mem + sizeof(struct Rom);
  rom->chr_rom = chr_rom_size > 0 ? rom->prg_rom + prg_rom_size : 0;
  rom->prg_rom_size = prg_rom_size;
  rom->chr_rom_size = chr_rom_size;
  rom->vert_mirror = (h.mapper_flags & NES_FLAG_VERT_MIRROR);
  rom->has_trainer = (h.mapper_flags & NES_FLAG_TRAINER);
  rom->alt_mirror = (h.mapper_flags & NES_FLAG_ALT_MIRROR);

  if (is_v2) {
    rom->mapper = (h.mapper_flags >> 4) | (h.mapper_sig2 & 0xF0) | ((h.sub_mapper & 0x0F) << 8);
    rom->submapper = (h.sub_mapper >> 4);
    rom->ntsc = (h.tv_system == NES_TV_NTSC) || (h.tv_system == NES_TV_MULTI);
    rom->pal = (h.tv_system == NES_TV_PAL) || (h.tv_system == NES_TV_MULTI);
    rom->dendy = (h.tv_system == NES_TV_DENDY) || (h.tv_system == NES_TV_MULTI);

    u8 prg_ram_shift = h.prg_ram_size & 0x0F;
    u8 prg_sram_shift = h.prg_ram_size >> 4;
    u8 chr_ram_shift = h.chr_ram_size & 0x0F;
    u8 chr_sram_shift = h.chr_ram_size >> 4;

    u32 prg_ram_size = (prg_ram_shift > 0) ? 64 << prg_ram_shift : 0;
    u32 prg_sram_size = (prg_sram_shift > 0) ? 64 << prg_sram_shift : 0;
    u32 chr_ram_size = (chr_ram_shift > 0) ? 64 << chr_ram_shift : 0;
    u32 chr_sram_size = (chr_sram_shift > 0) ? 64 << chr_sram_shift : 0;

    if ((h.mapper_flags & NES_FLAG_BATTERY) && prg_sram_size == 0 && chr_sram_size == 0) {
      prg_sram_size = 8192;
    }

    if (chr_rom_size == 0 && chr_ram_size == 0 && chr_sram_size == 0) {
      chr_ram_size = 8192;
    }

    rom->prg_ram_size = prg_ram_size + prg_sram_size;
    rom->chr_ram_size = chr_ram_size + chr_sram_size;

    rom->has_prg_battery = prg_sram_size > 0;
    rom->has_chr_battery = chr_sram_size > 0;
  } else {
    rom->mapper = (h.mapper_flags >> 4) | (h.mapper_sig2 & 0xF0);
    rom->submapper = 0;
    rom->ntsc = true;
    rom->pal = false;
    rom->dendy = false;

    rom->prg_ram_size = 8192;
    rom->chr_ram_size = chr_rom_size == 0 ? 8192 : 0;

    rom->has_prg_battery = (h.mapper_flags & NES_FLAG_BATTERY);
    rom->has_chr_battery = false;
  }

  return rom;
}

void mn_rom_release(Rom rom) { free(rom); }
