#pragma once

#include "myaownes.h"

uint map_size();
void* map_data();
bool map_ready();
void map_cpu_cyc();
void map_cpu_read(u16 addr, u8* val, bool trace);
void map_cpu_write(u16 addr, u8 val);
void map_ppu_addr(u16 addr);
void map_ppu_read(u16 addr, u8* val);
void map_ppu_write(u16 addr, u8 val);
void map_apu_irq(bool enabled);
bool map_rom_load(Rom rom, bool init);
