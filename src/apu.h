#pragma once

#include "myaownes.h"

uint apu_size();
void* apu_data();
void apu_power();
void apu_reset();
void apu_tick();
void apu_bus_read(uint addr, u8* val, bool trace);
void apu_bus_write(uint addr, uint val);
void apu_dmc_dma(uint val);
void apu_reset_out();
