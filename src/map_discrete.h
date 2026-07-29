#pragma once

#include "map.h"
#include "map_internal.h"

enum
{
  MAP_MIRROR_NONE = 0,
  MAP_MIRROR_SINGLE_LO_HI,
  MAP_MIRROR_VERT_HORIZ,
};

typedef struct
{
  u8 prg_mask;
  u8 prg_shift;
  u8 prg_slot;
  u8 prg_size_kb;

  u8 chr_mask;
  u8 chr_shift;
  u8 chr_slot;
  u8 chr_size_kb;

  u8 mirror_mask;
  u8 mirror_mode;

  u8 bus_conflict;
} map_discrete_common;
static_assert(sizeof(map_discrete_common) <= MAP_REG_SIZE);

static void map_discrete_common_mirror(u8 mode, bool val)
{
  switch (mode) {
    case MAP_MIRROR_SINGLE_LO_HI:
      if (val) {
        map_ppu_nt_single_high();
      } else {
        map_ppu_nt_single_low();
      }
      break;
    case MAP_MIRROR_VERT_HORIZ:
      if (val) {
        map_ppu_nt_horiz_mirror();
      } else {
        map_ppu_nt_vert_mirror();
      }
      break;
  }
}

static u8 map_bus_conflict(u16 addr, u8 val)
{
  u8 read_val = 0xFF;
  map_cpu_read(addr, &read_val, true);
  return val & read_val;
}

static bool map_discrete_common_cpu_write(u16 addr, u8 val)
{
  if (addr < 0x8000) {
    return false;
  }

  map_discrete_common* reg = (map_discrete_common*)map_reg();

  if (reg->bus_conflict) {
    val = map_bus_conflict(addr, val);
  }

  if (reg->prg_mask) {
    uint src_page = ((val & reg->prg_mask) >> reg->prg_shift);
    if (reg->prg_size_kb == 16) {
      map_prg_rom_page_16k(src_page, reg->prg_slot + 2);
    } else if (reg->prg_size_kb == 32) {
      src_page <<= 1;
      map_prg_rom_page_16k(src_page, 2);
      map_prg_rom_page_16k(src_page + 1, 3);
    }
  }

  if (reg->chr_mask) {
    uint src_page = ((val & reg->chr_mask) >> reg->chr_shift);
    if (reg->chr_size_kb == 4) {
      map_chr_page_4k(src_page, reg->chr_slot);
    } else if (reg->chr_size_kb == 8) {
      map_chr_page_8k(src_page, 0);
    }
  }

  if (reg->mirror_mask) {
    map_discrete_common_mirror(reg->mirror_mode, val & reg->mirror_mask);
  }

  return true;
}

static void map_discrete_common_load(u8 prg_mask, u8 prg_shift, u8 prg_slot, u8 prg_size_kb, u8 chr_mask, u8 chr_shift,
                                     u8 chr_slot, u8 chr_size_kb, u8 mirror_mask, u8 mirror_mode, u8 bus_conflict)
{
  map_discrete_common* reg = (map_discrete_common*)map_reg();
  reg->prg_mask = prg_mask;
  reg->prg_shift = prg_shift;
  reg->prg_slot = prg_slot;
  reg->prg_size_kb = prg_size_kb;
  reg->chr_mask = chr_mask;
  reg->chr_shift = chr_shift;
  reg->chr_slot = chr_slot;
  reg->chr_size_kb = chr_size_kb;
  reg->mirror_mask = mirror_mask;
  reg->mirror_mode = mirror_mode;
  reg->bus_conflict = bus_conflict;
  map_set_cpu_write_cb(map_discrete_common_cpu_write);
  if (prg_size_kb == 32) {
    map_prg_rom_page_16k(0, 2);
    map_prg_rom_page_16k(1, 3);
  }
  map_discrete_common_mirror(mirror_mode, 0);
}

static bool map_bnrom_nina_cpu_write(u16 addr, u8 val)
{
  if (addr < 0x7FFD) {
    return false;
  }
  mn_rom rom = mn_rom_get();
  bool bnrom = (rom->submapper == 2) || (rom->submapper == 0 && (rom->chr_rom_size + rom->chr_ram_size) == 8 * 1024);
  if (bnrom) {
    val = map_bus_conflict(addr, val);
    if (addr >= 0x8000) {
      map_prg_rom_page_32k(val, 1);
    } else {
      return false;
    }
  } else { // NINA
    switch (addr) {
      case 0x7FFD: map_prg_rom_page_32k(val, 1); break;
      case 0x7FFE: map_chr_page_4k(val, 0); break;
      case 0x7FFF: map_chr_page_4k(val, 1); break;
      default: return false;
    }
  }
  return true;
}

static void map_bnrom_nina_load() { map_set_cpu_write_cb(map_bnrom_nina_cpu_write); }

bool map_discrete_load(uint mapper, uint submapper)
{
  switch (mapper) {
    case 2: map_discrete_common_load(0xFF, 0, 0, 16, 0, 0, 0, 0, 0, 0, submapper != 1); break; // UxROM
    case 3: map_discrete_common_load(0, 0, 0, 0, 0x0F, 0, 0, 8, 0, 0, submapper != 1); break; // CxROM
    case 7:
      map_discrete_common_load(0x0F, 0, 0, 32, 0, 0, 0, 0, 0x10, MAP_MIRROR_SINGLE_LO_HI, submapper == 2);
      break; // AxROM
    case 11: map_discrete_common_load(0x0F, 0, 0, 32, 0xF0, 4, 0, 8, 0, 0, 0); break; // Color Dreams, bus conflict?
    case 13: map_discrete_common_load(0, 0, 0, 0, 0x03, 0, 1, 4, 0, 0, 1); break; // CPROM
    case 34: map_bnrom_nina_load(); break; // BxROM, NINA
    case 66: map_discrete_common_load(0xF0, 4, 0, 32, 0x0F, 0, 0, 8, 0, 0, 1); break; // GxROM
    case 94: map_discrete_common_load(0xFF, 2, 0, 16, 0, 0, 0, 0, 0, 0, 1); break; // UxROM
    case 177: map_discrete_common_load(0x1F, 0, 0, 32, 0, 0, 0, 0, 0x20, MAP_MIRROR_VERT_HORIZ, 0); break; // BxROM
    case 180: map_discrete_common_load(0xFF, 0, 1, 16, 0, 0, 0, 0, 0, 0, 1); break; // UxROM
    case 185: map_discrete_common_load(0, 0, 0, 0, 0x0F, 0, 0, 8, 0, 0, 1); break; // CxROM, CS1/CS2 no impl
    case 241: map_discrete_common_load(0xFF, 0, 0, 32, 0, 0, 0, 0, 0, 0, 0); break; // BxROM, LPC Speech Chip no impl
    default: return false;
  }
  return true;
}
