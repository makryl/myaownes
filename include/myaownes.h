#pragma once

typedef unsigned int uint;
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
  const u8* prg_rom;
  const u8* chr_rom;
  uint prg_rom_size;
  uint chr_rom_size;
  uint prg_ram_size;
  uint chr_ram_size;
  uint mapper;
  uint submapper;
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
bool mn_rom_set(Rom rom);
Rom mn_rom_get();
uint* mn_output();
void mn_output_size(bool auto_aspect, bool overscan, float scale, uint* dw, uint* dh);
void mn_output_fit(bool auto_aspect, bool overscan, float sw, float sh, float* dx, float* dy, float* dw, float* dh);
void mn_power();
void mn_reset();
void mn_frame(u8 joy1, u8 joy2);
bool mn_save(const char* path);
bool mn_load(const char* path);
bool mn_sram_save(const char* path);
bool mn_sram_load(const char* path);
