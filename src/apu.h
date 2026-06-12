#pragma once

#include "myanes.h"

void apu_power();
void apu_reset();
void apu_tick();
u8 apu_bus_read(u16 addr, bool trace);
void apu_bus_write(u16 addr, u8 val);
