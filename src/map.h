#pragma once

#include "myanes.h"

u8 map_openbus();
void map_cpu_cyc();
u8 map_cpu_read(u16 addr);
void map_cpu_write(u16 addr, u8 val);
u8 map_ppu_read(u16 addr);
void map_ppu_write(u16 addr, u8 val);
