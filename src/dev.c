#include "dev.h"
#include "myaownes.h"
#include "cpu.h"
#include "ppu.h"
#include "apu.h"
#include "map.h"
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

static const u32 PRG_PAGE_SIZE = 0x4000;
static const u32 CHR_PAGE_SIZE = 0x2000;

static struct
{
  u32 out[256 * 240];
  u8 joy[2];
} dev;

void dev_power()
{
  apu_power();
  cpu_power();
  ppu_power();
}

void mn_reset()
{
  apu_reset();
  cpu_reset();
  ppu_reset();
}

void mn_frame(u8 joy1, u8 joy2)
{
  if (!map_ready()) {
    return;
  }
  dev.joy[0] = joy1;
  dev.joy[1] = joy2;
  bool vblank_before;
  do {
    vblank_before = ppu_vblank();
    cpu_tick();
  } while (vblank_before || !ppu_vblank());
#if MN_TRACE_BLARGG
  tracef("\e[2J\e[Hstatus=%02X\n%s\n", *map_prg_ram(), (const char*)map_prg_ram() + 4);
#endif
}

u8 dev_input(u8 idx) { return dev.joy[idx]; }
void dev_output(u16 idx, u32 color) { dev.out[idx] = color; }
u32* mn_output() { return dev.out; }

static float dev_aspect_scale(bool a) { return (256.0f / 240.0f) * (a ? (8.0f / 7.0f) : 1.0f); }
static float dev_aspect_overscan(bool a) { return a ? (4.0f / 3.0f) : (256.0f / 224.0f); }
static float dev_aspect_target(bool a, bool o) { return o ? dev_aspect_overscan(a) : dev_aspect_scale(a); }

void mn_output_size(bool auto_aspect, bool overscan, float scale, int* dw, int* dh)
{
  float h = (overscan ? 224.0f : 240.0f) * scale;
  float w = h * dev_aspect_target(auto_aspect, overscan);
  *dw = (int)(w + 0.5f);
  *dh = (int)(h + 0.5f);
}

void mn_output_fit(bool auto_aspect, bool overscan, float sw, float sh, float* dx, float* dy, float* dw, float* dh)
{
  float aspect_scale = dev_aspect_scale(auto_aspect);
  float aspect_target = dev_aspect_target(auto_aspect, overscan);
  float aspect_output = sw / sh;
  if (aspect_target <= aspect_output) {
    *dh = sh * aspect_target / aspect_scale;
    *dw = *dh * aspect_scale;
  } else {
    *dw = sw;
    *dh = *dw / aspect_scale;
  }
  *dx = (sw - *dw) / 2.0f;
  *dy = (sh - *dh) / 2.0f;
}

bool mn_save(const char* path)
{
  Rom rom = mn_rom_get();
  if (!rom) {
    errorf("no active rom set\n");
    return false;
  }
  FILE* f = fopen(path, "wb");
  if (!f) {
    errorf("can not open for write %s: %s\n", path, strerror(errno));
    return false;
  }
  const char* err_fmt = "can not write file %s: %s\n";
  if (fwrite(apu_data(), 1, apu_size(), f) != apu_size()) {
    errorf(err_fmt, path, strerror(errno));
    return false;
  }
  if (fwrite(cpu_data(), 1, cpu_size(), f) != cpu_size()) {
    errorf(err_fmt, path, strerror(errno));
    return false;
  }
  if (fwrite(ppu_data(), 1, ppu_size(), f) != ppu_size()) {
    errorf(err_fmt, path, strerror(errno));
    return false;
  }
  if (fwrite(map_data(), 1, map_size(), f) != map_size()) {
    errorf(err_fmt, path, strerror(errno));
    return false;
  }
  if (fwrite(map_prg_ram(), 1, rom->prg_ram_size, f) != rom->prg_ram_size) {
    errorf(err_fmt, path, strerror(errno));
    return false;
  }
  if (fwrite(map_chr_ram(), 1, rom->chr_ram_size, f) != rom->chr_ram_size) {
    errorf(err_fmt, path, strerror(errno));
    return false;
  }
  return true;
}

