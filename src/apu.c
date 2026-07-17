#include "apu.h"
#include "cpu.h"
#include "map.h"
#include "common.h"
#include <string.h>

enum
{
  APU_STEP_NTSC = 3728,
  APU_STEP_PAL = 8313,
};

enum : u8
{
  APU_STATUS_PULSE1 = (1 << 0),
  APU_STATUS_PULSE2 = (1 << 1),
  APU_STATUS_TRIANGLE = (1 << 2),
  APU_STATUS_NOISE = (1 << 3),
  APU_STATUS_DMC = (1 << 4),
  APU_STATUS_FRAME_IRQ = (1 << 6),
  APU_STATUS_DMC_IRQ = (1 << 7),
};

static const uint apu_channel_len[32] = { 10, 254, 20, 2,  40, 4,  80, 6,  160, 8,  60, 10, 14, 12, 26, 14,
                                          12, 16,  24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30 };

static const uint apu_dmc_period[16] = { 428, 380, 340, 320, 286, 254, 226, 214, 190, 160, 142, 128, 106, 84, 72, 54 };

MN_CACHE_LINE static struct Apu
{
  uint cyc;

  uint step1;
  uint step2;
  uint step3;
  uint step4;
  uint step5;

  uint cyc_reset;

  uint pulse1_len;
  uint pulse2_len;
  uint triangle_len;
  uint noise_len;

  uint dmc_timer;
  uint dmc_sample_len;
  uint dmc_len;
  uint dmc_start;
  uint dmc_period;
  uint dmc_out_bit;
  u16 dmc_sample_addr;
  u16 dmc_addr;
  u8 dmc_buf;
  u8 dmc_out;

  bool mode5;
  bool frame_irq_disabled;
  bool frame_irq;
  bool dmc_irq_enabled;
  bool dmc_irq;
  bool dmc_has_buf;
  bool dmc_loop;
  bool dmc_silence;

  bool pulse1_enabled;
  bool pulse2_enabled;
  bool triangle_enabled;
  bool noise_enabled;
  bool dmc_enabled;

  bool halt_pulse1;
  bool halt_pulse2;
  bool halt_triangle;
  bool halt_noise;

  bool dec_pulse1;
  bool dec_pulse2;
  bool dec_triangle;
  bool dec_noise;
} apu;

uint apu_size() { return sizeof(apu); }
void* apu_data() { return &apu; }

static u8 apu_get_status()
{
  u8 status = 0;
  if (apu.pulse1_len > 0) {
    status |= APU_STATUS_PULSE1;
  }
  if (apu.pulse2_len > 0) {
    status |= APU_STATUS_PULSE2;
  }
  if (apu.triangle_len > 0) {
    status |= APU_STATUS_TRIANGLE;
  }
  if (apu.noise_len > 0) {
    status |= APU_STATUS_NOISE;
  }
  if (apu.dmc_len > 0) {
    status |= APU_STATUS_DMC;
  }
  if (!apu.frame_irq_disabled && apu.frame_irq) {
    status |= APU_STATUS_FRAME_IRQ;
  }
  if (apu.dmc_irq_enabled && apu.dmc_irq) {
    status |= APU_STATUS_DMC_IRQ;
  }
  return status;
}

#if MN_TRACE_APU // todo: keep only apu_trace
#define apu_trace(fmt, ...)                                                                                     \
  tracef("APU phase=%s cpu_cyc=%-10d apu_half_cyc=%-10d status=%02X  " fmt "\n", cpu_cyc() & 1 ? "PUT" : "GET", \
         cpu_cyc(), apu.cyc, apu_get_status() __VA_OPT__(, ) __VA_ARGS__)
#define apu_trace_dmc(fmt, ...)                                                      \
  tracef(fmt                                                                         \
         " | %s | CPU CYC:%10d | APU HALF CYC:%10d | dmc_len:%d | dmc_has_buf:%d | " \
         "dmc_out_bit:%d\n" __VA_OPT__(, ) __VA_ARGS__,                              \
         cpu_cyc() & 1 ? "PUT" : "GET", cpu_cyc(), apu.cyc, apu.dmc_len, apu.dmc_has_buf, apu.dmc_out_bit)
#else
#define apu_trace(fmt, ...) (void)0
#define apu_trace_dmc(fmt, ...) (void)0
#endif

