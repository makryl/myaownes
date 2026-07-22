#include "apu.h"
#include "cpu.h"
#include "map.h"
#include "common.h"
#include <string.h>

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

static const uint apu_dmc_period_ntsc[16] = { 428, 380, 340, 320, 286, 254, 226, 214, //
                                              190, 160, 142, 128, 106, 84,  72,  54 };
static const uint apu_dmc_period_pal[16] = { 398, 354, 316, 298, 266, 236, 210, 198, //
                                             176, 148, 132, 118, 98,  78,  66,  50 };

// todo: Dendy 0 2 1 3?
static const uint apu_duty_table[4][8] = {
  { 0, 0, 0, 0, 0, 0, 0, 1 },
  { 0, 0, 0, 0, 0, 0, 1, 1 },
  { 0, 0, 0, 0, 1, 1, 1, 1 },
  { 1, 1, 1, 1, 1, 1, 0, 0 },
};

static const uint apu_triangle_table[32] = { 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5,  4,  3,  2,  1,  0,
                                             0,  1,  2,  3,  4,  5,  6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };

static const uint apu_noise_period_ntsc[16] = { 4,   8,   16,  32,  64,  96,   128,  160, //
                                                202, 254, 380, 508, 762, 1016, 2034, 4068 };
static const uint apu_noise_period_pal[16] = { 4,   8,   14,  30,  60,  88,  118,  148, //
                                               188, 236, 354, 472, 708, 944, 1890, 3778 };

MN_CACHE_LINE static struct Apu
{
  u64 time_target;
  u64 time_current;

  uint cyc;
  uint cyc_reset;

  uint step1;
  uint step2;
  uint step3;
  uint step4;
  uint step5;

  int mix_hp_x;
  int mix_hp_y;
  int mix_lp_y;
  uint mix_accum;
  uint mix_sample_count;
  uint dither;
  uint out_size;

  uint pulse1_len;
  uint pulse1_period;
  uint pulse1_timer;
  uint pulse1_duty;
  uint pulse1_phase;
  uint pulse1_env_vol;
  uint pulse1_env_timer;
  uint pulse1_env_decay;
  uint pulse1_sweep_period;
  uint pulse1_sweep_shift;
  uint pulse1_sweep_timer;

  uint pulse2_len;
  uint pulse2_period;
  uint pulse2_timer;
  uint pulse2_duty;
  uint pulse2_phase;
  uint pulse2_env_vol;
  uint pulse2_env_timer;
  uint pulse2_env_decay;
  uint pulse2_sweep_period;
  uint pulse2_sweep_shift;
  uint pulse2_sweep_timer;

  uint triangle_len;
  uint triangle_period;
  uint triangle_timer;
  uint triangle_phase;
  uint triangle_reload_val;
  uint triangle_linear_val;

  uint noise_len;
  uint noise_period;
  uint noise_timer;
  uint noise_shift;
  uint noise_env_vol;
  uint noise_env_timer;
  uint noise_env_decay;

  uint dmc_timer;
  uint dmc_sample_len;
  uint dmc_len;
  uint dmc_start;
  uint dmc_period;
  uint dmc_out_bit;
  uint dmc_out;
  uint dmc_val;
  u16 dmc_sample_addr;
  u16 dmc_addr;
  u8 dmc_buf;

  bool mode5;
  bool frame_irq_disabled;
  bool frame_irq;

  bool dmc_enabled;
  bool dmc_irq_enabled;
  bool dmc_irq;
  bool dmc_has_buf;
  bool dmc_loop;
  bool dmc_silence;

  bool pulse1_enabled;
  bool pulse1_halt;
  bool pulse1_dec;
  bool pulse1_env_const;
  bool pulse1_env_start;
  bool pulse1_sweep_enabled;
  bool pulse1_sweep_negate;
  bool pulse1_sweep_reload;

  bool pulse2_enabled;
  bool pulse2_halt;
  bool pulse2_dec;
  bool pulse2_env_const;
  bool pulse2_env_start;
  bool pulse2_sweep_enabled;
  bool pulse2_sweep_negate;
  bool pulse2_sweep_reload;

  bool triangle_enabled;
  bool triangle_halt;
  bool triangle_dec;
  bool triangle_linear_reload;

  bool noise_enabled;
  bool noise_halt;
  bool noise_dec;
  bool noise_mode;
  bool noise_env_const;
  bool noise_env_start;

  MN_CACHE_LINE uint noise_period_table[16];
  MN_CACHE_LINE uint dmc_period_table[16];
  MN_CACHE_LINE uint pulse_table[31];
  MN_CACHE_LINE uint tnd_table[16][16][128];

  MN_CACHE_LINE i16 out_data[1024];
} apu;

