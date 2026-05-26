#pragma once

typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;
typedef signed char i8;
typedef struct Rom* Rom;

enum
{
  MN_INPUT_A = (1 << 0),
  MN_INPUT_B = (1 << 1),
  MN_INPUT_SELECT = (1 << 2),
  MN_INPUT_START = (1 << 3),
  MN_INPUT_UP = (1 << 4),
  MN_INPUT_DOWN = (1 << 5),
  MN_INPUT_LEFT = (1 << 6),
  MN_INPUT_RIGHT = (1 << 7),
};

Rom mn_rom_file(const char* path);
void mn_rom_release(Rom rom);
void mn_rom_load(Rom rom);
void mn_input(u8* joy1, u8* joy2);
void mn_output(u32* out);
void mn_power();
void mn_reset();
void mn_frame();
const u8* mn_sram();
