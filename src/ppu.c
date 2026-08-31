#include "ppu.h"
#include "cpu.h"
#include "map.h"
#include "common.h"
#include <string.h>

// Smooth (FBX)
MN_CACHE_LINE static uint ppu_palette[64] = {
  0xFF6A6D6A, 0xFF001380, 0xFF1E008A, 0xFF39007A, 0xFF550056, 0xFF5A0018, 0xFF4F1000, 0xFF3D1C00,
  0xFF253200, 0xFF003D00, 0xFF004000, 0xFF003924, 0xFF002E55, 0xFF000000, 0xFF000000, 0xFF000000,
  0xFFB9BCB9, 0xFF1850C7, 0xFF4B30E3, 0xFF7322D6, 0xFF951FA9, 0xFF9D285C, 0xFF983700, 0xFF7F4C00,
  0xFF5E6400, 0xFF227700, 0xFF027E02, 0xFF007645, 0xFF006E8A, 0xFF000000, 0xFF000000, 0xFF000000,
  0xFFFFFFFF, 0xFF68A6FF, 0xFF8C9CFF, 0xFFB586FF, 0xFFD975FD, 0xFFE377B9, 0xFFE58D68, 0xFFD49D29,
  0xFFB3AF0C, 0xFF7BC211, 0xFF55CA47, 0xFF46CB81, 0xFF47C1C5, 0xFF4A4D4A, 0xFF000000, 0xFF000000,
  0xFFFFFFFF, 0xFFCCEAFF, 0xFFDDDEFF, 0xFFECDAFF, 0xFFF8D7FE, 0xFFFCD6F5, 0xFFFDDBCF, 0xFFF9E7B5,
  0xFFF1F0AA, 0xFFDAFAA9, 0xFFC9FFBC, 0xFFC3FBD7, 0xFFC4F6F6, 0xFFBEC1BE, 0xFF000000, 0xFF000000,
};

static const float ppu_tint_accent = 0.75f;
static const uint ppu_rw_delay = 4;
static const uint ppu_mask_delay = 2;

enum
{
  PPU_DOT_ZERO = 0,
  PPU_DOT_BEGIN = 1,
  PPU_DOT_SPRITE_EVAL_BEGIN = 65,
  PPU_DOT_END = 256,
  PPU_DOT_SWAP_X = 257,
  PPU_DOT_SWAP_Y_BEGIN = 280,
  PPU_DOT_SWAP_Y_END = 304,
  PPU_DOT_SPRITE_BEGIN = 257,
  PPU_DOT_SPRITE_END = 320,
  PPU_DOT_PREFETCH_BEGIN = 321,
  PPU_DOT_PREFETCH_END = 336,
  PPU_DOT_LAST = 340,
};

enum
{
  PPU_REG_CTRL = 0,
  PPU_REG_MASK = 1,
  PPU_REG_STATUS = 2,
  PPU_REG_OAMADDR = 3,
  PPU_REG_OAMDATA = 4,
  PPU_REG_SCROLL = 5,
  PPU_REG_ADDR = 6,
  PPU_REG_DATA = 7,
};

enum : u8
{
  PPU_CTRL_NAMETABLE_X = (1 << 0),
  PPU_CTRL_NAMETABLE_Y = (1 << 1),
  PPU_CTRL_INC_Y = (1 << 2),
  PPU_CTRL_SPRITE_NAMETABLE = (1 << 3),
  PPU_CTRL_BACK_NAMETABLE = (1 << 4),
  PPU_CTRL_SPRITE_SIZE = (1 << 5),
  PPU_CTRL_EXT = (1 << 6), // todo: ?
  PPU_CTRL_NMI = (1 << 7),
};

enum : u8
{
  PPU_MASK_GRAY = (1 << 0),
  PPU_MASK_SHOW_LEFT_BACK = (1 << 1),
  PPU_MASK_SHOW_LEFT_SPRITE = (1 << 2),
  PPU_MASK_BACK = (1 << 3),
  PPU_MASK_SPRITE = (1 << 4),
  PPU_MASK_RED = (1 << 5),
  PPU_MASK_GREEN = (1 << 6),
  PPU_MASK_BLUE = (1 << 7),
};

enum : u8
{
  PPU_STATUS_SPRITE_OVERFLOW = (1 << 5),
  PPU_STATUS_SPRITE0_HIT = (1 << 6),
  PPU_STATUS_VBLANK = (1 << 7),
};