void apu_power()
{
  memset(&apu, 0, sizeof(apu));

  // todo: load rom
  bool ntsc = true;
  if (ntsc) {
    apu.step1 = 3728 * 2;
    apu.step2 = 7456 * 2;
    apu.step3 = 11185 * 2;
    apu.step4 = 14914 * 2;
    apu.step5 = 18640 * 2;
  } else {
    apu.step1 = 4156 * 2;
    apu.step2 = 8313 * 2;
    apu.step3 = 12469 * 2;
    apu.step4 = 16626 * 2;
    apu.step5 = 20782 * 2;
  }
}

void apu_reset()
{
  apu.pulse1_enabled = false;
  apu.pulse2_enabled = false;
  apu.triangle_enabled = false;
  apu.noise_enabled = false;
  apu.dmc_enabled = false;

  apu.pulse1_len = 0;
  apu.pulse2_len = 0;
  apu.triangle_len = 0;
  apu.noise_len = 0;
  apu.dmc_len = 0;

  apu.frame_irq = false;
  apu.dmc_irq = false;

  apu.cyc_reset = (cpu_cyc() & 1) ? 3 : 4;
}

void apu_bus_read(u16 addr, u8* val, bool trace)
{
  switch (addr & 0x1F) {
    case 0x15: {
      u8 status = apu_get_status();
      if (!trace) {
        apu.frame_irq = false;
        // apu_trace_dmc("APU READ $4015=%02X  ", status);
      }
      *val = status;
    }
  }
}

void apu_bus_write(u16 addr, u8 val)
{
  switch (addr & 0x1F) {
    case 0x00: apu.halt_pulse1 = (val & 0x20); break;
    case 0x04: apu.halt_pulse2 = (val & 0x20); break;
    case 0x08: apu.halt_triangle = (val & 0x80); break;
    case 0x0C: apu.halt_noise = (val & 0x20); break;
    case 0x03:
      if (!apu.dec_pulse1) {
        apu.pulse1_len = apu.pulse1_enabled ? apu_channel_len[val >> 3] : 0;
      }
      break;
    case 0x07:
      if (!apu.dec_pulse2) {
        apu.pulse2_len = apu.pulse2_enabled ? apu_channel_len[val >> 3] : 0;
      }
      break;
    case 0x0B:
      if (!apu.dec_triangle) {
        apu.triangle_len = apu.triangle_enabled ? apu_channel_len[val >> 3] : 0;
      }
      break;
    case 0x0F:
      if (!apu.dec_noise) {
        apu.noise_len = apu.noise_enabled ? apu_channel_len[val >> 3] : 0;
      }
      break;
    case 0x10:
      apu.dmc_loop = (val & 0x40);
      apu.dmc_period = (val & 0x0F);
      apu.dmc_irq_enabled = (val & 0x80);
      if (!apu.dmc_irq_enabled) {
        apu.dmc_irq = false;
      }
      break;
    case 0x12: apu.dmc_sample_addr = 0xC000 + (val * 64); break;
    case 0x13: apu.dmc_sample_len = (val * 16) + 1; break;
    case 0x15: {
      apu_trace_dmc("APU WRITE $4015=%02X ", val);
      apu.pulse1_enabled = (val & APU_STATUS_PULSE1);
      apu.pulse2_enabled = (val & APU_STATUS_PULSE2);
      apu.triangle_enabled = (val & APU_STATUS_TRIANGLE);
      apu.noise_enabled = (val & APU_STATUS_NOISE);
      apu.dmc_enabled = (val & APU_STATUS_DMC);
      if (!apu.pulse1_enabled) {
        apu.pulse1_len = 0;
      }
      if (!apu.pulse2_enabled) {
        apu.pulse2_len = 0;
      }
      if (!apu.triangle_enabled) {
        apu.triangle_len = 0;
      }
      if (!apu.noise_enabled) {
        apu.noise_len = 0;
      }
      if (!apu.dmc_enabled) {
        apu.dmc_len = 0;
      } else if (apu.dmc_len == 0) {
        apu.dmc_addr = apu.dmc_sample_addr;
        apu.dmc_len = apu.dmc_sample_len;
        apu.dmc_start = (cpu_cyc() & 1) ? 2 : 3;
      }
      apu.dmc_irq = false;
      break;
    }
    case 0x17: {
      apu.mode5 = (val & 0x80);
      apu.frame_irq_disabled = (val & 0x40);
      if (apu.frame_irq_disabled) {
        apu.frame_irq = false;
      }
      apu.cyc_reset = (cpu_cyc() & 1) ? 3 : 4;
      break;
    }
  }
}

