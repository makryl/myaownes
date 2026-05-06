#pragma once

#include "myanes.h"

u8 ppu_bus_read(u16 addr);
void ppu_bus_write(u16 addr, u8 val);
void ppu_tick();