uint apu_size() { return sizeof(apu); }
void* apu_data() { return &apu; }
void* mn_audio_data() { return apu.out_data; }
uint mn_audio_size() { return apu.out_size * sizeof(i16); }
void apu_reset_out() { apu.out_size = 0; }

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

  Rom rom = mn_rom_get();

  if (rom->ntsc) {
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

  if (rom->ntsc) {
    apu.time_target = (1789773ULL << 32) / MN_AUDIO_FREQ;
  } else if (rom->pal) {
    apu.time_target = (1662607ULL << 32) / MN_AUDIO_FREQ;
  } else if (rom->dendy) {
    apu.time_target = (1773448ULL << 32) / MN_AUDIO_FREQ;
  }

  if (rom->ntsc || rom->dendy) {
    memcpy(apu.noise_period_table, apu_noise_period_ntsc, sizeof(apu.noise_period_table));
    memcpy(apu.dmc_period_table, apu_dmc_period_ntsc, sizeof(apu.dmc_period_table));
  } else {
    memcpy(apu.noise_period_table, apu_noise_period_pal, sizeof(apu.noise_period_table));
    memcpy(apu.dmc_period_table, apu_dmc_period_pal, sizeof(apu.dmc_period_table));
  }

  // scale table values to i16 and upscale to << 10, reserving 6 bit for accum sum (~40 samples)
  apu.pulse_table[0] = 0;
  for (uint i = 1; i < 31; ++i) {
    double val = 95.52 / (8128.0 / i + 100.0); // 95.88 or 95.52?
    apu.pulse_table[i] = (uint)(val * 32767.0 * 1024.0 + 0.5);
  }

  for (int t = 0; t < 16; ++t) {
    for (int n = 0; n < 16; ++n) {
      for (int d = 0; d < 128; ++d) {
        if (t == 0 && n == 0 && d == 0) {
          apu.tnd_table[t][n][d] = 0;
        } else {
          double val = 159.79 / ((1.0 / ((double)t / 8227.0 + (double)n / 12241.0 + (double)d / 22638.0)) + 100.0);
          apu.tnd_table[t][n][d] = (uint)(val * 32767.0 * 1024.0 + 0.5);
        }
      }
    }
  }

  apu.noise_shift = 1;
  apu.dither = 0xDEAD;
}

