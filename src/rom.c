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
  u8 flags;
  u8 mapper;
  u8 mapper_ext;
  u8 prg_chr_ext;
  u8 prg_ram_size;
  u8 chr_ram_size;
  u8 tv_system;
  u8 hw_type;
  u8 pad[2];
};

enum : u8
{
  NES_FLAG_VERT_MIRROR = (1 << 0),
  NES_FLAG_SRAM = (1 << 1),
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

  size_t prg_size = (h.prg_pages | ((h.prg_chr_ext & 0x0F) << 8)) * PRG_PAGE_SIZE;
  size_t chr_size = (h.chr_pages | (h.prg_chr_ext >> 4)) * CHR_PAGE_SIZE;

  size_t size = prg_size + chr_size;
  void* mem = malloc(sizeof(struct Rom) + size);
  if (!mem) {
    errorf("no memory (%d) for %s", (int)size, path);
    fclose(f);
    return 0;
  }

  if (h.flags & NES_FLAG_TRAINER) { // todo
    fseek(f, 512, SEEK_CUR);
  }

  if (fread(mem + sizeof(struct Rom), 1, size, f) != size) {
    errorf("can not read data from %s: %s\n", path, strerror(errno));
    fclose(f);
    free(mem);
    return 0;
  }

  fclose(f);

  Rom rom = mem;
  rom->prg = mem + sizeof(struct Rom);
  rom->chr = chr_size > 0 ? rom->prg + prg_size : 0;
  rom->prg_size = prg_size;
  rom->chr_size = chr_size;
  rom->prg_ram_size = 64 << (h.prg_ram_size & 0x0F);
  rom->prg_sram_size = (h.prg_ram_size > 0) ? 64 << (h.prg_ram_size >> 4) : rom->has_sram ? 8192 : 0;
  rom->chr_ram_size = (h.chr_ram_size > 0) ? 64 << (h.chr_ram_size & 0x0F) : (chr_size == 0) ? 8192 : 0;
  rom->chr_sram_size = 64 << (h.chr_ram_size >> 4);
  rom->mapper = (h.mapper & 0xF0) | (h.flags >> 4) | ((h.mapper_ext & 0x0F) << 8);
  rom->hw_type = h.hw_type;
  rom->vert_mirror = (h.flags & NES_FLAG_VERT_MIRROR);
  rom->has_sram = (h.flags & NES_FLAG_SRAM);
  rom->has_trainer = (h.flags & NES_FLAG_TRAINER);
  rom->alt_mirror = (h.flags & NES_FLAG_ALT_MIRROR);
  rom->ntsc = (h.tv_system & NES_TV_NTSC) || (h.tv_system & NES_TV_MULTI);
  rom->pal = (h.tv_system & NES_TV_PAL) || (h.tv_system & NES_TV_MULTI);
  rom->dendy = (h.tv_system & NES_TV_DENDY) || (h.tv_system & NES_TV_MULTI);

  return rom;
}

void mn_rom_release(Rom rom) { free(rom); }
