#pragma once

#include "myaownes.h"

uint map_size();
void* map_data();
bool map_ready();
void map_cpu_cyc();
uint map_cpu_read(uint addr, uint val, bool trace);
void map_cpu_write(uint addr, uint val);
void map_ppu_addr(uint addr);
uint map_ppu_read(uint addr, uint val);
void map_ppu_write(uint addr, uint val);
void map_apu_irq(bool enabled);
bool map_rom_load(mn_rom rom, bool init);
