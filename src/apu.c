#include "apu.h"
#include "cpu.h"
#include "map.h"
#include "common.h"

#define APU_MIX_LUT_APPROX 0

enum
{
  APU_FILTER_HP = 10,
  APU_FILTER_LP = 14000,
  APU_FILTER_DITHER = 1,
  APU_FILTER_DMC = 0, // todo: make optional, nice pop filter, but fails apu_mixer tests
};

enum
{
  APU_STATUS_PULSE1 = (1 << 0),
  APU_STATUS_PULSE2 = (1 << 1),
  APU_STATUS_TRIANGLE = (1 << 2),
  APU_STATUS_NOISE = (1 << 3),
  APU_STATUS_DMC = (1 << 4),
  APU_STATUS_OPENBUS = (1 << 5),
  APU_STATUS_FRAME_IRQ = (1 << 6),
  APU_STATUS_DMC_IRQ = (1 << 7),
};

enum
{
  APU_STEP1_NTSC = 3728 * 2,
  APU_STEP2_NTSC = 7456 * 2,
  APU_STEP3_NTSC = 11185 * 2,
  APU_STEP4_NTSC = 14914 * 2,
  APU_STEP5_NTSC = 18640 * 2,

  APU_STEP1_PAL = 4156 * 2,
  APU_STEP2_PAL = 8313 * 2,
  APU_STEP3_PAL = 12469 * 2,
  APU_STEP4_PAL = 16626 * 2,
  APU_STEP5_PAL = 20782 * 2,
};

enum
{
  APU_FREQ_NTSC = 1789773,
  APU_FREQ_PAL = 1662607,
  APU_FREQ_DENDY = 1773448,
};

static const uint apu_channel_len[32] = { 10, 254, 20, 2,  40, 4,  80, 6,  160, 8,  60, 10, 14, 12, 26, 14,
                                          12, 16,  24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30 };

static const uint apu_dmc_period_ntsc[16] = { 428, 380, 340, 320, 286, 254, 226, 214, //
                                              190, 160, 142, 128, 106, 84,  72,  54 };
static const uint apu_dmc_period_pal[16] = { 398, 354, 316, 298, 266, 236, 210, 198, //
                                             176, 148, 132, 118, 98,  78,  66,  50 };

static const uint apu_duty_table[4] = {
  0b10000000,
  0b11000000,
  0b11110000,
  0b00111111,
};

static const uint apu_triangle_table[32] = { 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5,  4,  3,  2,  1,  0,
                                             0,  1,  2,  3,  4,  5,  6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };

static const uint apu_noise_period_ntsc[16] = { 4,   8,   16,  32,  64,  96,   128,  160, //
                                                202, 254, 380, 508, 762, 1016, 2034, 4068 };
static const uint apu_noise_period_pal[16] = { 4,   8,   14,  30,  60,  88,  118,  148, //
                                               188, 236, 354, 472, 708, 944, 1890, 3778 };

static uint apu_freq = 48000;

static struct
{
  i16 out_data[1024];
#if APU_MIX_LUT_APPROX
  uint tnd_table[204]; // 203
#else
  uint tnd_table[16 * 16 * 128];
#endif
  uint pulse_table[32]; // 31
  uint dmc_period_table[16];
  uint noise_period_table[16];
  uint duty_table[4];
  u64 time_target;
  i64 mix_hp_alpha;
  i64 mix_lp_alpha;
} apu_dyn;

static struct Apu
{
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
  uint sample;

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
  uint pulse1_val;

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
  uint pulse2_val;

  uint triangle_len;
  uint triangle_period;
  uint triangle_timer;
  uint triangle_phase;
  uint triangle_reload_val;
  uint triangle_linear_val;
  uint triangle_val;

  uint noise_len;
  uint noise_period;
  uint noise_timer;
  uint noise_shift;
  uint noise_env_vol;
  uint noise_env_timer;
  uint noise_env_decay;
  uint noise_val;

