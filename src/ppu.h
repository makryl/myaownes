#pragma once

#include "myanes.h"

void ppu_power();
void ppu_reset();
u8 ppu_bus_read(u16 addr, bool trace);
void ppu_bus_write(u16 addr, u8 val);
void ppu_tick();
u32 ppu_cyc();
u16 ppu_dot();
u16 ppu_sl();
bool ppu_vblank();
