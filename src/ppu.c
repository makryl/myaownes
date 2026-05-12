#include "ppu.h"
#include "cpu.h"
#include "dev.h"
#include "rom.h"
#include "common.h"
#include <string.h>

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
  PPU_CYC_PRE_FETCH = 321,
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

static u8 ppu_read_addr(u16 addr)
{
  addr &= 0x3FFF;
  if (addr < 0x2000) {
    return rom->chr[addr];
  } else if (addr < 0x3F00) {
    return dev.vram[addr & 0xFFF];
  } else {
    addr &= 0x1F;
    if (addr >= 0x10 && (addr & 0x03) == 0) {
      addr -= 0x10;
    }
    return ppu.pam[addr];
  }
}

static void ppu_write_addr(u16 addr, u8 val)
{
  addr &= 0x3FFF;
  if (addr < 0x2000) {
    // vrom
  } else if (addr < 0x3F00) {
    dev.vram[addr & 0xFFF] = val;
  } else {
    addr &= 0x1F;
    if (addr >= 0x10 && (addr & 0x03) == 0) {
      addr -= 0x10;
    }
    ppu.pam[addr] = val;
  }
}

static bool ppu_rendering_enabled() { return (ppu.mask & PPU_MASK_BACK) || (ppu.mask & PPU_MASK_SPRITE); }

static bool ppu_is_rendering()
{
  return ppu_rendering_enabled() && (dev.ppu_sl < PPU_SL_END || dev.ppu_sl == PPU_SL_PRE_RENDER);
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
        ppu.read_buf = ppu_read_addr(ppu.v - 0x1000);
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
      if (++ppu.write_latch) {
        ppu.t = (ppu.t & 0x7FE0) | (((u16)val & 0xF8) >> 3);
        ppu.x = (val & 7);
      } else {
        ppu.t = (ppu.t & 0x0C1F) | (((u16)val & 0xF8) << 2) | (((u16)val & 7) << 12);
      }
      break;
    case 6:
      if (++ppu.write_latch) {
        ppu.t = (ppu.t & 0x00FF) | (((u16)val & 0x3F) << 8);
      } else {
        ppu.t = (ppu.t & 0xFF00) | val;
      }
      ppu.v = ppu.t; // todo: delay
      break;
    case 7:
      ppu_write_addr(ppu.v, val);
      ppu_inc();
      break;
  }
}

static void ppu_render()
{
  // if (dev.ppu_sl == sprite[0].y) { // todo: opaque pixel check
  // ppu.status |= PPU_STATUS_HIT; // todo: if rendering enabled in 2001
  // }

  // u16 tile_addr = 0x2000 | (ppu.v & 0x0FFF);
  // u16 attr_addr = 0x23C0 | (ppu.v & 0x0C00) | (ppu.v & 0x0380) | (ppu.v & 0x1C); // NN 1111 YYY XXX
}

void ppu_tick()
{
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

  if (ppu_is_rendering()) {
    if (dev.ppu_sl == PPU_SL_PRE_RENDER) {
      if (dev.ppu_cyc >= PPU_CYC_SWAP_Y_BEGIN && dev.ppu_cyc <= PPU_CYC_SWAP_Y_END) {
        ppu.v = (ppu.v & 0x041F) | (ppu.t & 0x7BE0);
      }
      if (ppu.odd_frame && dev.ppu_cyc == PPU_CYC_LAST - 1) {
        ++dev.ppu_cyc;
      }
    }

    if ((dev.ppu_cyc >= PPU_CYC_BEGIN && dev.ppu_cyc <= PPU_CYC_END) || dev.ppu_cyc >= PPU_CYC_PRE_FETCH) {
      bool dot8 = (dev.ppu_cyc & 7) == 0;
      if (dot8) {
        //
      }
      if (ppu.increment_xy || dot8) {
        ppu_inc_x();
      }
    }

    if (ppu.increment_xy || dev.ppu_cyc == PPU_CYC_END) {
      ppu_inc_y();
    }

    if (dev.ppu_cyc == PPU_CYC_SWAP_X) {
      ppu.v = (ppu.v & 0x7BE0) | (ppu.t & 0x041F);
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