enum : u8
{
  PPU_SPRITE_COLOR0 = (1 << 0),
  PPU_SPRITE_COLOR1 = (1 << 1),
  PPU_SPRITE_PRIORITY = (1 << 5),
  PPU_SPRITE_FLIP_HORIZ = (1 << 6),
  PPU_SPRITE_FLIP_VERT = (1 << 7),
};

MN_CACHE_LINE static uint ppu_out[256 * 240];

MN_CACHE_LINE static struct Ppu
{
  uint cyc;
  uint dot;
  uint sl;
  uint sprite_eval_count;
  uint sprite_render_count;
  uint sprite_copy;
  uint sprite_y;
  uint read_delay;
  uint write_delay;
  uint mask_delay;
  uint sl_end;
  uint sl_vblank;
  uint sl_pre_render;

  u16 addr;
  u16 t;
  u16 v;
  u16 nt;
  u16 shift_tile_lo;
  u16 shift_tile_hi;
  u16 shift_attr_lo;
  u16 shift_attr_hi;

  u8 ctrl;
  u8 mask;
  u8 status;

  u8 x;
  u8 at;
  u8 lo;
  u8 hi;

  u8 sprite_tile;

  u8 oam_addr1;
  u8 oam_addr2;
  u8 oam_data;
  u8 open_bus;
  u8 reg_bus;
  u8 read_buf;

  u8 pixel;
  u8 color;

  u8 sprite_attr[8];
  u8 sprite_x[8];
  u8 sprite_shift_lo[8];
  u8 sprite_shift_hi[8];

  u8 reg_bus_decay[8];

  bool render_enabled;
  bool write_latch;
  bool odd_frame;
  bool suppress_vblank;
  bool check_nmi;
  bool sprite_eval_first;
  bool sprite_eval_done;
  bool sprite_eval_has0;
  bool sprite_render_has0;
  bool sprite_flip_horiz;
  bool inc_x;
  bool ntsc;

  MN_CACHE_LINE u8 oam1[0x100];
  MN_CACHE_LINE u8 oam2[0x20];
  MN_CACHE_LINE u8 pam[0x20];
} ppu;

uint ppu_size() { return sizeof(ppu); }
void* ppu_data() { return &ppu; }

uint* mn_video_data() { return ppu_out; }
void mn_palette(uint palette[64]) { memcpy(ppu_palette, palette, sizeof(ppu_palette)); }

void ppu_power()
{
  memset(&ppu_out, 0, sizeof(ppu_out));
  memset(&ppu, 0, sizeof(ppu));
  switch (mn_region_get()) {
    case MN_REGION_NTSC:
      ppu.sl_end = 239;
      ppu.sl_vblank = 241;
      ppu.sl_pre_render = 261;
      ppu.ntsc = true;
      break;
    case MN_REGION_PAL:
      ppu.sl_end = 239;
      ppu.sl_vblank = 241;
      ppu.sl_pre_render = 311;
      break;
    case MN_REGION_DENDY:
      ppu.sl_end = 239;
      ppu.sl_vblank = 291;
      ppu.sl_pre_render = 311;
      break;
  }
}

void ppu_reset()
{
  ppu.ctrl = 0;
  ppu.mask = 0;
  ppu.status = 0;
  ppu.t = 0;
  ppu.x = 0;
  ppu.read_buf = 0;
  ppu.write_latch = false;
  ppu.odd_frame = false;
}

uint ppu_cyc() { return ppu.cyc; }
uint ppu_dot() { return ppu.dot; }
uint ppu_sl() { return ppu.sl; }
bool ppu_vblank() { return ppu.sl >= ppu.sl_vblank && ppu.sl < ppu.sl_pre_render; }

#if MN_TRACE_PPU
#define ppu_trace(fmt, ...)                                                                                     \
  tracef("PPU c=%02X m=%02X s=%02X v=%04X t=%04X x=%02X w=%d o=%02X sl=%-3d dot=%-3d cpu_cyc=%-10d  " fmt "\n", \
         ppu.ctrl, ppu.mask, ppu.status, ppu.v, ppu.t, ppu.x, ppu.write_latch, ppu.oam_data, ppu.sl, ppu.dot,   \
         cpu_cyc() __VA_OPT__(, ) __VA_ARGS__);
#else
#define trace_ppu(fmt, ...) (void)0
#endif

static u8 ppu_pam_addr(u16 addr) { return (addr & 0x03) == 0 ? (addr & 0x0F) : (addr & 0x1F); }
static u8 ppu_pam_read(u16 addr) { return ppu.pam[ppu_pam_addr(addr)]; }
static u8 ppu_pam_clamp(u8 val) { return val & ((ppu.mask & PPU_MASK_GRAY) ? 0x30 : 0x3F); }
static void ppu_pam_write(u16 addr, u8 val) { ppu.pam[ppu_pam_addr(addr)] = val; }

