#include "iou.h"
#include "cpu.h"
#include <string.h>

static struct
{
  uint out;
  uint shift_idx;
  uint shift_delay;
  uint pending[2];
  uint shift[2];
} iou;

void iou_power() { memset(&iou, 0, sizeof(iou)); }

void iou_input(uint port1, uint port2)
{
  iou.pending[0] = port1;
  iou.pending[1] = port2;
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
  if ((cpu_cyc() & 1) && (iou.out & 1)) {
    iou.shift[0] = iou.pending[0];
    iou.shift[1] = iou.pending[1];
  }
}
