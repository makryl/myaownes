#pragma once

#include "myanes.h"

void map_cpu_cyc();
void map_cpu_read(u16 addr, u8* val);
void map_cpu_write(u16 addr, u8 val);
void map_ppu_addr(u16 addr);
void map_ppu_read(u16 addr, u8* val);
void map_ppu_write(u16 addr, u8 val);
void map_apu_irq(bool enabled);