static void ppu_addr(u16 addr)
{
  addr &= 0x3FFF;
  ppu.addr = addr;
  map_ppu_addr(addr);
}

static u8 ppu_read()
{
  if (ppu.addr >= 0x3F00) {
    return ppu_pam_clamp(ppu_pam_read(ppu.addr));
  } else {
    map_ppu_read(ppu.addr, &ppu.open_bus);
    return ppu.open_bus;
  }
}

static void ppu_write(u8 val)
{
  ppu.open_bus = val;
  if (ppu.addr >= 0x3F00) {
    ppu_pam_write(ppu.addr, val);
  } else {
    map_ppu_write(ppu.addr, val);
  }
}

static u16 ppu_swap_x() { return (ppu.v & 0x7BE0) | (ppu.t & 0x041F); }
static u16 ppu_swap_y() { return (ppu.v & 0x041F) | (ppu.t & 0x7BE0); }

static u16 ppu_inc_xy(bool inc_y)
{
  uint v = ppu.v;
  if ((v & 0x001F) == 31) { // if coarse X == 31
    v &= ~0x001F; // coarse X = 0
    v ^= 0x0400; // switch horizontal nametable
  } else {
    v += 1; // increment coarse X
  }
  if (inc_y) {
    if ((v & 0x7000) != 0x7000) { // if fine Y < 7
      v += 0x1000; // increment fine Y
    } else {
      v &= ~0x7000; // fine Y = 0
      uint y = (v & 0x03E0) >> 5; // y = coarse Y
      if (y == 29) {
        y = 0;
        v ^= 0x0800; // switch vertical nametable
      } else if (y == 31) {
        y = 0;
      } else {
        y += 1;
      }
      v = (v & ~0x03E0) | (y << 5);
    }
  }
  return v;
}

static u16 ppu_inc_v() { return ppu.v + ((ppu.ctrl & PPU_CTRL_INC_Y) ? 32 : 1); }

static bool ppu_inc_m()
{
  ppu.oam_addr1 = (ppu.oam_addr1 & 0xFC) | ((ppu.oam_addr1 + 1) & 3);
  return (ppu.oam_addr1 & 3) == 0;
}

static bool ppu_inc_n()
{
  ppu.oam_addr1 += 4;
  return (ppu.oam_addr1 & 0xFC) == 0;
}

static void ppu_reg_bus_set(u8 val, u8 mask)
{
  val &= mask;
  ppu.reg_bus = val | (ppu.reg_bus & ~mask);
  for (uint i = 0; i < 8; ++i) {
    if (val & (1 << i)) {
      ppu.reg_bus_decay[i] = 32; // 600ms = 36 vblanks at 60Hz, 30 vblanks at 50Hz
    }
  }
}

static void ppu_reg_bus_decay()
{
  for (uint i = 0; i < 8; ++i) {
    if (ppu.reg_bus_decay[i] > 0) {
      if (--ppu.reg_bus_decay[i] == 0) {
        ppu.reg_bus &= ~(1 << i);
      }
    }
  }
}

u8 ppu_bus_read(u16 addr, bool trace)
{
  switch (addr & 7) {
    case PPU_REG_STATUS: {
      if (trace) {
        return ppu.status;
      }
      ppu_reg_bus_set(ppu.status, PPU_STATUS_SPRITE_OVERFLOW | PPU_STATUS_SPRITE0_HIT | PPU_STATUS_VBLANK);
      ppu.write_latch = false;
      ppu.suppress_vblank = true;
      ppu.check_nmi = true;
      ppu.status &= ~PPU_STATUS_VBLANK;
      break;
    }
    case PPU_REG_OAMDATA: {
      u8 val = ppu.oam_data;
      if (trace) {
        return val;
      }
      ppu_reg_bus_set(val, 0xFF);
      break;
    }
    case PPU_REG_DATA: {
      if ((ppu.v & 0x3FFF) < 0x3F00) {
        u8 val = ppu.read_buf;
        if (trace) {
          return val;
        }
        ppu_addr(ppu.v);
        ppu_reg_bus_set(val, 0xFF);
      } else {
        u8 val = ppu_pam_clamp(ppu_pam_read(ppu.v));
        if (trace) {
          return val;
        }
        ppu_addr(ppu.v & 0x2FFF);
        ppu_reg_bus_set(val, 0x3F);
      }
      if (ppu.read_delay == 0) {
        ppu.read_delay = ppu_rw_delay;
      }
      break;
    }
  }
  return ppu.reg_bus;
}

