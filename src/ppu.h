#pragma once

#include "myanes.h"

void ppu_init();
void ppu_reset();
u8 ppu_bus_read(u16 addr, bool trace);
void ppu_bus_write(u16 addr, u8 val);
void ppu_tick();
