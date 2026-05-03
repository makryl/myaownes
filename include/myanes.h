#pragma once

typedef unsigned short u16;
typedef unsigned char u8;
typedef signed char i8;
typedef struct NES* NES;

NES mn_nes_file(const char* path);
void mn_nes_release(NES nes);
void mn_load(NES nes);
void mn_reset();
void mn_tick();