void ppu_bus_write(u16 addr, u8 val)
{
  ppu_reg_bus_set(val, 0xFF);
  switch (addr & 7) {
    case PPU_REG_CTRL:
      ppu.check_nmi = (ppu.ctrl & PPU_CTRL_NMI) != (val & PPU_CTRL_NMI);
      ppu.ctrl = val;
      ppu.t = (ppu.t & 0xF3FF) | ((val & 0x03) << 10);
      break;
    case PPU_REG_MASK:
      ppu.mask = val;
      ppu.mask_delay = ppu_mask_delay;
      break;
    case PPU_REG_OAMADDR:
      ppu.oam_addr1 = val;
      ppu.oam_data = ppu.oam1[ppu.oam_addr1];
      break;
    case PPU_REG_OAMDATA:
      if (ppu.render_enabled && (ppu.sl <= ppu.sl_end || ppu.sl == ppu.sl_pre_render) && ppu.dot >= PPU_DOT_BEGIN
          && ppu.dot <= PPU_DOT_SPRITE_END)
      {
        ppu_inc_n(); // NES bug: inc n on active sprite rendering
      } else {
        ppu.oam1[ppu.oam_addr1] = (ppu.oam_addr1 & 3) == 2 ? (val & 0xE3) : val;
        ++ppu.oam_addr1;
      }
      ppu.oam_data = ppu.oam1[ppu.oam_addr1];
      break;
    case PPU_REG_SCROLL:
      if (!ppu.write_latch) {
        ppu.t = (ppu.t & 0x7FE0) | ((val & 0xF8) >> 3);
        ppu.x = (val & 7);
      } else {
        ppu.t = (ppu.t & 0x0C1F) | ((val & 0xF8) << 2) | ((val & 7) << 12);
      }
      ppu.write_latch = !ppu.write_latch;
      break;
    case PPU_REG_ADDR:
      if (!ppu.write_latch) {
        ppu.t = (ppu.t & 0x00FF) | ((val & 0x3F) << 8);
      } else {
        ppu.t = (ppu.t & 0xFF00) | val;
        ppu.v = ppu.t;
        ppu_addr(ppu.v);
      }
      ppu.write_latch = !ppu.write_latch;
      break;
    case PPU_REG_DATA:
      ppu_addr(ppu.v);
      if (ppu.write_delay == 0) {
        ppu.write_delay = ppu_rw_delay;
      }
      break;
  }
}

static u16 ppu_nt_addr() { return 0x2000 | (ppu.v & 0x0FFF); }
static u16 ppu_at_addr() { return 0x23C0 | (ppu.v & 0x0C00) | ((ppu.v >> 4) & 0x38) | ((ppu.v >> 2) & 7); }

static uint pipe_step() { return (ppu.dot - PPU_DOT_BEGIN) % 8; }

static void ppu_fetch_back()
{
  ppu.shift_tile_lo <<= 1;
  ppu.shift_tile_hi <<= 1;
  ppu.shift_attr_lo <<= 1;
  ppu.shift_attr_hi <<= 1;

  switch (pipe_step()) {
    case 0: {
      ppu_addr(ppu_nt_addr());
      break;
    }
    case 1: { // NT
      ppu_addr(ppu_nt_addr());
      uint table = (ppu.ctrl & PPU_CTRL_BACK_NAMETABLE) ? 0x1000 : 0x0000;
      uint fine_y = (ppu.v & 0x7000) >> 12;
      ppu.nt = table | (ppu_read() << 4) | fine_y;
      break;
    }
    case 2: {
      ppu_addr(ppu_at_addr());
      break;
    }
    case 3: { // AT
      ppu_addr(ppu_at_addr());
      uint shift = ((ppu.v >> 4) & 4) | (ppu.v & 2);
      ppu.at = (ppu_read() >> shift) & 0x03;
      break;
    }
    case 4: {
      ppu_addr(ppu.nt);
      break;
    }
    case 5: { // chr lo
      ppu_addr(ppu.nt);
      ppu.lo = ppu_read();
      break;
    }
    case 6: {
      ppu_addr(ppu.nt | 8);
      break;
    }
    case 7: { // chr hi
      ppu_addr(ppu.nt | 8);
      ppu.hi = ppu_read();

      ppu.shift_tile_lo = (ppu.shift_tile_lo & 0xFF00) | ppu.lo;
      ppu.shift_tile_hi = (ppu.shift_tile_hi & 0xFF00) | ppu.hi;
      ppu.shift_attr_lo = (ppu.shift_attr_lo & 0xFF00) | ((ppu.at & 1) ? 0xFF : 0x00);
      ppu.shift_attr_hi = (ppu.shift_attr_hi & 0xFF00) | ((ppu.at & 2) ? 0xFF : 0x00);

      ppu.inc_x = true;
      break;
    }
  }
}

