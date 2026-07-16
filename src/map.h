#pragma once

#include "myaownes.h"

u32 map_size();
void* map_data();
void* map_prg_ram();
void* map_chr_ram();
bool map_ready();
void map_cpu_cyc();
void map_cpu_read(u16 addr, u8* val, bool trace);
void map_cpu_write(u16 addr, u8 val);
void map_ppu_addr(u16 addr);
void map_ppu_read(u16 addr, u8* val);
void map_ppu_write(u16 addr, u8 val);
void map_apu_irq(bool enabled);
bool map_rom_load(Rom rom, bool init);
