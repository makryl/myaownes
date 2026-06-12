#include "apu.h"
#include "cpu.h"
#include <string.h>

enum : u16
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

static struct Apu
{
  u16 cyc;
  u8 status;
  bool mode5;
  bool irq_disabled;
  u16 step1;
  u16 step2;
  u16 step3;
  u16 step4;
  u16 step5;
} apu;

void apu_power()
{
  memset(&apu, 0, sizeof(apu));
  apu_reset();

  // todo: load rom
  bool ntsc = true;
  if (ntsc) {
    apu.step1 = 3728;
    apu.step2 = 7456;
    apu.step3 = 11185;
    apu.step4 = 14914;
    apu.step5 = 18640;
  } else {
    apu.step1 = 4156;
    apu.step2 = 8313;
    apu.step3 = 12469;
    apu.step4 = 16626;
    apu.step5 = 20782;
  }
}

void apu_reset()
{
  apu.cyc = -1;
  apu.mode5 = false;
  apu.irq_disabled = true;
  apu.status &= ~APU_STATUS_FRAME_IRQ;
  cpu_irq(false);
}

u8 apu_bus_read(u16 addr, bool trace)
{
  switch (addr & 0x1F) {
    case 0x15: {
      u8 status = apu.status;
      if (!trace) {
        apu.status &= ~APU_STATUS_FRAME_IRQ;
        cpu_irq(false);
      }
      return status;
    }
  }
  return 0;
}

void apu_bus_write(u16 addr, u8 val)
{
  switch (addr & 0x1F) {
    case 0x15: {
      apu.status = (apu.status & APU_STATUS_FRAME_IRQ) | (val & 0x1F);
      break;
    }
    case 0x17: {
      apu.mode5 = val & 0x80;
      apu.irq_disabled = val & 0x40;

      if (apu.irq_disabled) {
        apu.status &= ~APU_STATUS_FRAME_IRQ;
        cpu_irq(false);
      }

      apu.cyc = -1;
      break;
    }
  }
}

void apu_tick()
{
  if ((cpu_cyc() & 1)) {
    u16 last_step = (apu.mode5 ? apu.step5 : apu.step4);

    if (apu.cyc == apu.step1) {
      //
    } else if (apu.cyc == apu.step2) {
      //
    } else if (apu.cyc == apu.step3) {
      //
    } else if (apu.cyc == last_step) {
      if (!apu.irq_disabled && !apu.mode5) {
        apu.status |= APU_STATUS_FRAME_IRQ;
        cpu_irq(true);
      }
      apu.cyc = -1;
    }
    ++apu.cyc;
  }
}
