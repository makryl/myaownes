#pragma once

typedef unsigned short u16;
typedef unsigned char u8;
typedef signed char i8;
typedef struct Rom* Rom;

Rom mn_nes_file(const char* path);
void mn_nes_release(Rom rom);
void mn_load(Rom rom);
void mn_power();
void mn_reset();
bool mn_frame();
