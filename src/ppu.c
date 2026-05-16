#include "ppu.h"
#include "cpu.h"
#include "dev.h"
#include "rom.h"
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

enum PpuSlCyc : u16
{
  PPU_SL_END = 239,
  PPU_SL_POST_RENDER = 240,
  PPU_SL_VBLANK = 241,
  PPU_SL_PRE_RENDER = 261,

  PPU_CYC_ZERO = 0,
  PPU_CYC_BEGIN = 1,
  PPU_CYC_END = 256,
  PPU_CYC_SWAP_X = 257,
  PPU_CYC_SWAP_Y_BEGIN = 280,
  PPU_CYC_SWAP_Y_END = 304,
  PPU_CYC_PREFETCH_BEGIN = 321,
  PPU_CYC_PREFETCH_END = 336,
  PPU_CYC_LAST = 340,
};

enum PpuCtrl : u8
{
  PPU_CTRL_NAMETABLE_X = (1 << 0),
  PPU_CTRL_NAMETABLE_Y = (1 << 1), // 240
  PPU_CTRL_INC_Y = (1 << 2),
  PPU_CTRL_SPRITE_NAMETABLE = (1 << 3),
  PPU_CTRL_BACK_NAMETABLE = (1 << 4),
  PPU_CTRL_SPRITE_SIZE = (1 << 5),
  PPU_CTRL_EXT = (1 << 6), // todo: ?
  PPU_CTRL_NMI = (1 << 7),
};

enum PpuMask : u8
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

enum PpuStatus : u8
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

struct PpuSprite
{
  u8 y;
  u8 idx;
  u8 attr;
  u8 x;
};

static struct Ppu
{
  u8 oam[0x100];
  u8 pam[0x20];

  u8 ctrl;
  u8 mask;
  u8 status;
  u8 oam_addr;

  u16 t;
  u16 v;
  u8 x;

  u8 nt;
  u8 at;
  u8 lo;
  u8 hi;

  u16 shift_tile_lo;
  u16 shift_tile_hi;
  u16 shift_attr_lo;
  u16 shift_attr_hi;

  u8 read_buf;
  bool write_latch;
  bool odd_frame;
  bool suppress_vblank;
  bool increment_xy;
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

#if MN_TRACE_PPU
#define trace_ppu(fmt, ...)                                                                                    \
  tracef("PPU  " fmt "  C:%02X M:%02X ST:%02X O:%02X SY:%02X SX:%02X A:%04X L:%d B:%02X PPU:%3d,%3d CYC:%d\n", \
         __VA_ARGS__, ppu.ctrl, ppu.mask, ppu.status, ppu.sprite_addr, ppu.scroll[0], ppu.scroll[1], ppu.addr, \
         ppu.write_latch, ppu.read_buf, dev.ppu_sl, dev.ppu_cyc, dev.cpu_cyc);
#else
#define trace_ppu(fmt, ...) (void)0
#endif

static u8 ppu_pam_addr(u16 addr) { return (addr & 0x03) == 0 ? (addr & 0x0F) : (addr & 0x1F); }

static u8 ppu_read_addr(u16 addr)
{
  addr &= 0x3FFF;
  if (addr < 0x2000) {
    return rom->chr ? rom->chr[addr] : dev.wram[addr];
  } else if (addr < 0x3F00) {
    return dev.vram[addr & 0xFFF];
  } else {
    return ppu.pam[ppu_pam_addr(addr)];
  }
}

static void ppu_write_addr(u16 addr, u8 val)
{
  addr &= 0x3FFF;
  if (addr < 0x2000) {
    if (!rom->chr) {
      dev.wram[addr] = val;
    }
  } else if (addr < 0x3F00) {
    dev.vram[addr & 0xFFF] = val;
  } else {
    ppu.pam[ppu_pam_addr(addr)] = val;
  }
}

static bool ppu_rendering_enabled() { return (ppu.mask & PPU_MASK_BACK) || (ppu.mask & PPU_MASK_SPRITE); }

static bool ppu_is_rendering()
{
  return ppu_rendering_enabled() && (dev.ppu_sl <= PPU_SL_END || dev.ppu_sl == PPU_SL_PRE_RENDER);
}

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
    u8 y = (ppu.v & 0x03E0) >> 5; // y = coarse Y
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
      u8 status = ppu.status;
      if (!trace) {
        ppu.write_latch = false;
        ppu.suppress_vblank = true;
        ppu.status &= ~PPU_STATUS_VBLANK;
      }
      return status;
    }
    case 4: return ppu.oam[ppu.oam_addr]; // ++?
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
      return val;
    }
  }
  return 0; // openbus?
}

