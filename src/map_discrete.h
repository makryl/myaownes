#pragma once

#include "map.h"
#include "map_internal.h"

enum
{
  MAP_MIRROR_NONE = 0,
  MAP_MIRROR_SINGLE_LO_HI,
  MAP_MIRROR_VERT_HORIZ,
  MAP_MIRROR_HORIZ_VERT,
};

typedef struct
{
  u16 addr_mask;
  u16 addr_val;

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
    case MAP_MIRROR_HORIZ_VERT:
      if (val) {
        map_ppu_nt_vert_mirror();
      } else {
        map_ppu_nt_horiz_mirror();
      }
      break;
  }
}

static u8 map_bus_conflict(u16 addr, u8 val)
{
  u8 read_val = 0xFF;
  map_cpu_read(addr, &read_val, false);
  return val & read_val;
}

static bool map_discrete_common_cpu_write(u16 addr, u8 val)
{
  map_discrete_common* reg = (map_discrete_common*)map_reg();

  if ((addr & reg->addr_mask) != reg->addr_val) {
    return false;
  }

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

static void map_discrete_common_load(u16 addr_mask, u16 addr_val, u8 prg_mask, u8 prg_shift, u8 prg_slot,
                                     u8 prg_size_kb, u8 chr_mask, u8 chr_shift, u8 chr_slot, u8 chr_size_kb,
                                     u8 mirror_mask, u8 mirror_mode, u8 bus_conflict)
{
  map_discrete_common* reg = (map_discrete_common*)map_reg();
  reg->addr_mask = addr_mask;
  reg->addr_val = addr_val;
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

static void map_discrete_prg_chr_mir(u8 prg_mask, u8 prg_shift, u8 prg_slot, u8 prg_size_kb, u8 chr_mask, u8 chr_shift,
                                     u8 chr_slot, u8 chr_size_kb, u8 mirror_mask, u8 mirror_mode, u8 bus_conflict)
{
  map_discrete_common_load(0x8000, 0x8000, prg_mask, prg_shift, prg_slot, prg_size_kb, chr_mask, chr_shift, chr_slot,
                           chr_size_kb, mirror_mask, mirror_mode, bus_conflict);
}

static void map_discrete_prg_chr(u8 prg_mask, u8 prg_shift, u8 prg_slot, u8 prg_size_kb, u8 chr_mask, u8 chr_shift,
                                 u8 chr_slot, u8 chr_size_kb, u8 bus_conflict)
{
  map_discrete_common_load(0x8000, 0x8000, prg_mask, prg_shift, prg_slot, prg_size_kb, chr_mask, chr_shift, chr_slot,
                           chr_size_kb, 0, 0, bus_conflict);
}

static void map_discrete_prg_mir(u8 prg_mask, u8 prg_shift, u8 prg_slot, u8 prg_size_kb, u8 mirror_mask, u8 mirror_mode,
                                 u8 bus_conflict)
{
  map_discrete_prg_chr_mir(prg_mask, prg_shift, prg_slot, prg_size_kb, 0, 0, 0, 0, mirror_mask, mirror_mode,
                           bus_conflict);
}

static void map_discrete_prg(u8 prg_mask, u8 prg_shift, u8 prg_slot, u8 prg_size_kb, u8 bus_conflict)
{
  map_discrete_prg_mir(prg_mask, prg_shift, prg_slot, prg_size_kb, 0, 0, bus_conflict);
}

static void map_discrete_chr_mir(u8 chr_mask, u8 chr_shift, u8 chr_slot, u8 chr_size_kb, u8 mirror_mask, u8 mirror_mode,
                                 u8 bus_conflict)
{
  map_discrete_prg_chr_mir(0, 0, 0, 0, chr_mask, chr_shift, chr_slot, chr_size_kb, mirror_mask, mirror_mode,
                           bus_conflict);
}

static void map_discrete_chr(u8 chr_mask, u8 chr_shift, u8 chr_slot, u8 chr_size_kb, u8 bus_conflict)
{
  map_discrete_chr_mir(chr_mask, chr_shift, chr_slot, chr_size_kb, 0, 0, bus_conflict);
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

static bool map_j87_cpu_write(u16 addr, u8 val)
{
  if ((addr & 0xE000) == 0x6000) {
    map_chr_page_8k(((val & 1) << 1) | ((val >> 1) & 1), 0);
    return true;
  }
  return false;
}

static bool map_camerica071_cpu_write(u16 addr, u8 val)
{
  bool result = false;
  u8* reg = map_reg();
  if ((addr & 0xC000) == 0x8000) { // outer bank
    reg[0] = (val & 0x18) << 1;
    map_prg_rom_page_16k(reg[0] | reg[1], 2);
    result = true;
  }
  if ((addr & 0xC000) == 0xC000) { // bank
    reg[1] = (val & 0x0F);
    map_prg_rom_page_16k(reg[0] | reg[1], 2);
    result = true;
  }
  mn_rom rom = mn_rom_get();
  if ((rom->submapper == 0 && (addr & 0xF000) == 0x9000) || (rom->submapper == 1 && (addr & 0xE000) == 0x8000)) {
    map_discrete_common_mirror(MAP_MIRROR_SINGLE_LO_HI, val & 0x10);
    result = true;
  }
  return result;
}

static bool map_camerica232_cpu_write(u16 addr, u8 val)
{
  bool result = false;
  u8* reg = map_reg();
  if ((addr & 0xC000) == 0x8000) { // outer bank
    reg[0] = (val & 0x18) >> 1;
    map_prg_rom_page_16k(reg[0] | reg[1], 2);
    map_prg_rom_page_16k(reg[0] | 3, 3);
    result = true;
  }
  if ((addr & 0xC000) == 0xC000) { // bank
    reg[1] = (val & 0x03);
    map_prg_rom_page_16k(reg[0] | reg[1], 2);
    result = true;
  }
  return result;
}

static void map_camerica232_load()
{
  map_set_cpu_write_cb(map_camerica232_cpu_write);
  map_prg_rom_page_16k(0, 2);
  map_prg_rom_page_16k(3, 3);
}

static bool map_sunsoft089_cpu_write(u16 addr, u8 val)
{
  if (addr < 0x8000) {
    return false;
  }
  val = map_bus_conflict(addr, val);
  map_prg_rom_page_16k((val & 0x70) >> 4, 2);
  map_chr_page_8k(((val & 0x80) >> 4) | (val & 0x07), 0);
  map_discrete_common_mirror(MAP_MIRROR_SINGLE_LO_HI, val & 0x08);
  return true;
}

static bool map_sunsoft184_cpu_write(u16 addr, u8 val)
{
  if ((addr & 0xE000) != 0x6000) {
    return false;
  }
  map_chr_page_4k((val & 0x07), 0);
  map_chr_page_4k((val & 0x70) >> 4, 1);
  return true;
}

static bool map_113_cpu_write(u16 addr, u8 val)
{
  if ((addr & 0xE100) != 0x4100) {
    return false;
  }
  map_prg_rom_page_32k((val & 0x38) >> 3, 1);
  map_chr_page_8k(((val & 0x40) >> 3) | (val & 0x07), 0);
  map_discrete_common_mirror(MAP_MIRROR_HORIZ_VERT, val & 0x80);
  return true;
}

static bool map_046_cpu_write(u16 addr, u8 val)
{
  u8* reg = map_reg();
  if (addr >= 0x8000) {
    reg[0] = (val & 0x01);
    reg[2] = ((val & 0x70) >> 4);
    map_prg_rom_page_32k(reg[0] | reg[1], 1);
    map_chr_page_8k(reg[2] | reg[3], 0);
    return true;
  } else if (addr >= 0x6000) {
    reg[1] = ((val & 0x0F) << 1);
    reg[3] = ((val & 0xF0) >> 3);
    map_prg_rom_page_32k(reg[0] | reg[1], 1);
    map_chr_page_8k(reg[2] | reg[3], 0);
    return true;
  }
  return false;
}

static void map_unrom512_load()
{
  mn_rom rom = mn_rom_get();
  bool bus_conflict = (rom->submapper == 2 || (rom->submapper == 0 && !rom->prg_has_battery));
  u16 addr_mask = bus_conflict ? 0x8000 : 0xC000;
  bool single_screen_switchable = (rom->alt_mirror && !rom->vert_mirror);
  bool has_mirrorring = (single_screen_switchable || rom->submapper == 3);
  uint mirror_mask = !has_mirrorring ? 0 : 0x80;
  uint mirror_mode = !has_mirrorring     ? MAP_MIRROR_NONE
                   : rom->submapper == 3 ? MAP_MIRROR_HORIZ_VERT
                                         : MAP_MIRROR_SINGLE_LO_HI;
  map_discrete_common_load(addr_mask, addr_mask, 0x1F, 0, 0, 16, 0x60, 5, 0, 8, mirror_mask, mirror_mode, bus_conflict);
}

static bool map_jaleco_072_092_cpu_write(u16 addr, u8 val)
{
  if (addr < 0x8000) {
    return false;
  }
  val = map_bus_conflict(addr, val);
  u8* reg = map_reg();
  u8 last = reg[1];
  reg[1] = val;
  if (!(last & 0x80) && (val & 0x80)) {
    map_prg_rom_page_16k(val & 0x0F, mn_rom_get()->mapper == 72 ? 2 : 3);
  }
  if (!(last & 0x40) && (val & 0x40)) {
    map_chr_page_8k(val & 0x0F, 0);
  }
  return true;
}

static void map_077_load()
{
  map_chr_page_2k(0, 0);
  map_chr_page_2k(-1, 1);
  map_chr_page_2k(-2, 2);
  map_chr_page_2k(-3, 3);
  map_discrete_prg_chr(0x0F, 0, 0, 32, 0xF0, 4, 0, 2, 1);
}

bool map_discrete_load()
{
  mn_rom rom = mn_rom_get();
  switch (rom->mapper) {
      // clang-format off
    case 2: map_discrete_prg(0xFF, 0, 0, 16, rom->submapper != 1); break; // UxROM
    case 3: map_discrete_chr(0x0F, 0, 0, 8, rom->submapper != 1); break; // CxROM
    case 7: map_discrete_prg_mir(0x0F, 0, 0, 32, 0x10, MAP_MIRROR_SINGLE_LO_HI, rom->submapper == 2); break; // AxROM
    case 11: map_discrete_prg_chr(0x0F, 0, 0, 32, 0xF0, 4, 0, 8, 0); break; // Color Dreams, bus conflict?
    case 13: map_discrete_chr(0x03, 0, 1, 4, 1); break; // CPROM
    case 30: map_unrom512_load(); break; // UNROM-512, no flash impl
    case 34: map_set_cpu_write_cb(map_bnrom_nina_cpu_write); break; // BxROM, NINA-001-002
    case 38: map_discrete_common_load(0xF000, 0x7000, 0x03, 0, 0, 32, 0x0C, 2, 0, 8, 0, 0, 0); break; // GxROM-like
    case 46: map_set_cpu_write_cb(map_046_cpu_write); break;
    case 66: map_discrete_prg_chr(0xF0, 4, 0, 32, 0x0F, 0, 0, 8, 1); break; // GxROM
    case 70: map_discrete_prg_chr(0xF0, 4, 0, 16, 0x0F, 0, 0, 8, 1); break;
    case 71: map_set_cpu_write_cb(map_camerica071_cpu_write); break;
    case 72: map_set_cpu_write_cb(map_jaleco_072_092_cpu_write); break; // Jaleco JF-17, no audio impl
    case 77: map_077_load(); break;
    case 78: map_discrete_prg_chr_mir(0x07, 0, 0, 16, 0xF0, 4, 0, 8, 0x08, (rom->alt_mirror || rom->submapper == 3) ? MAP_MIRROR_HORIZ_VERT : MAP_MIRROR_SINGLE_LO_HI, 1); break;
    case 79: map_discrete_common_load(0xE100, 0x4100, 0x08, 3, 0, 32, 0x07, 0, 0, 8, 0, 0, 0); break; // NINA-003-006
    case 81: map_discrete_prg_chr(0x0C, 2, 0, 16, 0x03, 0, 0, 8, 0); break;
    case 87: map_set_cpu_write_cb(map_j87_cpu_write); break;
    case 89: map_set_cpu_write_cb(map_sunsoft089_cpu_write); break;
    case 92: map_set_cpu_write_cb(map_jaleco_072_092_cpu_write); break; // Jaleco JF-19, no audio impl
    case 93: map_discrete_prg(0x70, 4, 0, 16, 1); break; // Sunsoft-2
    case 94: map_discrete_prg(0x1C, 2, 0, 16, 1); break; // UxROM
    case 101: map_discrete_common_load(0xE000, 0x6000, 0, 0, 0, 0, 0xFF, 0, 0, 8, 0, 0, 0); break; // CxROM-like
    case 107: map_discrete_prg_chr(0xFE, 1, 0, 32, 0xFF, 0, 0, 8, 0); break; // GxROM-like
    case 113: map_set_cpu_write_cb(map_113_cpu_write); break;
    case 133: map_discrete_common_load(0xE100, 0x4100, 0x04, 2, 0, 32, 0x03, 0, 0, 8, 0, 0, 0); break; // GxROM-like
    case 140: map_discrete_common_load(0xE000, 0x6000, 0x30, 4, 0, 32, 0x0F, 0, 0, 8, 0, 0, 0); break; // GxROM-like
    case 144: map_discrete_prg_chr(0x0F, 0, 0, 32, 0xF0, 4, 0, 8, 1); break; // Color Dreams, with least bit bus conflict?
    case 145: map_discrete_common_load(0xE100, 0x4100, 0, 0, 0, 0, 0x80, 7, 0, 8, 0, 0, 0); break; // CxROM-like
    case 146: map_discrete_common_load(0xE100, 0x4100, 0x08, 3, 0, 32, 0x07, 0, 0, 8, 0, 0, 0); break; // Sachen 3015 and SA-016 same as NINA-003-006
    case 148: map_discrete_prg_chr(0x08, 3, 0, 32, 0x07, 0, 0, 8, 1); break; // GxROM-like
    case 149: map_discrete_chr(0x80, 7, 0, 8, 1); break; // CxROM-like
    case 152: map_discrete_prg_chr_mir(0x70, 4, 0, 16, 0x0F, 0, 0, 8, 0x80, MAP_MIRROR_SINGLE_LO_HI, 1); break;
    case 177: map_discrete_prg_mir(0x1F, 0, 0, 32, 0x20, MAP_MIRROR_VERT_HORIZ, 0); break; // BxROM
    case 180: map_discrete_prg(0xFF, 0, 1, 16, 1); break; // UxROM
    case 184: map_set_cpu_write_cb(map_sunsoft184_cpu_write); break;
    case 185: map_discrete_chr(0x0F, 0, 0, 8, 1); break; // CxROM, CS1/CS2 no impl
    case 232: map_camerica232_load(); break;
    case 240: map_discrete_common_load(0xE000, 0x4000, 0xF0, 4, 0, 32, 0x0F, 0, 0, 8, 0, 0, 0); break; // GxROM-like
    case 241: map_discrete_prg(0xFF, 0, 0, 32, 0); break; // BxROM, LPC Speech Chip no impl
    case 271: map_discrete_prg_chr_mir(0x30, 4, 0, 32, 0x0F, 0, 0, 8, 0x20, MAP_MIRROR_HORIZ_VERT, 0); break; // GxROM-like
    // clang-format on
    default: return false;
  }
  return true;
}