bool mn_load(const char* path)
{
  Rom rom = mn_rom_get();
  if (!rom) {
    errorf("no active rom set\n");
    return false;
  }
  FILE* f = fopen(path, "rb");
  if (!f) {
    errorf("can not open for read %s: %s\n", path, strerror(errno));
    return false;
  }
  const char* err_fmt = "can not read file %s: %s\n";
  if (fread(apu_data(), 1, apu_size(), f) != apu_size()) {
    errorf(err_fmt, path, strerror(errno));
    return false;
  }
  if (fread(cpu_data(), 1, cpu_size(), f) != cpu_size()) {
    errorf(err_fmt, path, strerror(errno));
    return false;
  }
  if (fread(ppu_data(), 1, ppu_size(), f) != ppu_size()) {
    errorf(err_fmt, path, strerror(errno));
    return false;
  }
  if (fread(map_data(), 1, map_size(), f) != map_size()) {
    errorf(err_fmt, path, strerror(errno));
    return false;
  }
  if (fread(map_prg_ram(), 1, rom->prg_ram_size, f) != rom->prg_ram_size) {
    errorf(err_fmt, path, strerror(errno));
    return false;
  }
  if (fread(map_chr_ram(), 1, rom->chr_ram_size, f) != rom->chr_ram_size) {
    errorf(err_fmt, path, strerror(errno));
    return false;
  }
  return map_rom_load(rom, false);
}


bool mn_sram_save(const char* path)
{
  Rom rom = mn_rom_get();
  if (!rom) {
    errorf("no active rom set\n");
    return false;
  }
  FILE* f = fopen(path, "wb");
  if (!f) {
    errorf("can not open for write %s: %s\n", path, strerror(errno));
    return false;
  }
  const char* err_fmt = "can not write file %s: %s\n";
  if (fwrite(map_prg_ram(), 1, rom->prg_ram_size, f) != rom->prg_ram_size) {
    errorf(err_fmt, path, strerror(errno));
    return false;
  }
  if (fwrite(map_chr_ram(), 1, rom->chr_ram_size, f) != rom->chr_ram_size) {
    errorf(err_fmt, path, strerror(errno));
    return false;
  }
  return true;
}

bool mn_sram_load(const char* path)
{
  Rom rom = mn_rom_get();
  if (!rom) {
    errorf("no active rom set\n");
    return false;
  }
  FILE* f = fopen(path, "rb");
  if (!f) {
    errorf("can not open for read %s: %s\n", path, strerror(errno));
    return false;
  }
  const char* err_fmt = "can not read file %s: %s\n";
  if (fread(map_prg_ram(), 1, rom->prg_ram_size, f) != rom->prg_ram_size) {
    errorf(err_fmt, path, strerror(errno));
    return false;
  }
  if (fread(map_chr_ram(), 1, rom->chr_ram_size, f) != rom->chr_ram_size) {
    errorf(err_fmt, path, strerror(errno));
    return false;
  }
  return true;
}

Rom mn_rom_file(const char* path)
{
  FILE* f = fopen(path, "rb");
  if (!f) {
    errorf("can not open %s: %s\n", path, strerror(errno));
    return nullptr;
  }

  struct NESHeader h;
  if (!fread(&h, sizeof(h), 1, f)) {
    errorf("can not read header from %s: %s\n", path, strerror(errno));
    fclose(f);
    return nullptr;
  }
  if (h.magic[0] != 'N' || h.magic[1] != 'E' || h.magic[2] != 'S' || h.magic[3] != 0x1A) {
    errorf("not NES file %s\n", path);
    fclose(f);
    return nullptr;
  }

  if (h.prg_pages == 0) {
    errorf("no rom_pages in %s\n", path);
    fclose(f);
    return nullptr;
  }

  bool is_v2 = (h.mapper_sig2 & 0x0C) == 0x08;

  u32 prg_rom_size = 0;
  u32 chr_rom_size = 0;

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

  u32 total_rom_size = prg_rom_size + chr_rom_size;
  void* mem = malloc(sizeof(struct Rom) + total_rom_size);
  if (!mem) {
    errorf("no memory (%d) for %s", (int)total_rom_size, path);
    fclose(f);
    return nullptr;
  }

  if (h.mapper_flags & NES_FLAG_TRAINER) { // todo
    fseek(f, 512, SEEK_CUR);
  }

  if (fread((u8*)mem + sizeof(struct Rom), 1, total_rom_size, f) != total_rom_size) {
    errorf("can not read data from %s: %s\n", path, strerror(errno));
    fclose(f);
    free(mem);
    return nullptr;
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