void ppu_bus_write(u16 addr, u8 val)
{
  switch (addr & 0x7) {
    case 0:
      if ((ppu.status & PPU_STATUS_VBLANK) && !(ppu.ctrl & PPU_CTRL_NMI) && (val & PPU_CTRL_NMI)) {
        cpu_nmi();
      }
      ppu.ctrl = val;
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

static void ppu_swap_x() { ppu.v = (ppu.v & 0x7BE0) | (ppu.t & 0x041F); }
static void ppu_swap_y() { ppu.v = (ppu.v & 0x041F) | (ppu.t & 0x7BE0); }

static void ppu_fetch_back()
{
  ppu.shift_tile_lo <<= 1;
  ppu.shift_tile_hi <<= 1;
  ppu.shift_attr_lo <<= 1;
  ppu.shift_attr_hi <<= 1;

  switch (dev.ppu_cyc % 8) {
    case 1: { // nametable
      ppu.nt = ppu_read_addr(0x2000 | (ppu.v & 0x0FFF));
      break;
    }
    case 3: { // attribute
      u8 attr = ppu_read_addr(0x23C0 | (ppu.v & 0x0C00) | ((ppu.v & 0x0380) >> 4) | ((ppu.v & 0x001C) >> 2));
      u8 coarse_x = ppu.v & 0x001F;
      u8 coarse_y = (ppu.v & 0x03E0) >> 5;
      u8 shift = ((coarse_y & 2) << 1) | (coarse_x & 2);
      ppu.at = (attr >> shift) & 0x03;
      break;
    }
    case 5: { // chr lo
      u16 fine_y = (ppu.v & 0x7000) >> 12;
      u16 table = (ppu.ctrl & PPU_CTRL_BACK_NAMETABLE) ? 0x1000 : 0x0000;
      ppu.lo = ppu_read_addr(table + ((u16)ppu.nt << 4) + fine_y);
      break;
    }
    case 7: { // chr hi
      u16 fine_y = (ppu.v & 0x7000) >> 12;
      u16 table = (ppu.ctrl & PPU_CTRL_BACK_NAMETABLE) ? 0x1000 : 0x0000;
      ppu.hi = ppu_read_addr(table + ((u16)ppu.nt << 4) + fine_y + 8);
      break;
    }
    case 0: { // shift
      ppu.shift_tile_lo = (ppu.shift_tile_lo & 0xFF00) | ppu.lo;
      ppu.shift_tile_hi = (ppu.shift_tile_hi & 0xFF00) | ppu.hi;
      ppu.shift_attr_lo = (ppu.shift_attr_lo & 0xFF00) | ((ppu.at & 1) ? 0xFF : 0x00);
      ppu.shift_attr_hi = (ppu.shift_attr_hi & 0xFF00) | ((ppu.at & 2) ? 0xFF : 0x00);
      break;
    }
  }
}

static void ppu_fetch_unused()
{
  switch (dev.ppu_cyc % 8) {
    case 1: ppu.nt = ppu_read_addr(0x2000 | (ppu.v & 0x0FFF)); break; // unused
    case 3: ppu_read_addr(0x2000 | (ppu.v & 0x0FFF)); break; // ignored
  }
}

static void ppu_pixel()
{
  u8 pixel = (!ppu_rendering_enabled() && ppu.v >= 0x3F00) ? ppu.v : 0;

  if ((ppu.mask & PPU_MASK_BACK) && (dev.ppu_cyc > 8 || (ppu.mask & PPU_MASK_SHOW_LEFT_BACK))) {
    u16 bit_mask = 0x8000 >> ppu.x;
    u8 p0 = (ppu.shift_tile_lo & bit_mask) ? 1 : 0;
    u8 p1 = (ppu.shift_tile_hi & bit_mask) ? 1 : 0;
    u8 a0 = (ppu.shift_attr_lo & bit_mask) ? 1 : 0;
    u8 a1 = (ppu.shift_attr_hi & bit_mask) ? 1 : 0;
    pixel = p0 | (p1 << 1) | (a0 << 2) | (a1 << 3);
  }

  if ((ppu.mask & PPU_MASK_SPRITE) && (dev.ppu_cyc > 8 || (ppu.mask & PPU_MASK_SHOW_LEFT_SPRITE))) {
    // todo: sprites
    // pixel = 0x10;
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

  u16 screen_idx = (dev.ppu_sl << 8) | (dev.ppu_cyc - 1);
  dev.screen[screen_idx] = color;
}

void ppu_tick()
{
  if (dev.ppu_sl <= PPU_SL_END && dev.ppu_cyc >= PPU_CYC_BEGIN && dev.ppu_cyc <= PPU_CYC_END) {
    ppu_pixel();
  }

  if (ppu_is_rendering()) {
    if (dev.ppu_sl == PPU_SL_PRE_RENDER) {
      if (dev.ppu_cyc >= PPU_CYC_SWAP_Y_BEGIN && dev.ppu_cyc <= PPU_CYC_SWAP_Y_END) {
        ppu_swap_y();
      }
      if (ppu.odd_frame && dev.ppu_cyc == PPU_CYC_LAST - 1) {
        ++dev.ppu_cyc;
      }
    }

    if ((dev.ppu_cyc >= PPU_CYC_BEGIN && dev.ppu_cyc <= PPU_CYC_END)
        || (dev.ppu_cyc >= PPU_CYC_PREFETCH_BEGIN && dev.ppu_cyc <= PPU_CYC_PREFETCH_END))
    {
      ppu_fetch_back();
      if (ppu.increment_xy || (dev.ppu_cyc % 8) == 0) {
        ppu_inc_x();
      }
    } else {
      ppu_fetch_unused();
    }

    if (ppu.increment_xy || dev.ppu_cyc == PPU_CYC_END) {
      ppu_inc_y();
    }

    if (dev.ppu_cyc == PPU_CYC_SWAP_X) {
      ppu_swap_x();
    }
  }

  if (dev.ppu_sl == PPU_SL_PRE_RENDER && dev.ppu_cyc == PPU_CYC_BEGIN) {
    ppu.status &= ~(PPU_STATUS_VBLANK | PPU_STATUS_HIT | PPU_STATUS_SPRITE_OVERFLOW);
  }

  if (dev.ppu_sl == PPU_SL_VBLANK && dev.ppu_cyc == PPU_CYC_BEGIN) {
    if (!ppu.suppress_vblank) {
      ppu.status |= PPU_STATUS_VBLANK;
      dev.vblank = true;
      if (ppu.ctrl & PPU_CTRL_NMI) {
        cpu_nmi();
      }
    }
  }

  ppu.suppress_vblank = false;
  ppu.increment_xy = false;

  if (++dev.ppu_cyc > PPU_CYC_LAST) {
    dev.ppu_cyc = 0;
    if (++dev.ppu_sl > PPU_SL_PRE_RENDER) {
      dev.ppu_sl = 0;
      ppu.odd_frame = !ppu.odd_frame;
#if MN_TRACE_BLARGG
      static bool running = false;
      if (dev.sram[0] == 0x80) {
        running = true;
      } else if (running) {
        dev.quit = true;
      }
      tracef("\e[1;1HBLARGG status=%02X\n%s\n", dev.sram[0], (const char*)&dev.sram[4]);
#endif
    }
  }
}