void apu_reset()
{
  apu.time_current = 0;
  apu.mix_accum = 0;
  apu.mix_sample_count = 0;

  apu.pulse1_len = 0;
  apu.pulse2_len = 0;
  apu.triangle_len = 0;
  apu.noise_len = 0;
  apu.dmc_len = 0;

  apu.dmc_val = 0;

  apu.pulse1_enabled = false;
  apu.pulse2_enabled = false;
  apu.triangle_enabled = false;
  apu.noise_enabled = false;
  apu.dmc_enabled = false;

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
    case 0x00:
      apu.pulse1_duty = (val >> 6);
      apu.pulse1_halt = (val & 0x20);
      apu.pulse1_env_const = (val & 0x10);
      apu.pulse1_env_vol = (val & 0x0F);
      break;
    case 0x01:
      apu.pulse1_sweep_enabled = (val & 0x80);
      apu.pulse1_sweep_period = (val >> 4) & 0x07;
      apu.pulse1_sweep_negate = (val & 0x08);
      apu.pulse1_sweep_shift = (val & 0x07);
      apu.pulse1_sweep_reload = true;
      break;
    case 0x02: apu.pulse1_period = (apu.pulse1_period & 0x0700) | val; break;
    case 0x03:
      apu.pulse1_period = (apu.pulse1_period & 0x00FF) | ((val & 0x07) << 8);
      apu.pulse1_phase = 0;
      apu.pulse1_env_start = true;
      if (!apu.pulse1_dec) {
        apu.pulse1_len = apu.pulse1_enabled ? apu_channel_len[val >> 3] : 0;
      }
      break;
    case 0x04:
      apu.pulse2_duty = (val >> 6);
      apu.pulse2_halt = (val & 0x20);
      apu.pulse2_env_const = (val & 0x10);
      apu.pulse2_env_vol = (val & 0x0F);
      break;
    case 0x05:
      apu.pulse2_sweep_enabled = (val & 0x80);
      apu.pulse2_sweep_period = (val >> 4) & 0x07;
      apu.pulse2_sweep_negate = (val & 0x08);
      apu.pulse2_sweep_shift = (val & 0x07);
      apu.pulse2_sweep_reload = true;
      break;
    case 0x06: apu.pulse2_period = (apu.pulse2_period & 0x0700) | val; break;
    case 0x07:
      apu.pulse2_period = (apu.pulse2_period & 0x00FF) | ((val & 0x07) << 8);
      apu.pulse2_phase = 0;
      apu.pulse2_env_start = true;
      if (!apu.pulse2_dec) {
        apu.pulse2_len = apu.pulse2_enabled ? apu_channel_len[val >> 3] : 0;
      }
      break;
    case 0x08:
      apu.triangle_halt = (val & 0x80);
      apu.triangle_reload_val = (val & 0x7F);
      break;
    case 0x0A: apu.triangle_period = (apu.triangle_period & 0x0700) | val; break;
    case 0x0B:
      apu.triangle_period = (apu.triangle_period & 0x00FF) | ((val & 0x07) << 8);
      apu.triangle_linear_reload = true;
      if (!apu.triangle_dec) {
        apu.triangle_len = apu.triangle_enabled ? apu_channel_len[val >> 3] : 0;
      }
      break;
    case 0x0C:
      apu.noise_halt = (val & 0x20);
      apu.noise_env_const = (val & 0x10);
      apu.noise_env_vol = (val & 0x0F);
      break;
    case 0x0E:
      apu.noise_mode = (val & 0x80);
      apu.noise_period = (val & 0x0F);
      break;
    case 0x0F:
      apu.noise_env_start = true;
      if (!apu.noise_dec) {
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
    case 0x11: apu.dmc_val = (val & 0x7F); break;
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

static uint apu_pulse1_sweep()
{
  uint delta = apu.pulse1_period >> apu.pulse1_sweep_shift;
  if (apu.pulse1_sweep_negate) {
    return apu.pulse1_period > delta ? apu.pulse1_period - delta - 1 : 0;
  } else {
    return apu.pulse1_period + delta;
  }
}

static uint apu_pulse2_sweep()
{
  uint delta = apu.pulse2_period >> apu.pulse2_sweep_shift;
  if (apu.pulse2_sweep_negate) {
    return apu.pulse2_period > delta ? apu.pulse2_period - delta : 0;
  } else {
    return apu.pulse2_period + delta;
  }
}

static void apu_pulse1_tick()
{
  if (apu.pulse1_timer == 0) {
    apu.pulse1_timer = apu.pulse1_period;
    apu.pulse1_phase = (apu.pulse1_phase - 1) & 7;
  } else {
    --apu.pulse1_timer;
  }
}

static void apu_pulse2_tick()
{
  if (apu.pulse2_timer == 0) {
    apu.pulse2_timer = apu.pulse2_period;
    apu.pulse2_phase = (apu.pulse2_phase - 1) & 7;
  } else {
    --apu.pulse2_timer;
  }
}

static void apu_triangle_tick()
{
  if (apu.triangle_timer == 0) {
    apu.triangle_timer = apu.triangle_period;
    if (apu.triangle_len > 0 && apu.triangle_linear_val > 0 && apu.triangle_period > 2) {
      apu.triangle_phase = (apu.triangle_phase + 1) & 31;
    }
  } else {
    --apu.triangle_timer;
  }
}

static void apu_noise_tick()
{
  if (apu.noise_timer == 0) {
    apu.noise_timer = apu.noise_period_table[apu.noise_period];
    uint bit1 = apu.noise_shift & 1;
    uint bit2 = (apu.noise_mode) ? ((apu.noise_shift >> 6) & 1) : ((apu.noise_shift >> 1) & 1);
    uint feedback = bit1 ^ bit2;
    apu.noise_shift >>= 1;
    apu.noise_shift |= (feedback << 14);
  } else {
    --apu.noise_timer;
  }
}

static void apu_dmc_tick()
{
  if (apu.dmc_timer == 0) {
    apu.dmc_timer = apu.dmc_period_table[apu.dmc_period];
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
    if (!apu.dmc_silence) {
      if (apu.dmc_out & 1) {
        if (apu.dmc_val <= 125) {
          apu.dmc_val += 2;
        }
      } else {
        if (apu.dmc_val >= 2) {
          apu.dmc_val -= 2;
        }
      }
    }
    apu.dmc_out >>= 1;
    --apu.dmc_out_bit;
  }
  --apu.dmc_timer;

  if (apu.dmc_start > 0) {
    if (--apu.dmc_start == 0 && !apu.dmc_has_buf && apu.dmc_len > 0) {
      apu_trace_dmc("APU DMC START=%04X ", apu.dmc_addr);
      cpu_dmc(apu.dmc_addr);
    }
  }
}

static uint apu_pulse1_sample()
{
  if (apu.pulse1_len == 0 || apu.pulse1_period < 8 || apu_pulse1_sweep() > 0x07FF) {
    return 0;
  }
  uint sample = apu_duty_table[apu.pulse1_duty][apu.pulse1_phase];
  uint volume = apu.pulse1_env_const ? apu.pulse1_env_vol : apu.pulse1_env_decay;
  return sample * volume;
}

static uint apu_pulse2_sample()
{
  if (apu.pulse2_len == 0 || apu.pulse2_period < 8 || apu_pulse2_sweep() > 0x07FF) {
    return 0;
  }
  uint sample = apu_duty_table[apu.pulse2_duty][apu.pulse2_phase];
  uint volume = apu.pulse2_env_const ? apu.pulse2_env_vol : apu.pulse2_env_decay;
  return sample * volume;
}

static uint apu_triangle_sample() { return apu_triangle_table[apu.triangle_phase]; }

static uint apu_noise_sample()
{
  if (apu.noise_len == 0) {
    return 0;
  }
  uint sample = !(apu.noise_shift & 1);
  uint volume = apu.noise_env_const ? apu.noise_env_vol : apu.noise_env_decay;
  return sample * volume;
}

static uint apu_dmc_sample() { return apu.dmc_val; }

static void apu_update_len_get()
{
  apu.pulse1_dec = !apu.pulse1_halt && apu.pulse1_len > 0;
  apu.pulse2_dec = !apu.pulse2_halt && apu.pulse2_len > 0;
  apu.triangle_dec = !apu.triangle_halt && apu.triangle_len > 0;
  apu.noise_dec = !apu.noise_halt && apu.noise_len > 0;
}

static void apu_update_len_put()
{
  if (apu.pulse1_dec) {
    apu.pulse1_dec = false;
    --apu.pulse1_len;
  }
  if (apu.pulse2_dec) {
    apu.pulse2_dec = false;
    --apu.pulse2_len;
  }
  if (apu.triangle_dec) {
    apu.triangle_dec = false;
    --apu.triangle_len;
  }
  if (apu.noise_dec) {
    apu.noise_dec = false;
    --apu.noise_len;
  }

  if (apu.pulse1_sweep_timer == 0 && apu.pulse1_sweep_enabled && apu.pulse1_sweep_shift > 0) {
    apu.pulse1_period = apu_pulse1_sweep();
  }
  if (apu.pulse1_sweep_timer == 0 || apu.pulse1_sweep_reload) {
    apu.pulse1_sweep_timer = apu.pulse1_sweep_period;
    apu.pulse1_sweep_reload = false;
  } else {
    --apu.pulse1_sweep_timer;
  }

  if (apu.pulse2_sweep_timer == 0 && apu.pulse2_sweep_enabled && apu.pulse2_sweep_shift > 0) {
    apu.pulse2_period = apu_pulse2_sweep();
  }
  if (apu.pulse2_sweep_timer == 0 || apu.pulse2_sweep_reload) {
    apu.pulse2_sweep_timer = apu.pulse2_sweep_period;
    apu.pulse2_sweep_reload = false;
  } else {
    --apu.pulse2_sweep_timer;
  }
}

static void apu_update_env()
{
  if (apu.pulse1_env_start) {
    apu.pulse1_env_start = false;
    apu.pulse1_env_decay = 15;
    apu.pulse1_env_timer = apu.pulse1_env_vol;
  } else if (apu.pulse1_env_timer == 0) {
    apu.pulse1_env_timer = apu.pulse1_env_vol;
    if (apu.pulse1_env_decay > 0) {
      --apu.pulse1_env_decay;
    } else if (apu.pulse1_halt) {
      apu.pulse1_env_decay = 15;
    }
  } else {
    --apu.pulse1_env_timer;
  }

  if (apu.pulse2_env_start) {
    apu.pulse2_env_start = false;
    apu.pulse2_env_decay = 15;
    apu.pulse2_env_timer = apu.pulse2_env_vol;
  } else if (apu.pulse2_env_timer == 0) {
    apu.pulse2_env_timer = apu.pulse2_env_vol;
    if (apu.pulse2_env_decay > 0) {
      --apu.pulse2_env_decay;
    } else if (apu.pulse2_halt) {
      apu.pulse2_env_decay = 15;
    }
  } else {
    --apu.pulse2_env_timer;
  }

  if (apu.triangle_linear_reload) {
    apu.triangle_linear_val = apu.triangle_reload_val;
  } else if (apu.triangle_linear_val > 0) {
    --apu.triangle_linear_val;
  }
  if (!apu.triangle_halt) {
    apu.triangle_linear_reload = false;
  }

  if (apu.noise_env_start) {
    apu.noise_env_start = false;
    apu.noise_env_decay = 15;
    apu.noise_env_timer = apu.noise_env_vol;
  } else if (apu.noise_env_timer == 0) {
    apu.noise_env_timer = apu.noise_env_vol;
    if (apu.noise_env_decay > 0) {
      --apu.noise_env_decay;
    } else if (apu.noise_halt) {
      apu.noise_env_decay = 15;
    }
  } else {
    --apu.noise_env_timer;
  }
}

#define APU_PI 3.141592653589793238462643383279502884
#define apu_filter_alpha(freq) (1.0 / (1.0 + (2.0 * APU_PI * (freq)) / MN_AUDIO_FREQ))
static const i64 apu_hp10_alpha = (i64)(apu_filter_alpha(10.0) * 65536.0 + 0.5);
static const i64 apu_lp14k_alpha = (i64)((1.0 - apu_filter_alpha(14000.0)) * 65536.0 + 0.5);

static int apu_mix_high_pass(int sample)
{
  apu.mix_hp_y = (apu_hp10_alpha * ((i64)apu.mix_hp_y + (i64)sample - (i64)apu.mix_hp_x)) >> 16;
  apu.mix_hp_x = sample;
  return apu.mix_hp_y;
}

static int apu_mix_low_pass(int sample)
{
  apu.mix_lp_y = apu.mix_lp_y + ((apu_lp14k_alpha * ((i64)sample - (i64)apu.mix_lp_y)) >> 16);
  return apu.mix_lp_y;
}

static uint apu_xorshift32()
{
  uint x = apu.dither;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  apu.dither = x;
  return x;
}

static int apu_mix_dither(int sample)
{
  int rand1 = apu_xorshift32() & 0xFFFF;
  int rand2 = apu_xorshift32() & 0xFFFF;
  int dither_noise = rand1 - rand2;
  return sample + dither_noise;
}

static int apu_mix_clip(int sample)
{
  if (sample > 32767) {
    sample = 32767;
  } else if (sample < -32768) {
    sample = -32768;
  }
  return sample;
}

static void apu_mix()
{
  uint pulse = apu.pulse_table[apu_pulse1_sample() + apu_pulse2_sample()];
  uint tnd = apu.tnd_table[apu_triangle_sample()][apu_noise_sample()][apu_dmc_sample()];
  apu.mix_accum += pulse + tnd;
  ++apu.mix_sample_count;

  apu.time_current += (1ULL << 32);
  if (apu.time_current >= apu.time_target) {
    if (apu.out_size <= 1024) {
      // mix_accum already has << 10 in table values, upsample to 15, reserve 1 bit for filters
      int sample = (apu.mix_accum / apu.mix_sample_count) << 5;
      sample = apu_mix_high_pass(sample);
      sample = apu_mix_low_pass(sample);
      sample = apu_mix_dither(sample);
      sample = apu_mix_clip(sample >> 16);
      apu.out_data[apu.out_size++] = sample;
    }
    apu.mix_accum = 0;
    apu.mix_sample_count = 0;
    apu.time_current -= apu.time_target;
  }
}

void apu_tick()
{
  if (apu.cyc_reset > 0) {
    if (--apu.cyc_reset == 0) {
      apu.cyc = 0;
      if (apu.mode5) {
        apu_update_len_get();
        apu_update_len_put();
      }
    }
  }

  if (apu.cyc & 1) {
    apu_pulse1_tick();
    apu_pulse2_tick();
    apu_noise_tick();
  }
  apu_triangle_tick();
  apu_dmc_tick();
  apu_mix();

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
