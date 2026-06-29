#include "ppu.h"
#include "cpu.h"
#include "dev.h"
#include "map.h"
#include "common.h"
#include <string.h>

// clang-format off
static const u32 palette[0x40] = {
  0x7C7C7C, 0x0000FC, 0x0000BC, 0x4428BC, 0x940084, 0xA80020, 0xA81000, 0x881400, 0x503000, 0x007800, 0x006800, 0x005800, 0x004058, 0x000000, 0x000000, 0x000000, //
  0xBCBCBC, 0x0078F8, 0x0058F8, 0x6844FC, 0xD800CC, 0xE40058, 0xF83800, 0xE45C10, 0xAC7C00, 0x00B800, 0x00A800, 0x00A844, 0x008888, 0x000000, 0x000000, 0x000000, //
  0xF8F8F8, 0x3CBCFC, 0x6888FC, 0x9878F8, 0xF878F8, 0xF85898, 0xF87858, 0xFCA044, 0xF8B800, 0xB8F818, 0x58D854, 0x58F898, 0x00E8D8, 0x787878, 0x000000, 0x000000, //
  0xFFFFFF, 0xA4E4FC, 0xB8B8F8, 0xD8B8F8, 0xF8B8F8, 0xF8A4C0, 0xF0D0B0, 0xFCE0A8, 0xF8D878, 0xD8F878, 0xB8F8B8, 0xB8F8D8, 0x00FCFC, 0xD8D8D8, 0x000000, 0x000000, //
};
// clang-format on

static const float ppu_tint_accent = 0.75f;
static const u8 ppu_rw_delay = 4;

enum : u16
{
  PPU_SL_END = 239,
  PPU_SL_POST_RENDER = 240,
  PPU_SL_VBLANK = 241,
  PPU_SL_PRE_RENDER = 261,
};

enum : u16
{
  PPU_CYC_ZERO = 0,
  PPU_CYC_BEGIN = 1,
  PPU_CYC_END = 256,
  PPU_CYC_SWAP_X = 257,
  PPU_CYC_SWAP_Y_BEGIN = 280,
  PPU_CYC_SWAP_Y_END = 304,
  PPU_CYC_SPRITE_BEGIN = 257,
  PPU_CYC_SPRITE_END = 320,
  PPU_CYC_PREFETCH_BEGIN = 321,
  PPU_CYC_PREFETCH_END = 336,
  PPU_CYC_LAST = 340,
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

typedef struct
{
  u8 y;
  u8 tile;
  u8 attr;
  u8 x;
} PpuSprite;

static struct Ppu
{
  u8 oam[0x100];
  u8 pam[0x20];

  u16 cyc;
  u16 sl;

  u16 addr;

  u8 ctrl;
  u8 mask;
  u8 status;

  u16 t;
  u16 v;
  u8 x;
  u16 nm;
  u16 nt;
  u8 at;
  u8 lo;
  u8 hi;

  u16 shift_tile_lo;
  u16 shift_tile_hi;
  u16 shift_attr_lo;
  u16 shift_attr_hi;

  PpuSprite sprite[2][8];
  PpuSprite* sprite_eval;
  PpuSprite* sprite_render;
  u8 sprite_eval_count;
  u8 sprite_render_count;
  u8 sprite_busy;
  u8 sprite_shift_lo[8];
  u8 sprite_shift_hi[8];

  u8 open_bus;
  u8 open_bus_decay[8];

  u8 read_buf;
  u8 write_buf;
  u8 read_cyc;
  u8 write_cyc;

  u8 pixel;
  u8 color;

