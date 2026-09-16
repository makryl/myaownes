#include "iou.h"
#include "cpu.h"
#include "common.h"

static struct
{
  uint out;
  uint shift_idx;
  uint shift_delay;
  uint input[2];
  uint mask[2];
  uint shift[2];
} iou;

void iou_power() { memset(&iou, 0, sizeof(iou)); }

static uint iou_dpad_filter_pair(uint input, uint port, uint changed, uint a, uint b)
{
  if (changed & a) {
    if (port & a) {
      input &= ~b;
    } else {
      input |= (port & b);
    }
  } else if (changed & b) {
    if (port & b) {
      input &= ~a;
    } else {
      input |= (port & a);
    }
  }
  return input;
}

static uint iou_dpad_filter(uint input, uint port, uint changed)
{
  input = (input & ~changed) | (port & changed);
  input = iou_dpad_filter_pair(input, port, changed, MN_INPUT_UP, MN_INPUT_DOWN);
  input = iou_dpad_filter_pair(input, port, changed, MN_INPUT_LEFT, MN_INPUT_RIGHT);
  return input;
}

void iou_input(uint port1, uint port2)
{
  uint changed1 = (port1 ^ iou.mask[0]);
  uint changed2 = (port2 ^ iou.mask[1]);
  iou.mask[0] = port1;
  iou.mask[1] = port2;
  iou.input[0] = iou_dpad_filter(iou.input[0], port1, changed1);
  iou.input[1] = iou_dpad_filter(iou.input[1], port2, changed2);
}

uint iou_bus_read(uint addr, uint val, bool trace)
{
  uint idx = (addr & 1);
  if (!trace) {
    iou.shift_idx = idx;
    iou.shift_delay = 2;
  }
  val &= 0xE0;
  val |= (iou.shift[idx] & 1);
  // val |= (val & 1) << 1; // todo: Dendy uses expansion port for joy2?
  return val;
}

void iou_bus_write(uint, uint val) { iou.out = val; }

void iou_tick()
{
  if (iou.shift_delay > 0) {
    if (--iou.shift_delay == 0) {
      iou.shift[iou.shift_idx] = (iou.shift[iou.shift_idx] >> 1) | 0x80;
    }
  }
  if ((cpu_cyc() & 1)) {
    if (iou.input[0] & MN_INPUT_TURBO_A) {
      iou.input[0] ^= MN_INPUT_A;
    }
    if (iou.input[0] & MN_INPUT_TURBO_B) {
      iou.input[0] ^= MN_INPUT_B;
    }
    if (iou.input[1] & MN_INPUT_TURBO_A) {
      iou.input[1] ^= MN_INPUT_A;
    }
    if (iou.input[1] & MN_INPUT_TURBO_B) {
      iou.input[1] ^= MN_INPUT_B;
    }
    if ((iou.out & 1)) {
      iou.shift[0] = (iou.input[0] & 0xFF);
      iou.shift[1] = (iou.input[1] & 0xFF);
    }
  }
}
