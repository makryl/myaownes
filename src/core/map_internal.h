#pragma once

#include "myaownes.h"

enum
{
  MAP_REG_SIZE = 0x100,
};

typedef void (*map_cpu_cyc_cb)();
typedef bool (*map_cpu_read_cb)(uint, uint*, bool);
typedef bool (*map_cpu_write_cb)(uint, uint);
typedef void (*map_cpu_internal_cb)(uint, uint);
typedef void (*map_ppu_addr_cb)(uint);
typedef bool (*map_ppu_read_cb)(uint, uint*);
typedef bool (*map_ppu_write_cb)(uint, uint);

void map_set_cpu_cyc_cb(map_cpu_cyc_cb cb);
void map_set_cpu_read_cb(map_cpu_read_cb cb);
void map_set_cpu_write_cb(map_cpu_write_cb cb);
void map_set_cpu_internal_cb(map_cpu_internal_cb cb);
void map_set_ppu_addr_cb(map_ppu_addr_cb cb);
void map_set_ppu_read_cb(map_ppu_read_cb cb);
void map_set_ppu_write_cb(map_ppu_write_cb cb);

uint map_cpu_read_raw(uint addr, uint val);
void map_cpu_write_raw(uint addr, uint val);
uint map_ppu_read_raw(uint addr, uint val);
void map_ppu_write_raw(uint addr, uint val);

void* map_reg();
uint map_reg_read(uint idx);
void map_reg_write(uint idx, uint val);
void map_irq(bool enabled);
void map_cpu_irq();
void map_clear_pages();

void map_prg_clear_page(uint dp, uint shift);
void map_prg_rom_page(uint sp, uint dp, uint shift);
void map_prg_ram_page(uint sp, uint dp, uint shift, bool readonly);
void map_chr_page(uint sp, uint dp, uint shift);

void map_nt_page(uint sp, uint dp, bool use_chr);
void map_cpu_ciram_page(uint sp, uint dp, bool readonly);
void map_ciram_page(uint sp, uint dp);
void map_ciram_single_low();
void map_ciram_single_high();
void map_ciram_vert_mirror();
void map_ciram_horiz_mirror();
void map_ciram_four_screen();

void map_prg_clear_page_1k(uint dp);
void map_prg_clear_page_2k(uint dp);
void map_prg_clear_page_4k(uint dp);
void map_prg_clear_page_8k(uint dp);
void map_prg_clear_page_16k(uint dp);
void map_prg_clear_page_32k(uint dp);

void map_prg_rom_page_1k(uint sp, uint dp);
void map_prg_rom_page_2k(uint sp, uint dp);
void map_prg_rom_page_4k(uint sp, uint dp);
void map_prg_rom_page_8k(uint sp, uint dp);
void map_prg_rom_page_16k(uint sp, uint dp);
void map_prg_rom_page_32k(uint sp, uint dp);

void map_prg_ram_page_1k(uint sp, uint dp, bool readonly);
void map_prg_ram_page_2k(uint sp, uint dp, bool readonly);
void map_prg_ram_page_4k(uint sp, uint dp, bool readonly);
void map_prg_ram_page_8k(uint sp, uint dp, bool readonly);

void map_chr_page_1k(uint sp, uint dp);
void map_chr_page_2k(uint sp, uint dp);
void map_chr_page_4k(uint sp, uint dp);
void map_chr_page_8k(uint sp, uint dp);

bool map_discrete_load(bool init);
bool map_mmc_load(bool init);
bool map_other_load(bool init);
