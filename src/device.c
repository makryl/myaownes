#include "myanes.h"

#include "cpu.h"
#include "device.h"

Device device;

typedef struct
{
  u8 magic[4];
  u8 rom_pages;
  u8 vrom_pages;
  u8 flags;
  u8 mapper;
  u8 pad[8];
} NESHeader;

static const size_t ROM_PAGE_SIZE = 0x4000;
static const size_t VROM_PAGE_SIZE = 0x2000;

bool mn_load(const void* data, int size)
{
  if ((size_t)size < sizeof(NESHeader) + ROM_PAGE_SIZE) {
    errorf("not enough data\n");
    return false;
  }

  const NESHeader* h = data;
  if (h->magic[0] != 'N' || h->magic[1] != 'E' || h->magic[2] != 'S' || h->magic[3] != 0x1A) {
    errorf("not NES file\n");
    return false;
  }

  if ((size_t)size < sizeof(NESHeader) + h->rom_pages * ROM_PAGE_SIZE + h->vrom_pages * VROM_PAGE_SIZE) {
    errorf("not enough data\n");
    return false;
  }

  device.rom = data + sizeof(NESHeader);
  device.vrom = h->vrom_pages > 0 ? data + sizeof(NESHeader) + h->rom_pages * ROM_PAGE_SIZE : 0;
  device.rom_pages = h->rom_pages;
  device.vrom_pages = h->vrom_pages;
  device.flags = h->flags & 0xF;
  device.mapper = h->mapper | (h->flags >> 4);

  return true;
}

void mn_reset() { cpu_reset(); }

void mn_tick() { cpu_tick(); }