  uint dmc_timer;
  uint dmc_sample_len;
  uint dmc_len;
  uint dmc_load;
  uint dmc_reload;
  uint dmc_stop;
  uint dmc_period;
  uint dmc_out_bit;
  uint dmc_out;
  uint dmc_val;
  uint dmc_val_pending;
  uint dmc_val_filtered;
  uint dmc_wait_buf;
  uint dmc_sample_addr;
  uint dmc_addr;
  uint dmc_buf;

  bool test_mode: 1;
  bool mode5: 1;
  bool frame_irq_status: 1;
  bool frame_irq_disabled: 1;
  bool frame_irq: 1;
  bool step_env: 1;
  bool step_len: 1;
  bool sample_dirty: 1;
  bool irq_dirty: 1;

  bool dmc_enabled: 1;
  bool dmc_irq_enabled: 1;
  bool dmc_irq: 1;
  bool dmc_has_buf: 1;
  bool dmc_has_out: 1;
  bool dmc_loop: 1;
  bool dmc_val_changed: 1;

  bool pulse1_enabled: 1;
  bool pulse1_halt: 1;
  bool pulse1_dec: 1;
  bool pulse1_env_const: 1;
  bool pulse1_env_start: 1;
  bool pulse1_sweep_enabled: 1;
  bool pulse1_sweep_negate: 1;
  bool pulse1_sweep_reload: 1;

  bool pulse2_enabled: 1;
  bool pulse2_halt: 1;
  bool pulse2_dec: 1;
  bool pulse2_env_const: 1;
  bool pulse2_env_start: 1;
  bool pulse2_sweep_enabled: 1;
  bool pulse2_sweep_negate: 1;
  bool pulse2_sweep_reload: 1;

  bool triangle_enabled: 1;
  bool triangle_halt: 1;
  bool triangle_dec: 1;
  bool triangle_linear_reload: 1;

  bool noise_enabled: 1;
  bool noise_halt: 1;
  bool noise_dec: 1;
  bool noise_mode: 1;
  bool noise_env_const: 1;
  bool noise_env_start: 1;
} apu;

uint apu_size() { return sizeof(apu); }
void* apu_data() { return &apu; }
i16* mn_audio_data() { return apu_dyn.out_data; }
uint mn_audio_size() { return apu.out_size * sizeof(i16); }
void apu_reset_out() { apu.out_size = 0; }
void mn_apu_test_mode(bool enabled) { apu.test_mode = enabled; }
static bool apu_is_put_phase() { return (cpu_cyc() & 1); }
static bool apu_is_get_phase() { return !apu_is_put_phase(); }

#define APU_PI 3.141592653589793238462643383279502884
static float apu_filter_alpha(uint ff, uint of) { return 1.0 / (1.0 + (2.0 * APU_PI * ff) / of); }
static i64 apu_hp_alpha(uint of) { return (i64)(apu_filter_alpha(APU_FILTER_HP, of) * 65536.0 + 0.5); }
static i64 apu_lp_alpha(uint of) { return (i64)((1.0 - apu_filter_alpha(APU_FILTER_LP, of)) * 65536.0 + 0.5); }

void mn_audio_freq(uint freq)
{
  apu_freq = freq;
  apu_dyn.mix_hp_alpha = apu_hp_alpha(freq);
  apu_dyn.mix_lp_alpha = apu_lp_alpha(freq);
  switch (mn_region_get()) {
    case MN_REGION_NTSC: apu_dyn.time_target = ((u64)APU_FREQ_NTSC << 32) / freq; break;
    case MN_REGION_PAL: apu_dyn.time_target = ((u64)APU_FREQ_PAL << 32) / freq; break;
    case MN_REGION_DENDY: apu_dyn.time_target = ((u64)APU_FREQ_DENDY << 32) / freq; break;
  }
}

static uint apu_get_status()
{
  uint status = 0;
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
  if (apu.frame_irq_status) {
    status |= APU_STATUS_FRAME_IRQ;
  }
  if (apu.dmc_irq) {
    status |= APU_STATUS_DMC_IRQ;
  }
  return status;
}

