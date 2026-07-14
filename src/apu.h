#pragma once

#include "myaownes.h"

void apu_power();
void apu_reset();
void apu_tick();
void apu_bus_read(u16 addr, u8* val, bool trace);
void apu_bus_write(u16 addr, u8 val);
void apu_dmc_dma(u8 val);