  bool write_latch;
  bool odd_frame;
  bool increment_xy;
  bool suppress_vblank;
  bool check_nmi;
  bool sprite_eval_has0;
  bool sprite_render_has0;
} ppu;

void ppu_power() { memset(&ppu, 0, sizeof(ppu)); }

void ppu_reset()
{
  ppu.ctrl = 0;
  ppu.mask = 0;
  ppu.status = 0;
  ppu.t = 0;
  ppu.x = 0;
  ppu.read_buf = 0;
  ppu.write_buf = 0;
  ppu.write_latch = false;
  ppu.odd_frame = false;
}

u16 ppu_cyc() { return ppu.cyc; }
u16 ppu_sl() { return ppu.sl; }
bool ppu_is_vblank() { return ppu.status & PPU_STATUS_VBLANK; }

#if MN_TRACE_PPU
#define trace_ppu(fmt, ...)                                                                                 \
  tracef("PPU  " fmt "  C:%02X M:%02X S:%02X O:%02X V:%04X T:%04X X:%02X W:%d R:%02X PPU:%3d,%3d CYC:%d\n", \
         __VA_ARGS__, ppu.ctrl, ppu.mask, ppu.status, ppu.oam_addr, ppu.v, ppu.t, ppu.x, ppu.write_latch,   \
         ppu.read_buf, ppu.sl, ppu.cyc, cpu_cyc());
#else
#define trace_ppu(fmt, ...) (void)0
#endif

static u8 ppu_pam_addr(u16 addr) { return (addr & 0x03) == 0 ? (addr & 0x0F) : (addr & 0x1F); }

static void ppu_addr(u16 addr)
{
  addr &= 0x3FFF;
  ppu.addr = addr;
  map_ppu_addr(addr);
}

static u8 ppu_read()
{
  if (ppu.addr >= 0x3F00) {
    return ppu.pam[ppu_pam_addr(ppu.addr)];
  } else {
    u8 val = ppu.open_bus;
    map_ppu_read(ppu.addr, &val); // update open_bus? (ppu_open_bus.nes)
    return val;
  }
}

static void ppu_write(u8 val)
{
  if (ppu.addr >= 0x3F00) {
    ppu.pam[ppu_pam_addr(ppu.addr)] = val;
  } else {
    map_ppu_write(ppu.addr, val);
  }
}

static bool ppu_rendering_enabled() { return (ppu.mask & PPU_MASK_BACK) || (ppu.mask & PPU_MASK_SPRITE); }

static bool ppu_is_rendering()
{
  return ppu_rendering_enabled() && (ppu.sl <= PPU_SL_END || ppu.sl == PPU_SL_PRE_RENDER);
}

static void ppu_swap_x() { ppu.v = (ppu.v & 0x7BE0) | (ppu.t & 0x041F); }
static void ppu_swap_y() { ppu.v = (ppu.v & 0x041F) | (ppu.t & 0x7BE0); }

static void ppu_inc_x()
{
  if ((ppu.v & 0x001F) == 31) { // if coarse X == 31
    ppu.v &= ~0x001F; // coarse X = 0
    ppu.v ^= 0x0400; // switch horizontal nametable
  } else {
    ppu.v += 1; // increment coarse X
  }
}

static void ppu_inc_y()
{
  if ((ppu.v & 0x7000) != 0x7000) { // if fine Y < 7
    ppu.v += 0x1000; // increment fine Y
  } else {
    ppu.v &= ~0x7000; // fine Y = 0
    u16 y = (ppu.v & 0x03E0) >> 5; // y = coarse Y
    if (y == 29) {
      y = 0;
      ppu.v ^= 0x0800; // switch vertical nametable
    } else if (y == 31) {
      y = 0;
    } else {
      y += 1;
    }
    ppu.v = (ppu.v & ~0x03E0) | (y << 5);
  }
}

static void ppu_inc()
{
  if (ppu_is_rendering()) {
    ppu.increment_xy = true;
  } else {
    ppu.v += (ppu.ctrl & PPU_CTRL_INC_Y) ? 32 : 1;
    ppu_addr(ppu.v);
  }
}

static void ppu_open_bus_set(u8 val, u8 mask)
{
  val &= mask;
  ppu.open_bus = val | (ppu.open_bus & ~mask);
  for (u8 i = 0; i <= 7; ++i) {
    if (val & (1 << i)) {
      ppu.open_bus_decay[i] = 32; // 600ms = 36 vblanks at 60Hz, 30 vblanks at 50Hz
    }
  }
}

static void ppu_open_bus_decay()
{
  for (u8 i = 0; i < 8; ++i) {
    if (ppu.open_bus_decay[i] > 0) {
      if (--ppu.open_bus_decay[i] == 0) {
        ppu.open_bus &= ~(1 << i);
      }
    }
  }
}

u8 ppu_bus_read(u16 addr, bool trace)
{
  switch (addr & 0x7) {
    case 2: {
      if (trace) {
        return ppu.status;
      }
      ppu_open_bus_set(ppu.status, 0xE0);
      ppu.write_latch = false;
      ppu.suppress_vblank = true;
      ppu.check_nmi = true;
      ppu.status &= ~PPU_STATUS_VBLANK;
      break;
    }
    case 4: {
      u8 val = ppu.oam[ppu.nm & 0xFF];
      if (trace) {
        return val;
      }
      ppu_open_bus_set(val, 0xFF);
      break;
    }
    case 7: {
      if ((ppu.v & 0x3FFF) < 0x3F00) {
        u8 val = ppu.read_buf;
        if (trace) {
          return val;
        }
        ppu_open_bus_set(val, 0xFF);
      } else {
        u8 val = ppu_read();
        if (trace) {
          return val;
        }
        ppu.addr = ppu.v & 0x2FFF;
        ppu_open_bus_set(val, 0x3F);
      }
      if (ppu.read_cyc == 0) {
        ppu.read_cyc = ppu_rw_delay;
      }
      break;
    }
  }
  return ppu.open_bus;
}

void ppu_bus_write(u16 addr, u8 val)
{
  ppu_open_bus_set(val, 0xFF);
  switch (addr & 0x7) {
    case 0:
      ppu.check_nmi = (ppu.ctrl & PPU_CTRL_NMI) != (val & PPU_CTRL_NMI);
      ppu.ctrl = val;
      ppu.t = (ppu.t & 0xF3FF) | (((u16)val & 0x03) << 10);
      break;
    case 1: ppu.mask = val; break;
    case 3: ppu.nm = val; break;
    case 4:
      ppu.oam[ppu.nm & 0xFF] = (ppu.nm & 3) == 2 ? (val & 0xE3) : val;
      ++ppu.nm;
      break;
    case 5:
      if (!ppu.write_latch) {
        ppu.t = (ppu.t & 0x7FE0) | (((u16)val & 0xF8) >> 3);
        ppu.x = (val & 7);
      } else {
        ppu.t = (ppu.t & 0x0C1F) | (((u16)val & 0xF8) << 2) | (((u16)val & 7) << 12);
      }
      ppu.write_latch = !ppu.write_latch;
      break;
    case 6:
      if (!ppu.write_latch) {
        ppu.t = (ppu.t & 0x00FF) | (((u16)val & 0x3F) << 8);
      } else {
        ppu.t = (ppu.t & 0xFF00) | val;
        ppu.v = ppu.t;
        ppu_addr(ppu.v);
      }
      ppu.write_latch = !ppu.write_latch;
      break;
    case 7:
      ppu.write_buf = val;
      if (ppu.write_cyc == 0) {
        ppu.write_cyc = ppu_rw_delay;
      }
      break;
  }
}

static u16 ppu_nt_addr() { return 0x2000 | (ppu.v & 0x0FFF); }
static u16 ppu_at_addr() { return 0x23C0 | (ppu.v & 0x0C00) | ((ppu.v >> 4) & 0x38) | ((ppu.v >> 2) & 7); }

static bool ppu_fetch_back()
{
  ppu.shift_tile_lo <<= 1;
  ppu.shift_tile_hi <<= 1;
  ppu.shift_attr_lo <<= 1;
  ppu.shift_attr_hi <<= 1;

  switch (ppu.cyc % 8) {
    case 1: {
      ppu_addr(ppu_nt_addr());
      return false;
    }
    case 2: { // nametable
      ppu_addr(ppu_nt_addr());
      u16 table = (ppu.ctrl & PPU_CTRL_BACK_NAMETABLE) ? 0x1000 : 0x0000;
      u8 fine_y = (ppu.v & 0x7000) >> 12;
      ppu.nt = table | (ppu_read() << 4) | fine_y;
      return false;
    }
    case 3: {
      ppu_addr(ppu_at_addr());
      return false;
    }
    case 4: { // attribute
      ppu_addr(ppu_at_addr());
      u8 shift = ((ppu.v >> 4) & 4) | (ppu.v & 2);
      ppu.at = (ppu_read() >> shift) & 0x03;
      return false;
    }
    case 5: {
      ppu_addr(ppu.nt);
      return false;
    }
    case 6: { // chr lo
      ppu_addr(ppu.nt);
      ppu.lo = ppu_read();
      return false;
    }
    case 7: {
      ppu_addr(ppu.nt | 8);
      return false;
    }
    case 0: { // chr hi
      ppu_addr(ppu.nt | 8);
      ppu.hi = ppu_read();

      ppu.shift_tile_lo = (ppu.shift_tile_lo & 0xFF00) | ppu.lo;
      ppu.shift_tile_hi = (ppu.shift_tile_hi & 0xFF00) | ppu.hi;
      ppu.shift_attr_lo = (ppu.shift_attr_lo & 0xFF00) | ((ppu.at & 1) ? 0xFF : 0x00);
      ppu.shift_attr_hi = (ppu.shift_attr_hi & 0xFF00) | ((ppu.at & 2) ? 0xFF : 0x00);
      return true;
    }
    default: return false;
  }
}

static void ppu_evaluate_sprite_swap()
{
  ppu.sprite_render = ppu.sprite_eval;
  ppu.sprite_render_count = ppu.sprite_eval_count;
  ppu.sprite_render_has0 = ppu.sprite_eval_has0;
  memset(ppu.sprite_shift_lo, 0, sizeof(ppu.sprite_shift_lo));
  memset(ppu.sprite_shift_hi, 0, sizeof(ppu.sprite_shift_hi));
}

static void ppu_evaluate_sprites()
{
  if (ppu.sprite_busy) {
    --ppu.sprite_busy;
    return;
  }

  if (ppu.cyc == PPU_CYC_BEGIN) {
    ppu.sprite_eval = ppu.sprite[(ppu.sl & 1) ? 1 : 0];
    ppu.sprite_eval_count = 0;
    ppu.sprite_eval_has0 = false;
    ppu.sprite_busy = 64;
    memset(ppu.sprite_eval, 0xFF, sizeof(ppu.sprite[0]));
    return;
  }

  if (ppu.nm > 0xFF) {
    return;
  }

  u16 next_sl = ppu.sl == PPU_SL_PRE_RENDER ? 0 : (ppu.sl + 1);
  u8 height = (ppu.ctrl & PPU_CTRL_SPRITE_SIZE) ? 16 : 8;
  u8* oam_ptr = &ppu.oam[ppu.nm];
  u8 y = *oam_ptr;
  bool hit = next_sl > y && next_sl <= (y + height);

  if (ppu.sprite_eval_count < 8) {
    if (hit) {
      memcpy(&ppu.sprite_eval[ppu.sprite_eval_count++], oam_ptr, 4);
      ppu.sprite_busy += 6;
      if (oam_ptr == ppu.oam) {
        ppu.sprite_eval_has0 = true;
      }
    }
  } else {
    if (hit) {
      ppu.status |= PPU_STATUS_SPRITE_OVERFLOW;
    }
    ppu.nm = (ppu.nm & 0xFFFC) | ((ppu.nm + 1) & 3); // NES bug: increment both m and n: m here, n below.
  }
  ppu.nm += 4;
  ++ppu.sprite_busy;
}

static void ppu_fetch_sprites()
{
  ppu.nm = 0;

  u8 i = (ppu.cyc - PPU_CYC_SPRITE_BEGIN) / 8;
  PpuSprite* sprite = i < ppu.sprite_render_count ? &ppu.sprite_render[i] : 0;

  switch (ppu.cyc % 8) {
    case 1: {
      ppu_addr(ppu_nt_addr());
      break;
    }
    case 2: {
      ppu_addr(ppu_nt_addr());
      ppu.nt = ppu_read(); // unused NT

      if (sprite) {
        u16 next_sl = (ppu.sl == PPU_SL_PRE_RENDER) ? 0 : (ppu.sl + 1);
        u8 sprite_height = (ppu.ctrl & PPU_CTRL_SPRITE_SIZE) ? 16 : 8;

        u16 row = next_sl - sprite->y - 1;
        if (sprite->attr & PPU_SPRITE_FLIP_VERT) {
          row = (sprite_height - 1) - row;
        }

        if (!(ppu.ctrl & PPU_CTRL_SPRITE_SIZE)) { // 8x8
          u16 table = (ppu.ctrl & PPU_CTRL_SPRITE_NAMETABLE) ? 0x1000 : 0x0000;
          ppu.nt = table + (sprite->tile << 4) + row;
        } else {
          u16 table = (sprite->tile & 1) ? 0x1000 : 0x0000;
          ppu.nt = table + ((sprite->tile & 0xFE) << 4) + (row >= 8 ? row + 8 : row);
        }
      } else {
        u16 table = (ppu.ctrl & PPU_CTRL_SPRITE_NAMETABLE) ? 0x1000 : 0x0000;
        ppu.nt = table + (0xFF << 4);
      }
      break;
    }
    case 3: {
      ppu_addr(ppu_nt_addr());
      break;
    }
    case 4: {
      ppu_addr(ppu_nt_addr());
      ppu_read(); // ignored NT
      break;
    }
    case 5: {
      ppu_addr(ppu.nt);
      break;
    }
    case 6: {
      ppu_addr(ppu.nt);
      ppu.lo = ppu_read();
      break;
    }
    case 7: {
      ppu_addr(ppu.nt | 8);
      break;
    }
    case 0: {
      ppu_addr(ppu.nt | 8);
      ppu.hi = ppu_read();

      if (sprite) {
        if (sprite->attr & PPU_SPRITE_FLIP_HORIZ) {
          ppu.lo = ((ppu.lo & 0xF0) >> 4) | ((ppu.lo & 0x0F) << 4);
          ppu.lo = ((ppu.lo & 0xCC) >> 2) | ((ppu.lo & 0x33) << 2);
          ppu.lo = ((ppu.lo & 0xAA) >> 1) | ((ppu.lo & 0x55) << 1);
          ppu.hi = ((ppu.hi & 0xF0) >> 4) | ((ppu.hi & 0x0F) << 4);
          ppu.hi = ((ppu.hi & 0xCC) >> 2) | ((ppu.hi & 0x33) << 2);
          ppu.hi = ((ppu.hi & 0xAA) >> 1) | ((ppu.hi & 0x55) << 1);
        }
        ppu.sprite_shift_lo[i] = ppu.lo;
        ppu.sprite_shift_hi[i] = ppu.hi;
      }
      break;
    }
  }
}

static void ppu_render_pixel()
{
  u8 pixel_back = 0;
  if ((ppu.mask & PPU_MASK_BACK) && (ppu.cyc > 8 || (ppu.mask & PPU_MASK_SHOW_LEFT_BACK))) {
    u16 bit_mask = 0x8000 >> ppu.x;
    u8 p0 = (ppu.shift_tile_lo & bit_mask) ? 1 : 0;
    u8 p1 = (ppu.shift_tile_hi & bit_mask) ? 1 : 0;
    pixel_back = p0 | (p1 << 1);
    if (pixel_back) {
      u8 a0 = (ppu.shift_attr_lo & bit_mask) ? 1 : 0;
      u8 a1 = (ppu.shift_attr_hi & bit_mask) ? 1 : 0;
      pixel_back |= (a0 << 2) | (a1 << 3);
    }
  }

  u8 pixel_sprite = 0;
  bool sprite_priority = false;
  bool is_sprite0 = false;
  if ((ppu.mask & PPU_MASK_SPRITE) && (ppu.cyc > 8 || (ppu.mask & PPU_MASK_SHOW_LEFT_SPRITE))) {
    u8 screen_x = ppu.cyc - 1;
    for (u8 i = 0; i < ppu.sprite_render_count; i++) {
      PpuSprite* sprite = &ppu.sprite_render[i];
      if (screen_x >= sprite->x && screen_x < (sprite->x + 8)) {
        u8 offset = screen_x - sprite->x;
        u8 bit_mask = 0x80 >> offset;

        u8 p0 = (ppu.sprite_shift_lo[i] & bit_mask) ? 1 : 0;
        u8 p1 = (ppu.sprite_shift_hi[i] & bit_mask) ? 1 : 0;
        pixel_sprite = p0 | (p1 << 1);

        if (pixel_sprite) {
          pixel_sprite |= 0x10 | ((sprite->attr & 0x03) << 2);
          sprite_priority = (sprite->attr & PPU_SPRITE_PRIORITY) == 0;
          is_sprite0 = (i == 0 && ppu.sprite_render_has0);
          break;
        }
      }
    }
  }

  if (pixel_back && pixel_sprite && is_sprite0 && ppu.cyc < PPU_CYC_END) { // NES bug: hit not set on x=255
    ppu.status |= PPU_STATUS_SPRITE0_HIT;
  }

  if (pixel_sprite && (pixel_back == 0 || sprite_priority)) {
    ppu.pixel = pixel_sprite;
  } else {
    ppu.pixel = pixel_back;
  }
}

static void ppu_forced_vblank() { ppu.pixel = ppu.v >= 0x3F00 ? ppu.v : 0; }
static void ppu_palette_color() { ppu.color = ppu.pam[ppu_pam_addr(ppu.pixel)] & 0x3F; }

static void ppu_dac(u8 x)
{
  if (ppu.mask & PPU_MASK_GRAY) {
    ppu.color &= 0x30;
  }

  u32 rgb = palette[ppu.color];

  if ((ppu.mask & PPU_MASK_RED) || (ppu.mask & PPU_MASK_GREEN) || (ppu.mask & PPU_MASK_BLUE)) {
    float rf = 1.0f;
    float gf = 1.0f;
    float bf = 1.0f;
    if (ppu.mask & PPU_MASK_RED) {
      gf *= ppu_tint_accent;
      bf *= ppu_tint_accent;
    }
    if (ppu.mask & PPU_MASK_GREEN) {
      rf *= ppu_tint_accent;
      bf *= ppu_tint_accent;
    }
    if (ppu.mask & PPU_MASK_BLUE) {
      rf *= ppu_tint_accent;
      gf *= ppu_tint_accent;
    }
    u8 r = (rgb & 0xFF0000) >> 16;
    u8 g = (rgb & 0x00FF00) >> 8;
    u8 b = (rgb & 0x0000FF);
    r *= rf;
    g *= gf;
    b *= bf;
    rgb = (r << 16) | (g << 8) | b;
  }

  rgb |= 0xFF000000;

  u16 out_idx = (ppu.sl << 8) | x;
  dev_output(out_idx, rgb);
}

void ppu_tick()
{
  if (ppu.read_cyc > 0) {
    if (--ppu.read_cyc == 0) {
      ppu.read_buf = ppu_read();
      ppu_inc();
    }
  }
  if (ppu.write_cyc > 0) {
    if (--ppu.write_cyc == 0) {
      ppu_write(ppu.write_buf);
      ppu_inc();
    }
  }

  if (ppu.sl <= PPU_SL_END) {
    if (ppu_is_rendering()) {
      if (ppu.cyc >= PPU_CYC_BEGIN + 2 && ppu.cyc <= PPU_CYC_END + 2) {
        ppu_dac(ppu.cyc - 3);
      }
      if (ppu.cyc >= PPU_CYC_BEGIN + 1 && ppu.cyc <= PPU_CYC_END + 1) {
        ppu_palette_color();
      }
      if (ppu.cyc >= PPU_CYC_BEGIN && ppu.cyc <= PPU_CYC_END) {
        ppu_render_pixel();
      }
    } else {
      if (ppu.cyc >= PPU_CYC_BEGIN && ppu.cyc <= PPU_CYC_END) {
        ppu_forced_vblank();
        ppu_palette_color();
        ppu_dac(ppu.cyc - 1);
      }
    }
  }

  if (ppu_is_rendering()) {
    bool inc_x = false;
    if ((ppu.cyc >= PPU_CYC_BEGIN && ppu.cyc <= PPU_CYC_END)) {
      inc_x = ppu_fetch_back();
      ppu_evaluate_sprites();
    } else if (ppu.cyc >= PPU_CYC_SPRITE_BEGIN && ppu.cyc <= PPU_CYC_SPRITE_END) {
      if (ppu.cyc == PPU_CYC_SPRITE_BEGIN) {
        ppu_evaluate_sprite_swap();
      }
      ppu_fetch_sprites();
    } else if (ppu.cyc >= PPU_CYC_PREFETCH_BEGIN && ppu.cyc <= PPU_CYC_PREFETCH_END) {
      inc_x = ppu_fetch_back();
    }

    if (ppu.increment_xy || inc_x) {
      ppu_inc_x();
    }

    if (ppu.increment_xy || ppu.cyc == PPU_CYC_END) {
      ppu_inc_y();
    }

    if (ppu.cyc == PPU_CYC_SWAP_X) {
      ppu_swap_x();
    }

    if (ppu.sl == PPU_SL_PRE_RENDER && ppu.cyc >= PPU_CYC_SWAP_Y_BEGIN && ppu.cyc <= PPU_CYC_SWAP_Y_END) {
      ppu_swap_y();
    }
  }

  if (ppu.sl == PPU_SL_PRE_RENDER && ppu.cyc == PPU_CYC_ZERO) {
    ppu.status &= ~(PPU_STATUS_SPRITE0_HIT | PPU_STATUS_SPRITE_OVERFLOW);
    ppu_open_bus_decay();
  }

  if (ppu.sl == PPU_SL_PRE_RENDER && ppu.cyc == PPU_CYC_BEGIN) {
    ppu.status &= ~PPU_STATUS_VBLANK;
  }

  if ((ppu.sl == PPU_SL_VBLANK && ppu.cyc == PPU_CYC_BEGIN) && !ppu.suppress_vblank) {
    ppu.status |= PPU_STATUS_VBLANK;
    ppu.check_nmi = true;
  }

  if (ppu.check_nmi) {
    cpu_nmi(!ppu.suppress_vblank && (ppu.status & PPU_STATUS_VBLANK) && (ppu.ctrl & PPU_CTRL_NMI));
  }

  ppu.increment_xy = false;
  ppu.suppress_vblank = false;
  ppu.check_nmi = false;

  if (++ppu.cyc > PPU_CYC_LAST) {
    ppu.cyc = 0;
    if (++ppu.sl > PPU_SL_PRE_RENDER) {
      ppu.sl = 0;
      ppu.odd_frame = !ppu.odd_frame;
    }
  }
  if (ppu.odd_frame && ppu.sl == PPU_SL_PRE_RENDER && ppu.cyc == PPU_CYC_LAST - 1 && ppu_is_rendering()) {
    ++ppu.cyc;
  }
}
