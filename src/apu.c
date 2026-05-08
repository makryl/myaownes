#include "apu.h"

u8 apu_bus_read(u16 addr, bool trace)
{
  addr = addr & 0x1F;
  return 0;
}

void apu_bus_write(u16 addr, u8 val)
{
  addr = addr & 0x1F;
  //
}