static void ppu_clear_sprites()
{
  if (ppu.dot & 1) {
    ppu.oam_data = 0xFF;
  } else {
    ppu.oam2[(ppu.dot - PPU_DOT_BEGIN) >> 1] = ppu.oam_data;
  }
}

static void ppu_evaluate_sprites()
{
  if (ppu.dot == PPU_DOT_SPRITE_EVAL_BEGIN) {
    ppu.oam_addr2 = 0;
    ppu.sprite_eval_count = 0;
    ppu.sprite_copy = 0;
    ppu.sprite_eval_has0 = false;
    ppu.sprite_eval_done = false;
    ppu.sprite_eval_first = true;
  }

  if (ppu.dot & 1) {
    ppu.oam_data = ppu.oam1[ppu.oam_addr1];
    return;
  }

  u8 y = ppu.oam_data;

  if (ppu.oam_addr2 < 0x20 && !ppu.sprite_eval_done) {
    ppu.oam2[ppu.oam_addr2] = ppu.oam_data;
  } else {
    ppu.oam_data = ppu.oam2[ppu.oam_addr2 & 0x1F];
  }

  if (ppu.sprite_copy > 0) {
    if (ppu.oam_addr2 < 0x20) {
      ++ppu.oam_addr2;
    }
    if (ppu_inc_m()) {
      if (ppu_inc_n()) {
        ppu.sprite_eval_done = true;
      }
    }
    if (--ppu.sprite_copy == 0) {
      ppu.oam_addr1 &= 0xFC; // todo: why reset m? (read2004.nes)
    }
    return;
  }

  if (!ppu.sprite_eval_done) {
    uint height = (ppu.ctrl & PPU_CTRL_SPRITE_SIZE) ? 16 : 8;
    bool is_sprite0 = ppu.sprite_eval_first; // NES bug: sprite 0 is first sprite pointed by oam_addr1, may be not 0
    ppu.sprite_eval_first = false;
    if (ppu.sl >= y && ppu.sl < (y + height)) {
      if (is_sprite0) {
        ppu.sprite_eval_has0 = true;
      }
      if (ppu.oam_addr2 < 0x20) {
        ++ppu.oam_addr2;
      } else {
        ppu.status |= PPU_STATUS_SPRITE_OVERFLOW;
      }
      ppu_inc_m();
      ppu.sprite_copy = 3;
      ++ppu.sprite_eval_count;
      return;
    } else if (ppu.sprite_eval_count == 8) {
      ppu_inc_m(); // NES bug: increment both m and n.
    }
  }

  if (ppu_inc_n()) {
    ppu.sprite_eval_done = true;
  }
}

