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

struct Rom
{
  const char* path;
  const u8* prg_rom;
  const u8* chr_rom;
  u32 prg_rom_size;
  u32 chr_rom_size;
  u32 prg_ram_size;
  u32 chr_ram_size;
  u16 mapper;
  u8 submapper;
  bool vert_mirror;
  bool has_prg_battery;
  bool has_chr_battery;
  bool has_trainer;
  bool alt_mirror;
  bool ntsc;
  bool pal;
  bool dendy;
  bool mapper_error;
};

Rom mn_rom_file(const char* path);
void mn_rom_release(Rom rom);
bool mn_rom_load(Rom rom);
Rom mn_rom_get();
void mn_input(u8* joy1, u8* joy2);
void mn_output(u32* out);
void mn_power();
void mn_reset();
void mn_frame();
const u8* mn_prg_ram();
const u8* mn_chr_ram();