#if MN_TRACE_APU
#define apu_trace(fmt, ...)                                                                                          \
  tracef("APU phase=%s status=%02X apu_half_cyc=%-10d cpu_cyc=%-10d  " fmt "\n", apu_is_put_phase() ? "PUT" : "GET", \
         apu_get_status(), apu.cyc, cpu_cyc() __VA_OPT__(, ) __VA_ARGS__)
#define apu_trace_dmc(fmt, ...)                                                                                   \
  apu_trace(fmt " dmc_len=%d dmc_has_buf=%d dmc_out_bit=%d dmc_timer=%d" __VA_OPT__(, ) __VA_ARGS__, apu.dmc_len, \
            apu.dmc_has_buf, apu.dmc_out_bit, apu.dmc_timer)
#else
#define apu_trace(fmt, ...) (void)0
#define apu_trace_dmc(fmt, ...) (void)0
#endif

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

static bool apu_sweep_mute(uint current_period, uint target_period)
{
  return current_period < 8 || target_period > 0x07FF;
}

static uint apu_pulse1_sample()
{
  if (apu.pulse1_len == 0 || apu_sweep_mute(apu.pulse1_period, apu_pulse1_sweep())) {
    return 0;
  }
  uint sample = (apu_dyn.duty_table[apu.pulse1_duty] >> apu.pulse1_phase) & 1;
  uint volume = apu.pulse1_env_const ? apu.pulse1_env_vol : apu.pulse1_env_decay;
  return sample ? volume : 0;
}

static uint apu_pulse2_sample()
{
  if (apu.pulse2_len == 0 || apu_sweep_mute(apu.pulse2_period, apu_pulse2_sweep())) {
    return 0;
  }
  uint sample = (apu_dyn.duty_table[apu.pulse2_duty] >> apu.pulse2_phase) & 1;
  uint volume = apu.pulse2_env_const ? apu.pulse2_env_vol : apu.pulse2_env_decay;
  return sample ? volume : 0;
}

static uint apu_noise_sample()
{
  if (apu.noise_len == 0) {
    return 0;
  }
  uint sample = !(apu.noise_shift & 1);
  uint volume = apu.noise_env_const ? apu.noise_env_vol : apu.noise_env_decay;
  return sample ? volume : 0;
}

static uint apu_dmc_sample()
{
  if (APU_FILTER_DMC) {
    if ((apu.cyc % 4) == 0) {
      if (apu.dmc_val_filtered < apu.dmc_val) {
        ++apu.dmc_val_filtered;
      } else if (apu.dmc_val_filtered > apu.dmc_val) {
        --apu.dmc_val_filtered;
      }
    }
    return apu.dmc_val_filtered;
  }
  return apu.dmc_val;
}

static uint apu_tnd_table_idx(uint t, uint n, uint d)
{
#if APU_MIX_LUT_APPROX
  return 3 * t + 2 * n + d;
#else
  return (t << 11) | (n << 7) | d;
#endif
}

static uint apu_sample()
{
  if (apu.sample_dirty) {
    apu.pulse1_val = apu_pulse1_sample();
    apu.pulse2_val = apu_pulse2_sample();
    apu.noise_val = apu_noise_sample();
    uint pulse = apu_dyn.pulse_table[apu.pulse1_val + apu.pulse2_val];
    uint tnd = apu_dyn.tnd_table[apu_tnd_table_idx(apu.triangle_val, apu.noise_val, apu_dmc_sample())];
    apu.sample = pulse + tnd;
    apu.sample_dirty = false;
  }
  return apu.sample;
}

