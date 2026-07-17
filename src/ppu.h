#pragma once

#include "myaownes.h"

uint ppu_size();
void* ppu_data();
void ppu_power();
void ppu_reset();
u8 ppu_bus_read(u16 addr, bool trace);
void ppu_bus_write(u16 addr, u8 val);
void ppu_tick();
uint ppu_cyc();
uint ppu_dot();
uint ppu_sl();
bool ppu_vblank();