void apu_dmc_dma(u8 val)
{
  apu_trace_dmc("APU DMC DMA=%02X     ", val);
  apu.dmc_buf = val;
  apu.dmc_has_buf = true;
  apu.dmc_addr = 0x8000 | (apu.dmc_addr + 1);

  if (apu.dmc_len > 0) {
    if (--apu.dmc_len == 0) {
      if (apu.dmc_loop) {
        apu.dmc_addr = apu.dmc_sample_addr;
        apu.dmc_len = apu.dmc_sample_len;
      } else {
        apu.dmc_irq = true;
      }
    }
  }
}

static void apu_update_len_get()
{
  apu.dec_pulse1 = !apu.halt_pulse1 && apu.pulse1_len > 0;
  apu.dec_pulse2 = !apu.halt_pulse2 && apu.pulse2_len > 0;
  apu.dec_triangle = !apu.halt_triangle && apu.triangle_len > 0;
  apu.dec_noise = !apu.halt_noise && apu.noise_len > 0;
}

static void apu_update_len_put()
{
  if (apu.dec_pulse1) {
    apu.dec_pulse1 = false;
    --apu.pulse1_len;
  }
  if (apu.dec_pulse2) {
    apu.dec_pulse2 = false;
    --apu.pulse2_len;
  }
  if (apu.dec_triangle) {
    apu.dec_triangle = false;
    --apu.triangle_len;
  }
  if (apu.dec_noise) {
    apu.dec_noise = false;
    --apu.noise_len;
  }
}

static void apu_update_env()
{
  //
}

void apu_tick()
{
  if (apu.dmc_timer == 0) {
    apu.dmc_timer = apu_dmc_period[apu.dmc_period];
    if (apu.dmc_out_bit == 0) {
      if (apu.dmc_has_buf) {
        apu_trace_dmc("APU DMC SHIFT=%04X ", apu.dmc_addr);
        apu.dmc_out = apu.dmc_buf;
        apu.dmc_has_buf = false;
        apu.dmc_silence = false;
        if (apu.dmc_len > 0) {
          cpu_dmc(apu.dmc_addr);
        }
      } else {
        if (!apu.dmc_silence) {
          apu_trace_dmc("APU DMC SILENCE    ");
        }
        apu.dmc_silence = true;
      }
      apu.dmc_out_bit = 8;
    }
    --apu.dmc_out_bit;
  }
  --apu.dmc_timer;

  if (apu.dmc_start > 0) {
    if (--apu.dmc_start == 0 && !apu.dmc_has_buf && apu.dmc_len > 0) {
      apu_trace_dmc("APU DMC START=%04X ", apu.dmc_addr);
      cpu_dmc(apu.dmc_addr);
    }
  }

  if (apu.cyc_reset > 0) {
    if (--apu.cyc_reset == 0) {
      apu.cyc = 0;
      if (apu.mode5) {
        apu_update_len_get();
        apu_update_len_put();
      }
    }
  }

  uint last_step = (apu.mode5 ? apu.step5 : apu.step4);

  if (apu.cyc == apu.step1 + 1 || apu.cyc == apu.step2 + 1 || apu.cyc == apu.step3 + 1 || apu.cyc == last_step + 1) {
    apu_update_env();
    apu_trace("env");
  }
  if (apu.cyc == apu.step2 || apu.cyc == last_step) {
    apu_update_len_get();
  }
  if (apu.cyc == apu.step2 + 1 || apu.cyc == last_step + 1) {
    apu_update_len_put();
    apu_trace("len");
  }
  if (apu.cyc == last_step || apu.cyc == last_step + 1 || apu.cyc == last_step + 2) {
    if (!apu.mode5) {
      apu.frame_irq = true;
      apu_trace("frame irq");
    }
  }
  if (apu.cyc == last_step + 2) {
    apu.cyc = 0;
  }

  map_apu_irq((!apu.frame_irq_disabled && apu.frame_irq) || (apu.dmc_irq_enabled && apu.dmc_irq));

  ++apu.cyc;
}
