#include "dev.h"
#include "myaownes.h"
#include "cpu.h"
#include "ppu.h"
#include "apu.h"
#include "map.h"
#include "common.h"

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
  tracef("\e[2J\e[Hstatus=%02X\n%s\n", *mn_prg_ram(), (const char*)(mn_prg_ram() + 4));
#endif
}

u8 dev_input(u8 idx) { return dev.joy[idx]; }
void dev_output(u16 idx, u32 color) { dev.out[idx] = color; }
u32* mn_output() { return dev.out; }

void mn_output_rect(bool overscan, float sw, float sh, float* dx, float* dy, float* dw, float* dh)
{
  const float aspect = (256.0f * 8.0f) / (240.0f * 7.0f);
  float src_aspect = sw / sh;
  float target_aspect = overscan ? (4.0f / 3.0f) : aspect;
  if (target_aspect <= src_aspect) {
    *dh = overscan ? (sh * (240.0f * 7.0f * 4.0f) / (256.0f * 8.0f * 3.0f)) : sh;
    *dw = *dh * aspect;
  } else {
    *dw = sw;
    *dh = *dw / aspect;
  }
  *dx = (sw - *dw) / 2.0f;
  *dy = (sh - *dh) / 2.0f;
}
