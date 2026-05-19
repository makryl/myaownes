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
  u8 pad[8];
};

enum NESFlag : u8
{
  NES_FLAG_VERT_MIRROR = (1 << 0),
  NES_FLAG_SRAM = (1 << 1),
  NES_FLAG_TRAINER = (1 << 2),
  NES_FLAG_ALT_MIRROR = (1 << 3),
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

  size_t size = h.prg_pages * PRG_PAGE_SIZE + h.chr_pages * CHR_PAGE_SIZE;
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
  rom->chr = h.chr_pages > 0 ? rom->prg + h.prg_pages * PRG_PAGE_SIZE : 0;
  rom->prg_pages = h.prg_pages;
  rom->chr_pages = h.chr_pages;
  rom->mapper = h.mapper | (h.flags >> 4);
  rom->vert_mirror = (h.flags & NES_FLAG_VERT_MIRROR);
  rom->has_sram = (h.flags & NES_FLAG_SRAM);
  rom->has_trainer = (h.flags & NES_FLAG_TRAINER);
  rom->alt_mirror = (h.flags & NES_FLAG_ALT_MIRROR);
  return rom;
}

void mn_rom_release(Rom rom) { free(rom); }
