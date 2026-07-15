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

static float dev_aspect_scale(bool a) { return (256.0f / 240.0f) * (a ? (8.0f / 7.0f) : 1.0f); }
static float dev_aspect_overscan(bool a) { return a ? (4.0f / 3.0f) : (256.0f / 224.0f); }
static float dev_aspect_target(bool a, bool o) { return o ? dev_aspect_overscan(a) : dev_aspect_scale(a); }

void mn_output_size(bool auto_aspect, bool overscan, float scale, int* dw, int* dh)
{
  float h = (overscan ? 224.0f : 240.0f) * scale;
  float w = h * dev_aspect_target(auto_aspect, overscan);
  *dw = (int)(w + 0.5f);
  *dh = (int)(h + 0.5f);
}

void mn_output_fit(bool auto_aspect, bool overscan, float sw, float sh, float* dx, float* dy, float* dw, float* dh)
{
  float aspect_scale = dev_aspect_scale(auto_aspect);
  float aspect_target = dev_aspect_target(auto_aspect, overscan);
  float aspect_output = sw / sh;
  if (aspect_target <= aspect_output) {
    *dh = sh * aspect_target / aspect_scale;
    *dw = *dh * aspect_scale;
  } else {
    *dw = sw;
    *dh = *dw / aspect_scale;
  }
  *dx = (sw - *dw) / 2.0f;
  *dy = (sh - *dh) / 2.0f;
}