void apu_power()
{
  memset(&apu_dyn, 0, sizeof(apu_dyn));
  memset(&apu, 0, sizeof(apu));

  mn_audio_freq(apu_freq);

  switch (mn_region_get()) {
    case MN_REGION_NTSC:
      apu.step1 = APU_STEP1_NTSC;
      apu.step2 = APU_STEP2_NTSC;
      apu.step3 = APU_STEP3_NTSC;
      apu.step4 = APU_STEP4_NTSC;
      apu.step5 = APU_STEP5_NTSC;
      memcpy(apu_dyn.noise_period_table, apu_noise_period_ntsc, sizeof(apu_dyn.noise_period_table));
      memcpy(apu_dyn.dmc_period_table, apu_dmc_period_ntsc, sizeof(apu_dyn.dmc_period_table));
      memcpy(apu_dyn.duty_table, apu_duty_table, sizeof(apu_dyn.duty_table));
      break;
    case MN_REGION_PAL:
      apu.step1 = APU_STEP1_PAL;
      apu.step2 = APU_STEP2_PAL;
      apu.step3 = APU_STEP3_PAL;
      apu.step4 = APU_STEP4_PAL;
      apu.step5 = APU_STEP5_PAL;
      memcpy(apu_dyn.noise_period_table, apu_noise_period_pal, sizeof(apu_dyn.noise_period_table));
      memcpy(apu_dyn.dmc_period_table, apu_dmc_period_pal, sizeof(apu_dyn.dmc_period_table));
      memcpy(apu_dyn.duty_table, apu_duty_table, sizeof(apu_dyn.duty_table));
      break;
    case MN_REGION_DENDY:
      apu.step1 = APU_STEP1_NTSC;
      apu.step2 = APU_STEP2_NTSC;
      apu.step3 = APU_STEP3_NTSC;
      apu.step4 = APU_STEP4_NTSC;
      apu.step5 = APU_STEP5_NTSC;
      memcpy(apu_dyn.noise_period_table, apu_noise_period_ntsc, sizeof(apu_dyn.noise_period_table));
      memcpy(apu_dyn.dmc_period_table, apu_dmc_period_ntsc, sizeof(apu_dyn.dmc_period_table));
      apu_dyn.duty_table[0] = apu_duty_table[0];
      apu_dyn.duty_table[1] = apu_duty_table[2];
      apu_dyn.duty_table[2] = apu_duty_table[1];
      apu_dyn.duty_table[3] = apu_duty_table[3];
      break;
  }

  // scale table values to i16 and upscale to << 10, reserving 6 bit for accum sum (~40 samples)
  apu_dyn.pulse_table[0] = 0;
  for (uint i = 1; i < 31; ++i) {
#if APU_MIX_LUT_APPROX
    double val = 95.52 / (8128.0 / i + 100.0);
#else
    double val = 95.88 / (8128.0 / i + 100.0);
#endif
    apu_dyn.pulse_table[i] = (uint)(val * 32767.0 * 1024.0 + 0.5);
  }

#if APU_MIX_LUT_APPROX
  for (int i = 0; i < 203; ++i) {
    double val = 163.67 / (24329.0 / i + 100.0);
    apu_dyn.tnd_table[i] = (uint)(val * 32767.0 * 1024.0 + 0.5);
  }
#else
  for (int t = 0; t < 16; ++t) {
    for (int n = 0; n < 16; ++n) {
      for (int d = 0; d < 128; ++d) {
        if (t == 0 && n == 0 && d == 0) {
          apu_dyn.tnd_table[apu_tnd_table_idx(t, n, d)] = 0;
        } else {
          double val = 159.79 / ((1.0 / ((double)t / 8227.0 + (double)n / 12241.0 + (double)d / 22638.0)) + 100.0);
          apu_dyn.tnd_table[apu_tnd_table_idx(t, n, d)] = (uint)(val * 32767.0 * 1024.0 + 0.5);
        }
      }
    }
  }
#endif

  apu.noise_shift = 1;
  apu.dither = 0xDEAD;
  apu.mix_hp_x = apu_sample() << 5;
  apu.dmc_timer = apu_dyn.dmc_period_table[apu.dmc_period];
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
  apu.dmc_val_pending = 0;
  apu.dmc_val_changed = false;

  apu.pulse1_enabled = false;
  apu.pulse2_enabled = false;
  apu.triangle_enabled = false;
  apu.noise_enabled = false;
  apu.dmc_enabled = false;

  apu.frame_irq = false;
  apu.dmc_irq = false;
  apu.irq_dirty = true;

  apu.sample_dirty = true;

  apu.cyc_reset = apu_is_put_phase() ? 3 : 4;
}

