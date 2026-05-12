#include "rom.h"
#include "common.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

struct NESHeader
{
  u8 magic[4];
  u8 rom_pages;
  u8 vrom_pages;
  u8 flags;
  u8 mapper;
  u8 pad[8];
};

enum NESFlag : u8
{
  NES_FLAG_VERT_MIRROR = (1 << 0),
  NES_FLAG_SRAM = (1 << 1),
  NES_FLAG_TRAINER = (1 << 2),
  NES_FLAG_VRAM = (1 << 3),
};

static const size_t ROM_PAGE_SIZE = 0x4000;
static const size_t VROM_PAGE_SIZE = 0x2000;

Rom mn_nes_file(const char* path)
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

  if (h.rom_pages == 0) {
    errorf("no rom_pages in %s\n", path);
    fclose(f);
    return 0;
  }

  size_t size = h.rom_pages * ROM_PAGE_SIZE + h.vrom_pages * VROM_PAGE_SIZE;
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

  Rom nes = mem;
  nes->prg = mem + sizeof(struct Rom);
  nes->chr = h.vrom_pages > 0 ? nes->prg + h.rom_pages * ROM_PAGE_SIZE : 0;
  nes->prg_pages = h.rom_pages;
  nes->chr_pages = h.vrom_pages;
  nes->mapper = h.mapper | (h.flags >> 4);
  nes->vert_mirror = (h.flags & NES_FLAG_VERT_MIRROR);
  nes->has_sram = (h.flags & NES_FLAG_SRAM);
  nes->has_trainer = (h.flags & NES_FLAG_TRAINER);
  nes->has_vram = (h.flags & NES_FLAG_VRAM);
  return nes;
}

void mn_nes_release(Rom rom) { free(rom); }
