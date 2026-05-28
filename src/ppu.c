#include "ppu.h"
#include "cpu.h"
#include "dev.h"
#include "map.h"
#include "common.h"
#include <string.h>

// clang-format off
static const u32 palette[0x40] = {
  // 0x666666, 0x002A88, 0x1412A7, 0x3B00A4, 0x5C007E, 0x6E0040, 0x6C0600, 0x561D00, 0x333500, 0x0B4800, 0x005200, 0x004F08, 0x00404D, 0x000000, 0x000000, 0x000000, //
  // 0xADADAD, 0x155FD9, 0x4240FF, 0x7527FE, 0xA01ADF, 0xB71E8B, 0xB33100, 0x964D00, 0x706600, 0x387F00, 0x108B00, 0x00831E, 0x007474, 0x000000, 0x000000, 0x000000, //
  // 0xFFFEFF, 0x64B0FF, 0x9290FF, 0xC676FF, 0xF36AFF, 0xFE6ECC, 0xFE8170, 0xEA9E22, 0xBCBE00, 0x88D800, 0x5CE430, 0x45E082, 0x48CDDE, 0x4F4F4F, 0x000000, 0x000000, //
  // 0xFFFEFF, 0xC0DFFF, 0xD3D2FF, 0xE8C8FF, 0xFBC2FF, 0xFEC4EA, 0xFECCC5, 0xF7D8A5, 0xE4E594, 0xCFEF96, 0xBDF4AB, 0xB3F3CC, 0xB5EBF2, 0xB8B8B8, 0x000000, 0x000000, //

  0x7C7C7C, 0x0000FC, 0x0000BC, 0x4428BC, 0x940084, 0xA80020, 0xA81000, 0x881400, 0x503000, 0x007800, 0x006800, 0x005800, 0x004058, 0x000000, 0x000000, 0x000000, //
  0xBCBCBC, 0x0078F8, 0x0058F8, 0x6844FC, 0xD800CC, 0xE40058, 0xF83800, 0xE45C10, 0xAC7C00, 0x00B800, 0x00A800, 0x00A844, 0x008888, 0x000000, 0x000000, 0x000000, //
  0xF8F8F8, 0x3CBCFC, 0x6888FC, 0x9878F8, 0xF878F8, 0xF85898, 0xF87858, 0xFCA044, 0xF8B800, 0xB8F818, 0x58D854, 0x58F898, 0x00E8D8, 0x787878, 0x000000, 0x000000, //
  0xFFFFFF, 0xA4E4FC, 0xB8B8F8, 0xD8B8F8, 0xF8B8F8, 0xF8A4C0, 0xF0D0B0, 0xFCE0A8, 0xF8D878, 0xD8F878, 0xB8F8B8, 0xB8F8D8, 0x00FCFC, 0xD8D8D8, 0x000000, 0x000000, //
};
// clang-format on

static const float ppu_tint_accent = 0.75f;

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
  // PPU_STATUS_CAN_WRITE = (1 << 4),
  PPU_STATUS_SPRITE_OVERFLOW = (1 << 5),
  PPU_STATUS_HIT = (1 << 6),
  PPU_STATUS_VBLANK = (1 << 7),
};

enum PpuSpriteAttr : u8
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

  u8 ctrl;
  u8 mask;
  u8 status;
  u8 oam_addr;

  u16 t;
  u16 v;
  u8 x;

  u16 nt;
  u8 at;
  u8 lo;
  u8 hi;

  u16 attr_addr;
  u8 attr_shift;

  u16 shift_tile_lo;
  u16 shift_tile_hi;
  u16 shift_attr_lo;
  u16 shift_attr_hi;

  PpuSprite sprite[2][8];
  PpuSprite* sprite_oam;
  PpuSprite* sprite_eval;
  PpuSprite* sprite_render;
  u16 sprite_addr;
  u8 sprite_eval_count;
  u8 sprite_render_count;
  u8 sprite_busy;
  u8 sprite_shift_lo[8];
  u8 sprite_shift_hi[8];

  u8 read_buf;
  u8 open_bus;
  u8 write_v;

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
  ppu.write_latch = false;
  ppu.odd_frame = false;
}

u16 ppu_cyc() { return ppu.cyc; }
u16 ppu_sl() { return ppu.sl; }
bool ppu_is_vblank() { return ppu.status & PPU_STATUS_VBLANK; }

#if MN_TRACE_PPU
#define trace_ppu(fmt, ...)                                                                                    \
  tracef("PPU  " fmt "  C:%02X M:%02X ST:%02X O:%02X SY:%02X SX:%02X A:%04X L:%d B:%02X PPU:%3d,%3d CYC:%d\n", \
         __VA_ARGS__, ppu.ctrl, ppu.mask, ppu.status, ppu.sprite_addr, ppu.scroll[0], ppu.scroll[1], ppu.addr, \
         ppu.write_latch, ppu.read_buf, ppu.sl, ppu.cyc, cpu_cyc());