uint apu_bus_read(uint addr, uint val, bool trace)
{
  switch (addr & 0x1F) {
    case 0x15: {
      uint status = apu_get_status();
      if (!trace) {
        apu.frame_irq = false;
        apu.irq_dirty = true;
      }
      val = (val & APU_STATUS_OPENBUS) | status;
      break;
    }
  }
  if (apu.test_mode) {
    switch (addr & 0x1F) {
      case 0x18: val = apu.pulse1_val + apu.pulse2_val; break;
      case 0x19: val = apu.triangle_val | (apu.noise_val << 4); break;
      case 0x1A: val = apu.dmc_val; break;
    }
  }
  apu_trace("read addr=%04X val=%02X", addr, val);
  return val;
}

void apu_bus_write(uint addr, uint val)
{
  apu_trace("write addr=%04X val=%02X", addr, val);
  apu.sample_dirty = true;
  switch (addr & 0x1F) {
    case 0x00:
      apu.pulse1_duty = ((val >> 6) & 3);
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
    case 0x02: apu.pulse1_period = (apu.pulse1_period & 0x0700) | (val & 0xFF); break;
    case 0x03:
      apu.pulse1_period = (apu.pulse1_period & 0x00FF) | ((val & 0x07) << 8);
      apu.pulse1_phase = 0;
      apu.pulse1_env_start = true;
      if (!apu.pulse1_dec) {
        apu.pulse1_len = apu.pulse1_enabled ? apu_channel_len[(val >> 3) & 0x1F] : 0;
      }
      break;
    case 0x04:
      apu.pulse2_duty = ((val >> 6) & 3);
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
    case 0x06: apu.pulse2_period = (apu.pulse2_period & 0x0700) | (val & 0xFF); break;
    case 0x07:
      apu.pulse2_period = (apu.pulse2_period & 0x00FF) | ((val & 0x07) << 8);
      apu.pulse2_phase = 0;
      apu.pulse2_env_start = true;
      if (!apu.pulse2_dec) {
        apu.pulse2_len = apu.pulse2_enabled ? apu_channel_len[(val >> 3) & 0x1F] : 0;
      }
      break;
    case 0x08:
      apu.triangle_halt = (val & 0x80);
      apu.triangle_reload_val = (val & 0x7F);
      break;
    case 0x0A: apu.triangle_period = (apu.triangle_period & 0x0700) | (val & 0xFF); break;
    case 0x0B:
      apu.triangle_period = (apu.triangle_period & 0x00FF) | ((val & 0x07) << 8);
      apu.triangle_linear_reload = true;
      if (!apu.triangle_dec) {
        apu.triangle_len = apu.triangle_enabled ? apu_channel_len[(val >> 3) & 0x1F] : 0;
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
        apu.noise_len = apu.noise_enabled ? apu_channel_len[(val >> 3) & 0x1F] : 0;
      }
      break;
    case 0x10:
      apu.dmc_loop = (val & 0x40);
      apu.dmc_period = (val & 0x0F);
      apu.dmc_irq_enabled = (val & 0x80);
      if (!apu.dmc_irq_enabled) {
        apu.dmc_irq = false;
        apu.irq_dirty = true;
      }
      break;
    case 0x11:
      apu.dmc_val_pending = (val & 0x7F);
      apu.dmc_val_changed = true;
      break;
    case 0x12: apu.dmc_sample_addr = 0xC000 + ((val & 0xFF) * 64); break;
    case 0x13: apu.dmc_sample_len = ((val & 0xFF) * 16) + 1; break;
    case 0x15: {
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
        apu.dmc_stop = apu_is_put_phase() ? 2 : 3;
      } else if (apu.dmc_len == 0) {
        apu.dmc_load = apu_is_put_phase() ? 2 : 3;
      }
      apu.dmc_irq = false;
      apu.irq_dirty = true;
      break;
    }
    case 0x17: {
      apu.mode5 = (val & 0x80);
      apu.frame_irq_disabled = (val & 0x40);
      if (apu.frame_irq_disabled) {
        apu.frame_irq = false;
        apu.irq_dirty = true;
      }
      apu.cyc_reset = apu_is_put_phase() ? 3 : 4;
      break;
    }
    case 0x1A:
      // todo: W$401A: [L..T TTTT] - set state of triangle's sequencer to T, and lock all channels if L=1
      // (pulse+noise always output current volume, triangle/DPCM no longer advance)
      break;
  }
}

