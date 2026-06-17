#include "dev.h"
#include "myanes.h"
#include "cpu.h"
#include "ppu.h"
#include "apu.h"
#include "common.h"

static struct
{
  u8* joy[2];
  u32* out;
} dev;

void mn_input(u8* joy1, u8* joy2)
{
  dev.joy[0] = joy1;
  dev.joy[1] = joy2;
}

void mn_output(u32* out) { dev.out = out; }

void mn_power()
{
  cpu_power();
  ppu_power();
  apu_power();
}

void mn_reset()
{
  cpu_reset();
  ppu_reset();
  apu_reset();
}

void mn_frame()
{
  bool vblank_before;
  do {
    vblank_before = ppu_is_vblank();
    cpu_tick();
  } while (vblank_before || !ppu_is_vblank());
#if MN_TRACE_BLARGG
  tracef("\e[2J\e[Hstatus=%02X\n%s\n", *mn_prg_ram(), (const char*)(mn_prg_ram() + 4));
#endif
}

u8 dev_input(u8 idx) { return *dev.joy[idx]; }
void dev_output(u16 idx, u32 color) { dev.out[idx] = color; }