#else
#define trace_ppu(fmt, ...) (void)0
#endif

static u8 ppu_pam_addr(u16 addr) { return (addr & 0x03) == 0 ? (addr & 0x0F) : (addr & 0x1F); }

static u8 ppu_read_addr(u16 addr)
{
  addr &= 0x3FFF;
  if (addr >= 0x3F00) {
    return ppu.pam[ppu_pam_addr(addr)];
  } else {
    return map_ppu_read(addr);
  }
}

static void ppu_write_addr(u16 addr, u8 val)
{
  addr &= 0x3FFF;
  if (addr >= 0x3F00) {
    ppu.pam[ppu_pam_addr(addr)] = val;
  } else {
    map_ppu_write(addr, val);
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
  }
}

u8 ppu_bus_read(u16 addr, bool trace)
{
  switch (addr & 0x7) {
    case 2: {
      if (trace) {
        return ppu.status;
      }
      ppu.open_bus = ppu.status;
      ppu.write_latch = false;
      ppu.suppress_vblank = true;
      ppu.check_nmi = true;
      ppu.status &= ~PPU_STATUS_VBLANK;
      break;
    }
    case 4: {
      ppu.open_bus = ppu.oam[ppu.oam_addr]; // ++?
      break;
    }
    case 7: {
      u8 val;
      if ((ppu.v & 0x3FFF) < 0x3F00) {
        val = ppu.read_buf;
        ppu.read_buf = ppu_read_addr(ppu.v);
      } else {
        val = ppu_read_addr(ppu.v);
        ppu.read_buf = ppu_read_addr(ppu.v & 0x2FFF);
      }
      if (trace) {
        return val;
      }
      ppu_inc();
      ppu.open_bus = val;
      break;
    }
  }
  return ppu.open_bus;
}

void ppu_bus_write(u16 addr, u8 val)
{
  ppu.open_bus = val;
  switch (addr & 0x7) {
    case 0:
      ppu.check_nmi = (ppu.ctrl & PPU_CTRL_NMI) != (val & PPU_CTRL_NMI);
      ppu.ctrl = val;
      ppu.t = (ppu.t & 0xF3FF) | (((u16)val & 0x03) << 10);
      break;
    case 1: ppu.mask = val; break;
    case 3: ppu.oam_addr = val; break;
    case 4: ppu.oam[ppu.oam_addr++] = val; break;
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
      }
      ppu.write_latch = !ppu.write_latch;
      break;
    case 7:
      ppu_write_addr(ppu.v, val);
      ppu_inc();
      break;
  }
}