static void apu_pulse1_tick()
{
  if (apu.pulse1_timer == 0) {
    apu.pulse1_timer = apu.pulse1_period;
    apu.pulse1_phase = (apu.pulse1_phase - 1) & 7;
    apu.sample_dirty = true;
  } else {
    --apu.pulse1_timer;
  }
}

static void apu_pulse2_tick()
{
  if (apu.pulse2_timer == 0) {
    apu.pulse2_timer = apu.pulse2_period;
    apu.pulse2_phase = (apu.pulse2_phase - 1) & 7;
    apu.sample_dirty = true;
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
      apu.triangle_val = apu_triangle_table[apu.triangle_phase];
      apu.sample_dirty = true;
    }
  } else {
    --apu.triangle_timer;
  }
}

static void apu_noise_tick()
{
  if (apu.noise_timer == 0) {
    apu.noise_timer = apu_dyn.noise_period_table[apu.noise_period];
    uint bit1 = apu.noise_shift & 1;
    uint bit2 = apu.noise_mode ? ((apu.noise_shift >> 6) & 1) : ((apu.noise_shift >> 1) & 1);
    uint feedback = bit1 ^ bit2;
    apu.noise_shift >>= 1;
    apu.noise_shift |= (feedback << 14);
    apu.sample_dirty = true;
  }
  --apu.noise_timer;
}

void apu_dmc_dma(uint val)
{
  apu_trace_dmc("dmc dma val=%02X cpu_cyc_corrected=%d", val, cpu_cyc() - 1);
  apu.dmc_buf = val;
  apu.dmc_has_buf = true;
  if (apu.dmc_len > 0) {
    if (++apu.dmc_addr > 0xFFFF) {
      apu.dmc_addr = 0x8000;
    }
    if (--apu.dmc_len == 0) {
      if (apu.dmc_loop) {
        apu.dmc_addr = apu.dmc_sample_addr;
        apu.dmc_len = apu.dmc_sample_len;
      } else {
        if (apu.dmc_irq_enabled) {
          apu.dmc_irq = true;
          apu.irq_dirty = true;
        }
        apu.dmc_stop = 3; // NES bug: implicit stop
      }
    }
  }
}

static void apu_dmc_swap()
{
  apu_trace_dmc("dmc swap");
  apu.dmc_out = apu.dmc_buf;
  apu.dmc_has_buf = false;
  apu.dmc_has_out = true;
}