static void ppu_fetch_sprites()
{
  if (ppu.dot == PPU_DOT_SPRITE_BEGIN) {
    ppu.oam_addr1 = 0;
    ppu.sprite_render_count = ppu.sprite_eval_count > 8 ? 8 : ppu.sprite_eval_count;
    ppu.sprite_render_has0 = ppu.sprite_eval_has0;
  }

  uint i = (ppu.dot - PPU_DOT_SPRITE_BEGIN) / 8;

  switch (pipe_step()) {
    case 0: {
      ppu_addr(ppu_nt_addr());
      ppu.oam_data = ppu.oam2[i * 4 + 0];
      uint next_sl = (ppu.sl == ppu.sl_pre_render) ? 0 : (ppu.sl + 1);
      ppu.sprite_y = next_sl - ppu.oam_data - 1;
      break;
    }
    case 1: {
      ppu_addr(ppu_nt_addr());
      ppu_read(); // unused NT
      ppu.oam_data = ppu.oam2[i * 4 + 1];
      ppu.sprite_tile = ppu.oam_data;
      if (ppu.ctrl & PPU_CTRL_SPRITE_SIZE) {
        ppu.nt = (ppu.sprite_tile & 1) ? 0x1000 : 0x0000;
      } else {
        ppu.nt = (ppu.ctrl & PPU_CTRL_SPRITE_NAMETABLE) ? 0x1000 : 0x0000;
      }
      break;
    }
    case 2: {
      ppu_addr(ppu_nt_addr());
      ppu.oam_data = ppu.oam2[i * 4 + 2];
      ppu.sprite_attr[i] = ppu.oam_data;
      ppu.sprite_flip_horiz = (ppu.oam_data & PPU_SPRITE_FLIP_HORIZ);
      if (ppu.oam_data & PPU_SPRITE_FLIP_VERT) {
        uint height = (ppu.ctrl & PPU_CTRL_SPRITE_SIZE) ? 16 : 8;
        ppu.sprite_y = (height - 1) - ppu.sprite_y;
      }
      if (i < ppu.sprite_render_count) {
        if (ppu.ctrl & PPU_CTRL_SPRITE_SIZE) {
          ppu.nt |= ((ppu.sprite_tile & 0xFE) << 4) + (ppu.sprite_y >= 8 ? ppu.sprite_y + 8 : ppu.sprite_y);
        } else {
          ppu.nt |= (ppu.sprite_tile << 4) + ppu.sprite_y;
        }
      } else {
        ppu.nt |= (0xFF << 4);
      }
      break;
    }
    case 3: {
      ppu_addr(ppu_nt_addr());
      ppu_read(); // ignored NT
      ppu.oam_data = ppu.oam2[i * 4 + 3];
      ppu.sprite_x[i] = ppu.oam_data;
      break;
    }
    case 4: {
      ppu_addr(ppu.nt);
      break;
    }
    case 5: {
      ppu_addr(ppu.nt);
      ppu.lo = ppu_read();
      break;
    }
    case 6: {
      ppu_addr(ppu.nt | 8);
      break;
    }
    case 7: {
      ppu_addr(ppu.nt | 8);
      ppu.hi = ppu_read();

      if (i < ppu.sprite_render_count) {
        if (ppu.sprite_flip_horiz) {
          ppu.lo = ((ppu.lo & 0xF0) >> 4) | ((ppu.lo & 0x0F) << 4);
          ppu.lo = ((ppu.lo & 0xCC) >> 2) | ((ppu.lo & 0x33) << 2);
          ppu.lo = ((ppu.lo & 0xAA) >> 1) | ((ppu.lo & 0x55) << 1);
          ppu.hi = ((ppu.hi & 0xF0) >> 4) | ((ppu.hi & 0x0F) << 4);
          ppu.hi = ((ppu.hi & 0xCC) >> 2) | ((ppu.hi & 0x33) << 2);
          ppu.hi = ((ppu.hi & 0xAA) >> 1) | ((ppu.hi & 0x55) << 1);
        }
        ppu.sprite_shift_lo[i] = ppu.lo;
        ppu.sprite_shift_hi[i] = ppu.hi;
      } else {
        ppu.sprite_shift_lo[i] = 0;
        ppu.sprite_shift_hi[i] = 0;
      }
      break;
    }
  }
}

static void ppu_fetch_unused()
{
  switch (pipe_step()) {
    case 0: { // 337
      ppu_addr(ppu_nt_addr());
      break;
    }
    case 1: { // 338
      ppu_addr(ppu_nt_addr());
      ppu.nt = ppu_read(); // unused NT
      break;
    }
    case 2: { // 339
      ppu_addr(ppu_nt_addr());
      break;
    }
    case 3: { // 340
      ppu_addr(ppu_nt_addr());
      ppu_read(); // ignored NT
      break;
    }
      // case 4: { // never called because delay 340->0
      //   ppu_addr(ppu.nt); // lo byte only?
      //   break;
      // }
  }
}

static void ppu_fetch()
{
  if (ppu.render_enabled && (ppu.sl <= ppu.sl_end || ppu.sl == ppu.sl_pre_render)) {
    if ((ppu.dot >= PPU_DOT_BEGIN && ppu.dot < PPU_DOT_SPRITE_EVAL_BEGIN)) {
      ppu_clear_sprites();
    } else if (ppu.dot >= PPU_DOT_SPRITE_EVAL_BEGIN && ppu.dot <= PPU_DOT_END) {
      if (ppu.sl != ppu.sl_pre_render) {
        ppu_evaluate_sprites();
      }
    } else if (ppu.dot >= PPU_DOT_SPRITE_BEGIN && ppu.dot <= PPU_DOT_SPRITE_END) {
      ppu_fetch_sprites();
    }
    if ((ppu.dot >= PPU_DOT_BEGIN && ppu.dot <= PPU_DOT_END)
        || (ppu.dot >= PPU_DOT_PREFETCH_BEGIN && ppu.dot <= PPU_DOT_PREFETCH_END))
    {
      ppu_fetch_back();
    } else if (ppu.dot > PPU_DOT_PREFETCH_END) {
      ppu_fetch_unused();
    }
  }
  if (ppu.dot == PPU_DOT_PREFETCH_BEGIN) {
    ppu.oam_data = ppu.oam1[ppu.oam_addr1]; // restore oam data from oam2 to oam1
  }
}

