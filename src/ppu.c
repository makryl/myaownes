#include "ppu.h"
#include "cpu.h"
#include "dev.h"
#include "nes.h"
#include "common.h"

enum PpuCoords : u16
{
  PPU_SL_VBLANK = 241,
  PPU_SL_LAST = 261,
  PPU_CYC_BEGIN = 1,
  PPU_CYC_LAST = 340,
};

enum PpuControl : u16
{
  PPU_CONTROL0_VERT = (1 << 2),
  PPU_CONTROL0_HIT_ALLOWED = (1 << 6),
  PPU_CONTROL0_NMI_ALLOWED = (1 << 7),
};

enum PpuStatus : u8
{
  // PPU_STATUS_CAN_WRITE = (1 << 4),
  // PPU_STATUS_MORE_8_SPRITE = (1 << 5),
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

static struct Ppu
{
  u8 control0;
  u8 control1;
  u8 status;
  u8 sprite_addr;
  u8 scroll[2];
  u16 addr;
  u8 write_latch;
  u8 read_buf;
} ppu;

static struct PpuSprite
{
  u8 y;
  u8 idx;
  u8 attr;
  u8 x;
} sprite[64];


#if MN_TRACE_PPU
#define trace_ppu(fmt, ...)                                                                                            \
  tracef("PPU  " fmt "  C0:%02X C1:%02X ST:%02X SA:%02X SY:%02X SX:%02X A:%04X L:%02X B:%02X PPU:%3d,%3d CYC:%d\n",    \
         __VA_ARGS__, ppu.control0, ppu.control1, ppu.status, ppu.sprite_addr, ppu.scroll[0], ppu.scroll[1], ppu.addr, \
         ppu.write_latch, ppu.read_buf, dev.ppu_sl, dev.ppu_cyc, dev.cpu_cyc);
#else
#define trace_ppu(fmt, ...) (void)0
#endif

static u8 ppu_read_addr(u16 addr)
{
  addr &= 0x3FFF;
  if (addr < 0x2000) {
    return dev.nes->vrom[addr];
  } else if (addr < 0x3F00) {
    return dev.vram[addr & 0xFFF];
  } else {
    addr &= 0x1F;
    if (addr >= 0x10 && (addr & 0x03) == 0) {
      addr -= 0x10;
    }
    return dev.pram[addr];
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
    dev.pram[addr] = val;
  }
}

u8 ppu_bus_read(u16 addr, bool trace)
{
  if (!trace) {
    // trace_ppu("before ppu_bus_read %04X", addr);
  }
  switch (addr & 0x7) {
    case 0: return ppu.control0;
    case 1: return ppu.control1;
    case 2:
      if (trace) {
        return ppu.status;
      }
      u8 status = ppu.status;
      ppu.status &= ~(PPU_STATUS_VBLANK);
      ppu.write_latch = 0;
      if (!trace) {
        // trace_ppu("after  ppu_bus_read %04X", addr);
      }
      return status;
    case 4: return ((u8*)sprite)[ppu.sprite_addr]; // ++?
    case 7: {
      addr = ppu.addr;
      u8 val;
      if ((addr & 0x3FFF) < 0x3F00) {
        val = ppu.read_buf;
      } else {
        val = ppu_read_addr(addr);
        addr -= 0x1000;
      }
      if (trace) {
        return val;
      }
      ppu.read_buf = ppu_read_addr(addr);
      ppu.addr += (ppu.control0 & PPU_CONTROL0_VERT) ? 32 : 1;
      if (!trace) {
        // trace_ppu("after  ppu_bus_read %04X", addr);
      }
      return val;
    }
  }
  return 0; // openbus?
}

void ppu_bus_write(u16 addr, u8 val)
{
  // trace_ppu("before ppu_bus_write %04X %02X", addr, val);
  switch (addr & 0x7) {
    case 0: ppu.control0 = val; break;
    case 1: ppu.control1 = val; break;
    case 3: ppu.sprite_addr = val; break;
    case 4: ((u8*)sprite)[ppu.sprite_addr++] = val; break;
    case 5: ppu.scroll[1 & ppu.write_latch++] = val; break;
    case 6: ((u8*)&ppu.addr)[(1 & ppu.write_latch++) ? 0 : 1] = val; break;
    case 7:
      ppu_write_addr(ppu.addr, val);
      ppu.addr += (ppu.control0 & PPU_CONTROL0_VERT) ? 32 : 1;
      break;
  }
  // trace_ppu("after  ppu_bus_write %04X %02X", addr, val);
}

void ppu_tick()
{
  if (dev.ppu_sl == sprite[0].y) { // todo: opaque pixel check
    ppu.status |= PPU_STATUS_HIT; // todo: if rendering enabled in 2001
  }
  if (dev.ppu_sl == PPU_SL_VBLANK && dev.ppu_cyc == PPU_CYC_BEGIN + 10) {
    ppu.status |= PPU_STATUS_VBLANK;
  }
  if (dev.ppu_sl == PPU_SL_LAST && dev.ppu_cyc == PPU_CYC_BEGIN + 3) {
    ppu.status &= ~(PPU_STATUS_VBLANK | PPU_STATUS_HIT);
  }

  if ((ppu.status & PPU_STATUS_VBLANK) && (ppu.control0 & PPU_CONTROL0_NMI_ALLOWED)) {
    cpu_nmi();
  }

  if (++dev.ppu_cyc > PPU_CYC_LAST) {
    dev.ppu_cyc = 0;
    if (++dev.ppu_sl > PPU_SL_LAST) {
      dev.ppu_sl = 0;
      if ((++dev.ppu_frame) & 1) {
        ++dev.ppu_cyc; // todo: if rendering enabled in 2001
      }
#if MN_TRACE_BLARGG
      tracef("BLARGG %02X : %s\n", dev.sram[0], (const char*)&dev.sram[4]);
#endif
    }
  }
}