static void apu_dmc_tick()
{
  if (apu.dmc_timer == 4) { // todo: need to explain "4" or refactor delays dmc_reload/dmc_stop/dmc_wait_buf
    if (apu.dmc_has_out) {
      if (apu.dmc_out & 1) {
        if (apu.dmc_val <= 125) {
          apu.dmc_val_pending = apu.dmc_val + 2;
          apu.dmc_val_changed = true;
        }
      } else {
        if (apu.dmc_val >= 2) {
          apu.dmc_val_pending = apu.dmc_val - 2;
          apu.dmc_val_changed = true;
        }
      }
      apu.dmc_out >>= 1;
    }
    if (apu.dmc_out_bit == 0) {
      apu.dmc_out_bit = 8;
      apu.dmc_has_out = false;
      if (apu.dmc_has_buf) {
        apu_dmc_swap();
      } else {
        apu.dmc_wait_buf = 4; // NES bug: unexpected DMA may occur before reload
      }
      if (apu.dmc_len > 0 && !apu.dmc_has_buf) {
        apu.dmc_reload = 3;
      }
    }
    --apu.dmc_out_bit;
  }
  if (--apu.dmc_timer == 0) {
    apu.dmc_timer = apu_dyn.dmc_period_table[apu.dmc_period];
  }

  if (apu.dmc_wait_buf > 0) {
    --apu.dmc_wait_buf;
    if (apu.dmc_has_buf) {
      apu_dmc_swap();
      apu.dmc_wait_buf = 0;
    }
  }

  if (apu.dmc_val_changed) { // conflict 4011 with dmc_timer
    apu.dmc_val = apu.dmc_val_pending;
    apu.dmc_val_changed = false;
    apu.sample_dirty = true;
  }

  if (apu.dmc_load > 0) {
    if (--apu.dmc_load == 0) {
      apu.dmc_addr = apu.dmc_sample_addr;
      apu.dmc_len = apu.dmc_sample_len;
      if (apu.dmc_len > 0 && !apu.dmc_has_buf) {
        cpu_dmc(true, apu.dmc_addr);
        apu_trace_dmc("dmc load dmc_addr=%04X", apu.dmc_addr);
      }
    }
  }
  if (apu.dmc_reload > 0) {
    if (--apu.dmc_reload == 0) {
      cpu_dmc(true, apu.dmc_addr);
      apu_trace_dmc("dmc reload dmc_addr=%04X", apu.dmc_addr);
    }
  }
  if (apu.dmc_stop > 0) {
    if (--apu.dmc_stop == 0) {
      apu.dmc_len = 0;
      apu.dmc_reload = 0;
      cpu_dmc(false, 0);
      apu_trace_dmc("dmc stop");
    }
  }
}

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

  uint pulse1_sweep = apu_pulse1_sweep();
  if (apu.pulse1_sweep_timer == 0 && apu.pulse1_sweep_enabled && apu.pulse1_sweep_shift > 0
      && !apu_sweep_mute(apu.pulse1_period, pulse1_sweep))
  {
    apu.pulse1_period = pulse1_sweep;
  }
  if (apu.pulse1_sweep_timer == 0 || apu.pulse1_sweep_reload) {
    apu.pulse1_sweep_timer = apu.pulse1_sweep_period;
    apu.pulse1_sweep_reload = false;
  } else {
    --apu.pulse1_sweep_timer;
  }

  uint pulse2_sweep = apu_pulse2_sweep();
  if (apu.pulse2_sweep_timer == 0 && apu.pulse2_sweep_enabled && apu.pulse2_sweep_shift > 0
      && !apu_sweep_mute(apu.pulse2_period, pulse2_sweep))
  {
    apu.pulse2_period = pulse2_sweep;
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
    apu.sample_dirty = true;
  } else if (apu.pulse1_env_timer == 0) {
    apu.pulse1_env_timer = apu.pulse1_env_vol;
    if (apu.pulse1_env_decay > 0) {
      --apu.pulse1_env_decay;
    } else if (apu.pulse1_halt) {
      apu.pulse1_env_decay = 15;
    }
    apu.sample_dirty = true;
  } else {
    --apu.pulse1_env_timer;
  }

  if (apu.pulse2_env_start) {
    apu.pulse2_env_start = false;
    apu.pulse2_env_decay = 15;
    apu.pulse2_env_timer = apu.pulse2_env_vol;
    apu.sample_dirty = true;
  } else if (apu.pulse2_env_timer == 0) {
    apu.pulse2_env_timer = apu.pulse2_env_vol;
    if (apu.pulse2_env_decay > 0) {
      --apu.pulse2_env_decay;
    } else if (apu.pulse2_halt) {
      apu.pulse2_env_decay = 15;
    }
    apu.sample_dirty = true;
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
    apu.sample_dirty = true;
  } else if (apu.noise_env_timer == 0) {
    apu.noise_env_timer = apu.noise_env_vol;
    if (apu.noise_env_decay > 0) {
      --apu.noise_env_decay;
    } else if (apu.noise_halt) {
      apu.noise_env_decay = 15;
    }
    apu.sample_dirty = true;
  } else {
    --apu.noise_env_timer;
  }
}

static int apu_mix_high_pass(int sample)
{
  apu.mix_hp_y = (apu_dyn.mix_hp_alpha * ((i64)apu.mix_hp_y + (i64)sample - (i64)apu.mix_hp_x)) >> 16;
  apu.mix_hp_x = sample;
  return apu.mix_hp_y;
}