static void ppu_render_pixel(uint x)
{
  u8 pixel_back = 0;
  if ((ppu.mask & PPU_MASK_BACK) && (x >= 8 || (ppu.mask & PPU_MASK_SHOW_LEFT_BACK))) {
    uint bit_mask = 0x8000 >> ppu.x;
    uint p0 = (ppu.shift_tile_lo & bit_mask);
    uint p1 = (ppu.shift_tile_hi & bit_mask);
    if (p0 | p1) {
      uint a0 = (ppu.shift_attr_lo & bit_mask);
      uint a1 = (ppu.shift_attr_hi & bit_mask);
      pixel_back = (p0 > 0) | ((p1 > 0) << 1) | ((a0 > 0) << 2) | ((a1 > 0) << 3);
    }
  }

  u8 pixel_sprite = 0;
  bool sprite_priority = false;
  bool sprite0_hit = false;
  if ((ppu.mask & PPU_MASK_SPRITE) && (x >= 8 || (ppu.mask & PPU_MASK_SHOW_LEFT_SPRITE))) {
    for (uint i = 0; i < ppu.sprite_render_count; ++i) {
      uint offset = x - ppu.sprite_x[i];
      if (offset < 8) {
        uint bit_mask = 0x80 >> offset;
        uint p0 = (ppu.sprite_shift_lo[i] & bit_mask);
        uint p1 = (ppu.sprite_shift_hi[i] & bit_mask);
        if (p0 | p1) {
          pixel_sprite = (p0 > 0) | ((p1 > 0) << 1) | 0x10 | ((ppu.sprite_attr[i] & 0x03) << 2);
          sprite_priority = (ppu.sprite_attr[i] & PPU_SPRITE_PRIORITY) == 0;
          sprite0_hit = (i == 0 && ppu.sprite_render_has0);
          break;
        }
      }
    }
  }

  if (pixel_back && pixel_sprite && sprite0_hit && x < 255) { // NES bug: hit not set on x=255
    ppu.status |= PPU_STATUS_SPRITE0_HIT;
  }

  if (pixel_sprite && (pixel_back == 0 || sprite_priority)) {
    ppu.pixel = pixel_sprite;
  } else {
    ppu.pixel = pixel_back;
  }
}

static void ppu_render_palette()
{
  u8 pixel = ppu.render_enabled ? ppu.pixel : ((ppu.v & 0x3FFF) >= 0x3F00 ? ppu.v : 0);
  ppu.color = ppu_pam_read(pixel);
}

static void ppu_render_mask(uint x)
{
  ppu.color = ppu_pam_clamp(ppu.color);

  uint rgb = ppu_palette[ppu.color];

  if ((ppu.mask & PPU_MASK_RED) || (ppu.mask & PPU_MASK_GREEN) || (ppu.mask & PPU_MASK_BLUE)) {
    float rf = 1.0f;
    float gf = 1.0f;
    float bf = 1.0f;
    if (ppu.ntsc) {
      if (ppu.mask & PPU_MASK_RED) {
        gf *= ppu_tint_accent;
        bf *= ppu_tint_accent;
      }
      if (ppu.mask & PPU_MASK_GREEN) {
        rf *= ppu_tint_accent;
        bf *= ppu_tint_accent;
      }
    } else {
      if (ppu.mask & PPU_MASK_GREEN) {
        gf *= ppu_tint_accent;
        bf *= ppu_tint_accent;
      }
      if (ppu.mask & PPU_MASK_RED) {
        rf *= ppu_tint_accent;
        bf *= ppu_tint_accent;
      }
    }
    // todo: (Dendy) blue emphasis delay
    if (ppu.mask & PPU_MASK_BLUE) {
      rf *= ppu_tint_accent;
      gf *= ppu_tint_accent;
    }
    uint r = (rgb & 0xFF0000) >> 16;
    uint g = (rgb & 0x00FF00) >> 8;
    uint b = (rgb & 0x0000FF);
    r *= rf;
    g *= gf;
    b *= bf;
    rgb = 0xFF000000 | (r << 16) | (g << 8) | b;
  }

  uint out_idx = (ppu.sl << 8) | x;
  ppu_out[out_idx] = rgb;
}