static bool ppu_fetch_back()
{
  ppu.shift_tile_lo <<= 1;
  ppu.shift_tile_hi <<= 1;
  ppu.shift_attr_lo <<= 1;
  ppu.shift_attr_hi <<= 1;

  switch (ppu.cyc % 8) {
    case 1: { // nametable
      u16 table = (ppu.ctrl & PPU_CTRL_BACK_NAMETABLE) ? 0x1000 : 0x0000;
      u16 idx = ppu_read_addr(0x2000 | (ppu.v & 0x0FFF));
      u16 fine_y = (ppu.v & 0x7000) >> 12;
      ppu.nt = table | (idx << 4) | fine_y;

      u16 nt_select = ppu.v & 0x0C00;
      u16 coarse_x = ppu.v & 0x001F;
      u16 coarse_y = (ppu.v & 0x03E0) >> 5;
      ppu.attr_addr = 0x23C0 | nt_select | ((coarse_y >> 2) << 3) | (coarse_x >> 2);
      ppu.attr_shift = ((coarse_y & 2) << 1) | (coarse_x & 2);
      return false;
    }
    case 3: { // attribute
      u8 attr = ppu_read_addr(ppu.attr_addr);
      ppu.at = (attr >> ppu.attr_shift) & 0x03;
      return false;
    }
    case 5: { // chr lo
      ppu.lo = ppu_read_addr(ppu.nt);
      return false;
    }
    case 7: { // chr hi
      ppu.hi = ppu_read_addr(ppu.nt | 8);
      return false;
    }
    case 0: { // shift
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
    ppu.sprite_oam = (PpuSprite*)ppu.oam;
    ppu.sprite_eval = ppu.sprite[(ppu.sl & 1) ? 1 : 0];
    ppu.sprite_eval_count = 0;
    ppu.sprite_eval_has0 = false;
    ppu.sprite_addr = 0;
    ppu.sprite_busy = 63;
    return;
  }

  if ((u8*)ppu.sprite_oam > &ppu.oam[sizeof(ppu.oam) - sizeof(PpuSprite)]) {
    return;
  }

  u16 next_sl = ppu.sl == PPU_SL_PRE_RENDER ? 0 : (ppu.sl + 1);

  u8 sprite_height = (ppu.ctrl & PPU_CTRL_SPRITE_SIZE) ? 16 : 8;
  if (next_sl > ppu.sprite_oam->y && next_sl <= (ppu.sprite_oam->y + sprite_height)) {
    if (ppu.sprite_eval_count < 8) {
      memcpy(&ppu.sprite_eval[ppu.sprite_eval_count++], ppu.sprite_oam, sizeof(PpuSprite));
      ppu.sprite_busy += 6;
      if (ppu.sprite_oam == (PpuSprite*)ppu.oam) {
        ppu.sprite_eval_has0 = true;
      }
    } else {
      ppu.status |= PPU_STATUS_SPRITE_OVERFLOW;
    }
  }

  ++ppu.sprite_oam;
  ppu.sprite_busy += 2;
}

static void ppu_fetch_sprites()
{
  u8 i = (ppu.cyc - PPU_CYC_SPRITE_BEGIN) / 8;
  PpuSprite* sprite = &ppu.sprite_render[i];

  switch (ppu.cyc % 8) {
    case 1: {
      ppu.nt = ppu_read_addr(0x2000 | (ppu.v & 0x0FFF)); // unused NT
      if (sprite) {
        u16 next_sl = (ppu.sl == PPU_SL_PRE_RENDER) ? 0 : (ppu.sl + 1);
        u8 sprite_height = (ppu.ctrl & PPU_CTRL_SPRITE_SIZE) ? 16 : 8;

        u16 row = next_sl - sprite->y - 1;
        if (sprite->attr & PPU_SPRITE_FLIP_VERT) {
          row = (sprite_height - 1) - row;
        }

        if (!(ppu.ctrl & PPU_CTRL_SPRITE_SIZE)) { // 8x8
          u16 table = (ppu.ctrl & PPU_CTRL_SPRITE_NAMETABLE) ? 0x1000 : 0x0000;
          ppu.sprite_addr = table + (sprite->tile << 4) + row;
        } else {
          u16 table = (sprite->tile & 1) ? 0x1000 : 0x0000;
          ppu.sprite_addr = table + ((sprite->tile & 0xFE) << 4) + (row >= 8 ? row + 8 : row);
        }
      } else {
        u16 table = (ppu.ctrl & PPU_CTRL_SPRITE_NAMETABLE) ? 0x1000 : 0x0000;
        ppu.sprite_addr = table + (0xFF << 4);
      }
      break;
    }
    case 3: {
      ppu_read_addr(0x2000 | (ppu.v & 0x0FFF)); // ignored NT
      break;
    }
    case 5: {
      ppu.lo = ppu_read_addr(ppu.sprite_addr);
      break;
    }
    case 7: {
      ppu.hi = ppu_read_addr(ppu.sprite_addr + 8);
      break;
    }
    case 0: {
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

static void ppu_render()
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

  u8 pixel = 0;
  if ((!ppu_rendering_enabled() && ppu.v >= 0x3F00)) {
    pixel = ppu.v;
  } else {
    if (pixel_back && pixel_sprite && is_sprite0 && ppu.cyc < PPU_CYC_END) { // intended emulation bug (<)
      ppu.status |= PPU_STATUS_HIT;
    }
    if (pixel_sprite && (pixel_back == 0 || sprite_priority)) {
      pixel = pixel_sprite;
    } else {
      pixel = pixel_back;
    }
  }

  u8 color_idx = ppu.pam[ppu_pam_addr(pixel)] & 0x3F;
  u32 color = palette[color_idx];

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
    u8 r = (color & 0xFF0000) >> 16;
    u8 g = (color & 0x00FF00) >> 8;
    u8 b = (color & 0x0000FF);
    r *= rf;
    g *= gf;
    b *= bf;
    color = (r << 16) | (g << 8) | b;
  }

  u16 out_idx = (ppu.sl << 8) | (ppu.cyc - 1);
  dev_output(out_idx, color);
}

void ppu_tick()
{
  if (ppu.sl <= PPU_SL_END && ppu.cyc >= PPU_CYC_BEGIN && ppu.cyc <= PPU_CYC_END) {
    ppu_render();
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
    ppu.status &= ~(PPU_STATUS_HIT | PPU_STATUS_SPRITE_OVERFLOW);
  }

  if (ppu.sl == PPU_SL_PRE_RENDER && ppu.cyc == PPU_CYC_BEGIN) {
    ppu.status &= ~(PPU_STATUS_VBLANK);
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