static int apu_mix_low_pass(int sample)
{
  apu.mix_lp_y = apu.mix_lp_y + ((apu_dyn.mix_lp_alpha * ((i64)sample - (i64)apu.mix_lp_y)) >> 16);
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

static int apu_mix_dither(int sample, int mask)
{
  if (APU_FILTER_DITHER) {
    int rand1 = apu_xorshift32() & mask;
    int rand2 = apu_xorshift32() & mask;
    int dither_noise = rand1 - rand2;
    sample += dither_noise;
  }
  return sample;
}

static int apu_mix_clip(int sample)
{
  if (sample > 32767) {
    tracef("clip %08X -> %08X\n", sample, 32767);
    sample = 32767;
  } else if (sample < -32768) {
    tracef("clip %08X -> %08X\n", sample, -32768);
    sample = -32768;
  }
  return sample;
}

static void apu_mix()
{
  apu.mix_accum += apu_sample();
  ++apu.mix_sample_count;

  apu.time_current += (1ULL << 32);
  if (apu.time_current >= apu_dyn.time_target) {
    if (apu.out_size <= 1024) {
      // mix_accum already has << 10 in table values, upsample to 15, reserve 1 bit for filters
      int sample = (apu.mix_accum / apu.mix_sample_count) << 5;
      sample = apu_mix_high_pass(sample);
      sample = apu_mix_low_pass(sample);
      sample = apu_mix_dither(sample, 0x7FFF);
      sample = apu_mix_clip(sample >> 15);
      apu_dyn.out_data[apu.out_size++] = sample;
    }
    apu.mix_accum = 0;
    apu.mix_sample_count = 0;
    apu.time_current -= apu_dyn.time_target;
  }
}

void apu_tick()
{
  if (apu.cyc_reset > 0) {
    if (--apu.cyc_reset == 0) {
      apu.cyc = 0;
      apu_trace("reset");
    }
  }

  if (apu_is_get_phase()) { // better results with GET phase in apu_mixer/square.nes
    apu_pulse1_tick();
    apu_pulse2_tick();
  }
  apu_noise_tick();
  apu_triangle_tick();
  apu_dmc_tick();
  apu_mix();

  uint last_step = (apu.mode5 ? apu.step5 : apu.step4);
  if (apu.step_env) {
    apu.step_env = false;
    apu_update_env();
    apu_trace("env pulse1_decay=%d pulse2_decay=%d triangle_linear=%d noise_decay=%d", apu.pulse1_env_decay,
              apu.pulse2_env_decay, apu.triangle_linear_val, apu.noise_env_decay);
  } else if (apu.cyc == apu.step1 || apu.cyc == apu.step2 || apu.cyc == apu.step3 || apu.cyc == last_step
             || (apu.mode5 && apu.cyc_reset == 2))
  {
    apu.step_env = true;
  }

  if (apu.step_len) {
    apu.step_len = false;
    apu_update_len_put();
    apu_trace("len pulse1_len=%d pulse2_len=%d triangle_len=%d noise_len=%d", apu.pulse1_len, apu.pulse2_len,
              apu.triangle_len, apu.noise_len);
  } else if (apu.cyc == apu.step2 || apu.cyc == last_step || (apu.mode5 && apu.cyc_reset == 2)) {
    apu.step_len = true;
    apu_update_len_get();
  }

  if (apu.cyc == last_step || apu.cyc == last_step + 1 || apu.cyc == last_step + 2) {
    if (!apu.mode5) {
      if (!apu.frame_irq_disabled) {
        apu.frame_irq = true;
        apu.irq_dirty = true;
      }
      apu_trace("frame irq");
    }
  }
  if (apu.cyc == last_step + 2) {
    apu.cyc = 0;
  }

  if (apu_is_get_phase()) { // NES bug: IRQ flags change immediately, but status change becomes visible on GET phase
    apu.frame_irq_status = apu.frame_irq || (apu.cyc >= last_step && !apu.mode5); // NES bug: irq status on last step
  }

  if (apu.irq_dirty) {
    map_apu_irq(apu.frame_irq || apu.dmc_irq);
    apu.irq_dirty = false;
  }

  ++apu.cyc;
}