static void ppu_render()
{
  if (ppu.sl <= ppu.sl_end) {
    if (ppu.dot >= PPU_DOT_BEGIN + 2 && ppu.dot <= PPU_DOT_END + 2) {
      ppu_render_mask(ppu.dot - PPU_DOT_BEGIN - 2);
    }
    if (ppu.dot >= PPU_DOT_BEGIN + 1 && ppu.dot <= PPU_DOT_END + 1) {
      ppu_render_palette();
    }
    if (ppu.dot >= PPU_DOT_BEGIN && ppu.dot <= PPU_DOT_END) {
      ppu_render_pixel(ppu.dot - PPU_DOT_BEGIN);
    }
  }
}

void ppu_tick()
{
  ppu_render();
  ppu_fetch();

  if (ppu.sl == ppu.sl_pre_render && ppu.dot == PPU_DOT_ZERO) {
    ppu.status &= ~(PPU_STATUS_SPRITE0_HIT | PPU_STATUS_SPRITE_OVERFLOW);
    ppu_reg_bus_decay();
  }

  if (ppu.sl == ppu.sl_pre_render && ppu.dot == PPU_DOT_BEGIN) {
    ppu.status &= ~PPU_STATUS_VBLANK;
  }

  if ((ppu.sl == ppu.sl_vblank && ppu.dot == PPU_DOT_BEGIN) && !ppu.suppress_vblank) {
    ppu.status |= PPU_STATUS_VBLANK;
    ppu.check_nmi = true;
  }

  if (ppu.check_nmi) {
    cpu_nmi(!ppu.suppress_vblank && (ppu.status & PPU_STATUS_VBLANK) && (ppu.ctrl & PPU_CTRL_NMI));
  }

  bool inc_v = false;
  if (ppu.read_delay > 0) {
    if (--ppu.read_delay == 0) {
      ppu.read_buf = ppu_read();
      inc_v = true;
    }
  }

  if (ppu.write_delay > 0) {
    if (--ppu.write_delay == 0) {
      ppu_write(ppu.reg_bus);
      inc_v = true;
    }
  }

  bool v_changed = false;
  uint v_inc = 0xFFFF;
  uint v_swap = 0xFFFF;
  if (ppu.render_enabled) {
    if ((ppu.sl <= ppu.sl_end || ppu.sl == ppu.sl_pre_render) && ppu.dot == PPU_DOT_SWAP_X) {
      v_swap = ppu_swap_x();
      v_changed = true;
    }
    if (ppu.sl == ppu.sl_pre_render && ppu.dot >= PPU_DOT_SWAP_Y_BEGIN && ppu.dot <= PPU_DOT_SWAP_Y_END) {
      v_swap = ppu_swap_y();
      v_changed = true;
    }
  }

  if (ppu.inc_x) {
    v_inc = ppu_inc_xy(inc_v || ppu.dot == PPU_DOT_END);
    v_changed = true;
  } else if (inc_v) {
    bool active_rendering = (ppu.render_enabled && (ppu.sl <= ppu.sl_end || ppu.sl == ppu.sl_pre_render));
    v_inc = active_rendering ? ppu_inc_xy(true) : ppu_inc_v();
    v_changed = true;
  }
  if (v_changed) {
    ppu.v = (v_inc & v_swap);
  }
  if (inc_v) {
    ppu_addr(ppu.v);
  }

  ppu.inc_x = false;
  ppu.suppress_vblank = false;
  ppu.check_nmi = false;

  if (ppu.mask_delay > 0) {
    if (--ppu.mask_delay == 0) {
      bool was_enabled = ppu.render_enabled;
      ppu.render_enabled = (ppu.mask & PPU_MASK_BACK) || (ppu.mask & PPU_MASK_SPRITE);
      if (was_enabled && !ppu.render_enabled) {
        ppu.oam_data = ppu.oam1[ppu.oam_addr1]; // restore oam data from oam2 to oam1
      }
    }
  }

  if (ppu.ntsc && ppu.odd_frame && ppu.sl == ppu.sl_pre_render && ppu.dot == PPU_DOT_LAST - 1 && ppu.render_enabled) {
    ++ppu.dot;
  }

  if (++ppu.dot > PPU_DOT_LAST) {
    ppu.dot = 0;
    if (++ppu.sl > ppu.sl_pre_render) {
      ppu.sl = 0;
      ppu.odd_frame = !ppu.odd_frame;
    }
  }
  ++ppu.cyc;
}
